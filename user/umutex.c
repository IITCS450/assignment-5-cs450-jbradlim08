#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"
#include "umutex.h"

void
mutex_init(umutex_t *m)
{
  m->locked = 0;
  m->owner = -1;
}

void
mutex_lock(umutex_t *m)
{
  tid_t self;

  self = thread_self();
  while(m->locked){
    if(m->owner == self)
      return;
    thread_yield();
  }

  m->locked = 1;
  m->owner = self;
}

void
mutex_unlock(umutex_t *m)
{
  if(m->locked && m->owner == thread_self()){
    m->owner = -1;
    m->locked = 0;
  }
}
