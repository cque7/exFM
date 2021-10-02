#pragma once

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

class PthreadMutexWithCond {
 protected:
  pthread_mutex_t lock_;
  pthread_cond_t cond_;

 public:
  PthreadMutexWithCond() {
    pthread_mutex_init(&lock_, (pthread_mutexattr_t *)NULL);
    pthread_cond_init(&cond_, NULL);
  }

  inline void lock() { pthread_mutex_lock(&lock_); }

  inline void unlock() { pthread_mutex_unlock(&lock_); }

  inline int tryLock() { return pthread_mutex_trylock(&lock_); }

  inline void wait() { pthread_cond_wait(&cond_, &lock_); }

  inline void notify() { pthread_cond_signal(&cond_); }
  // 禁用拷贝
 private:
  PthreadMutexWithCond(const PthreadMutexWithCond &ohter);
  PthreadMutexWithCond &operator=(const PthreadMutexWithCond &that);
};