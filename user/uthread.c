#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

#define MAX_UTHREADS 8
#define UTHREAD_STACK_SIZE 4096

enum thread_state {
  T_FREE = 0,
  T_RUNNING,
  T_RUNNABLE,
  T_ZOMBIE
};

// Matches the stack layout used by user/uthread_switch.S.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

struct thread {
  tid_t tid;
  enum thread_state state;
  void (*start_routine)(void*);
  void *arg;
  void *stack;
  struct context *context;
};

extern void thread_switch(struct context **old, struct context *new);

static struct thread threads[MAX_UTHREADS];
static int current_tid = -1;
static int library_initialized;

static void thread_stub(void);
static void thread_exit(void);

static struct thread*
thread_by_tid(tid_t tid)
{
  int i;

  for(i = 0; i < MAX_UTHREADS; i++){
    if(threads[i].state != T_FREE && threads[i].tid == tid)
      return &threads[i];
  }
  return 0;
}

static struct thread*
alloc_thread_slot(void)
{
  int i;

  for(i = 0; i < MAX_UTHREADS; i++){
    if(threads[i].state == T_FREE)
      return &threads[i];
  }
  return 0;
}

static void
clear_thread(struct thread *t)
{
  t->tid = -1;
  t->state = T_FREE;
  t->start_routine = 0;
  t->arg = 0;
  t->stack = 0;
  t->context = 0;
}

static struct thread*
pick_next_thread(void)
{
  int i;
  int start;

  if(current_tid < 0)
    start = 0;
  else
    start = (current_tid + 1) % MAX_UTHREADS;

  for(i = 0; i < MAX_UTHREADS; i++){
    struct thread *t = &threads[(start + i) % MAX_UTHREADS];
    if(t->state == T_RUNNABLE)
      return t;
  }
  return 0;
}

static void
thread_stub(void)
{
  struct thread *self = thread_by_tid(current_tid);

  if(self == 0 || self->start_routine == 0)
    exit();

  self->start_routine(self->arg);
  thread_exit();
}

static void
thread_exit(void)
{
  struct thread *self;
  struct thread *next;

  self = thread_by_tid(current_tid);
  if(self == 0)
    exit();

  self->state = T_ZOMBIE;
  next = pick_next_thread();
  if(next == 0)
    exit();

  next->state = T_RUNNING;
  current_tid = next->tid;
  thread_switch(&self->context, next->context);

  // A finished thread should never resume.
  exit();
}

void
thread_init(void)
{
  int i;

  for(i = 0; i < MAX_UTHREADS; i++)
    clear_thread(&threads[i]);

  threads[0].tid = 0;
  threads[0].state = T_RUNNING;
  threads[0].context = 0;
  current_tid = 0;
  library_initialized = 1;
}

tid_t
thread_create(void (*fn)(void*), void *arg)
{
  struct thread *t;
  char *stack_top;

  if(!library_initialized)
    thread_init();

  t = alloc_thread_slot();
  if(t == 0)
    return -1;

  t->stack = malloc(UTHREAD_STACK_SIZE);
  if(t->stack == 0){
    clear_thread(t);
    return -1;
  }

  stack_top = (char*)t->stack + UTHREAD_STACK_SIZE;
  stack_top -= sizeof(struct context);
  t->context = (struct context*)stack_top;
  memset(t->context, 0, sizeof(struct context));

  t->tid = (tid_t)(t - threads);
  t->state = T_RUNNABLE;
  t->start_routine = fn;
  t->arg = arg;
  t->context->eip = (uint)thread_stub;

  return t->tid;
}

void
thread_yield(void)
{
  struct thread *self;
  struct thread *next;

  if(!library_initialized)
    thread_init();

  self = thread_by_tid(current_tid);
  if(self == 0)
    return;

  next = pick_next_thread();
  if(next == 0)
    return;

  if(self->state == T_RUNNING)
    self->state = T_RUNNABLE;

  next->state = T_RUNNING;
  current_tid = next->tid;
  thread_switch(&self->context, next->context);

  self = thread_by_tid(current_tid);
  if(self != 0)
    self->state = T_RUNNING;
}

int
thread_join(tid_t tid)
{
  struct thread *target;

  if(!library_initialized)
    thread_init();

  if(tid == current_tid)
    return -1;

  target = thread_by_tid(tid);
  if(target == 0)
    return -1;

  while(target->state != T_ZOMBIE)
    thread_yield();

  if(target->stack != 0)
    free(target->stack);
  clear_thread(target);
  return 0;
}

tid_t
thread_self(void)
{
  if(!library_initialized)
    thread_init();

  return current_tid;
}
