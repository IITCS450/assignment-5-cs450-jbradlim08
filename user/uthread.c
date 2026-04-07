#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

// Cooperative user-level threading state. Context switching is still TODO,
// but the thread table, IDs, and stack bookkeeping live here.

#define MAX_UTHREADS 8
#define UTHREAD_STACK_SIZE 4096

enum thread_state {
  T_FREE = 0,
  T_RUNNING,
  T_RUNNABLE,
  T_ZOMBIE
};

struct thread {
  tid_t tid;
  enum thread_state state;
  void (*start_routine)(void*);
  void *arg;
  void *stack;
  void *stack_top;
  int waiting_on;
};

static struct thread threads[MAX_UTHREADS];
static int current_tid = -1;
static int library_initialized;

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
  t->stack_top = 0;
  t->waiting_on = -1;
}

void
thread_init(void)
{
  int i;

  for(i = 0; i < MAX_UTHREADS; i++)
    clear_thread(&threads[i]);

  threads[0].tid = 0;
  threads[0].state = T_RUNNING;
  threads[0].waiting_on = -1;
  current_tid = 0;
  library_initialized = 1;
}

tid_t
thread_create(void (*fn)(void*), void *arg)
{
  struct thread *t;

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

  t->tid = (tid_t)(t - threads);
  t->state = T_RUNNABLE;
  t->start_routine = fn;
  t->arg = arg;
  t->stack_top = (char*)t->stack + UTHREAD_STACK_SIZE;
  t->waiting_on = -1;

  // TODO: Prepare the initial saved context so the scheduler can enter fn(arg).
  return t->tid;
}

void
thread_yield(void)
{
  struct thread *self;

  if(!library_initialized)
    thread_init();

  self = thread_by_tid(current_tid);
  if(self == 0)
    return;

  // TODO: Pick the next runnable thread and switch contexts.
  self->state = T_RUNNING;
}

int
thread_join(tid_t tid)
{
  struct thread *target;

  if(!library_initialized)
    thread_init();

  target = thread_by_tid(tid);
  if(target == 0 || tid == current_tid)
    return -1;

  // TODO: Yield until the target becomes T_ZOMBIE, then reclaim its stack.
  return (target->state == T_ZOMBIE) ? 0 : -1;
}
