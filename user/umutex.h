#pragma once

typedef struct {
  volatile int locked;
  int owner;
} umutex_t;
void mutex_init(umutex_t *m);
void mutex_lock(umutex_t *m);
void mutex_unlock(umutex_t *m);
