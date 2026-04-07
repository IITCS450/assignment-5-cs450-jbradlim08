#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"
#include "umutex.h"

#define N 8
#define PRODUCERS 2
#define ITEMS_PER_PRODUCER 100
#define TOTAL_ITEMS (PRODUCERS * ITEMS_PER_PRODUCER)
static int buf[N], head, tail, count;
static umutex_t mu;
static int produced[PRODUCERS];
static int consumed;
static int duplicate_count;
static int invalid_count;
static int seen[PRODUCERS][ITEMS_PER_PRODUCER];

static void
producer(void *arg)
{
  int id = (int)arg;
  int producer_idx = id - 1;
  int i;

  for(i = 0; i < ITEMS_PER_PRODUCER; ){
    mutex_lock(&mu);
    if(count < N){
      buf[tail] = id * 1000 + i;
      tail = (tail+1)%N;
      count++;
      produced[producer_idx]++;
      i++;
    }
    mutex_unlock(&mu);
    thread_yield();
  }
}

static void
consumer(void *arg)
{
  (void)arg;
  while(1){
    int x;
    int producer_idx;
    int item_idx;

    mutex_lock(&mu);
    if(consumed >= TOTAL_ITEMS){
      mutex_unlock(&mu);
      break;
    }

    if(count == 0){
      mutex_unlock(&mu);
      thread_yield();
      continue;
    }

    x = buf[head];
    head = (head+1)%N;
    count--;
    consumed++;

    producer_idx = (x / 1000) - 1;
    item_idx = x % 1000;

    if(producer_idx < 0 || producer_idx >= PRODUCERS ||
       item_idx < 0 || item_idx >= ITEMS_PER_PRODUCER){
      invalid_count++;
    } else if(seen[producer_idx][item_idx]){
      duplicate_count++;
    } else {
      seen[producer_idx][item_idx] = 1;
    }

    if(consumed % 50 == 0)
      printf(1, "consumer got %d/%d (last=%d)\n", consumed, TOTAL_ITEMS, x);

    mutex_unlock(&mu);
    thread_yield();
  }
}

int
main(void)
{
  tid_t p1, p2, c1;

  thread_init();
  mutex_init(&mu);

  p1 = thread_create(producer, (void*)1);
  p2 = thread_create(producer, (void*)2);
  c1 = thread_create(consumer, 0);

  if(p1 < 0 || p2 < 0 || c1 < 0){
    printf(1, "test_pc: thread creation failed\n");
    exit();
  }

  thread_join(p1);
  thread_join(p2);
  thread_join(c1);

  printf(1, "test_pc: produced=[%d,%d] consumed=%d count=%d\n",
         produced[0], produced[1], consumed, count);

  if(produced[0] == ITEMS_PER_PRODUCER &&
     produced[1] == ITEMS_PER_PRODUCER &&
     consumed == TOTAL_ITEMS &&
     count == 0 &&
     invalid_count == 0 &&
     duplicate_count == 0){
    printf(1, "test_pc: PASS\n");
  } else {
    printf(1, "test_pc: FAIL invalid=%d duplicate=%d\n",
           invalid_count, duplicate_count);
  }

  exit();
}
