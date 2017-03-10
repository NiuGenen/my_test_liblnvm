
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "BGThreads.h"
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include <string.h>

static void PthreadCall(const char *label, int result) {
    if (result != 0) {
        printf("pthread %s: %s\n", label, strerror(result));
        abort();
    }
}

Mutex::Mutex() { PthreadCall("init mutex", pthread_mutex_init(&mu_, NULL)); }

Mutex::~Mutex() { PthreadCall("destroy mutex", pthread_mutex_destroy(&mu_)); }

void Mutex::Lock() { PthreadCall("lock", pthread_mutex_lock(&mu_)); }

void Mutex::Unlock() { PthreadCall("unlock", pthread_mutex_unlock(&mu_)); }

CondVar::CondVar(Mutex *mu)
    : mu_(mu) {
    PthreadCall("init cv", pthread_cond_init(&cv_, NULL));
}

CondVar::~CondVar() { PthreadCall("destroy cv", pthread_cond_destroy(&cv_)); }

void CondVar::Wait() {
    PthreadCall("wait", pthread_cond_wait(&cv_, &mu_->mu_));
}

void CondVar::Signal() {
    PthreadCall("signal", pthread_cond_signal(&cv_));
}

void CondVar::SignalAll() {
    PthreadCall("broadcast", pthread_cond_broadcast(&cv_));
}

namespace {
int DBGCounts = 0;
#define DBG "[DBG] - "
}

BGThreads::~BGThreads() { //need to wait for the threads.
//    MutexLock l(&mutex_);
//    while (elems_ > 0) {
//    StartAll();
//
//    }
    while (elems_ > 0) {
        printf(DBG "Destory BG: %d elems left.\n", elems_);
        cv_.SignalAll();
        sleep(1);
    }

    //clean
    for (int i = 0; i < elems_; i++) {
        PthreadCall("join", pthread_join(slots_[i].thread_, NULL));
    }
    delete[] slots_;
}


void BGThreads::AddSchedule(function_t func, void *arg) {
    int i;
    wraparg *w;

    if (elems_ > (size_ >> 1)) {
        Resize(size_ << 1);
    }

    {
        MutexLock l(&mu_);
        //find a slot and put the func.
        for (i = elems_; i < size_; i++) {
            if (slots_[i].state_ == Empty) {
                break;
            }
        }
        elems_++;
        w = new wraparg(i, func, arg, this);
        slots_[i].state_ = Pending;
        slots_[i].args_ = w;
    }

    PthreadCall("start thread", pthread_create(&slots_[i].thread_, NULL,  &BGThreads::Wrapper, (void *)w));
}
void BGThreads::StartAll() {
    MutexLock l(&mutex_);
    printf(DBG "BGThreads: Ready to run %d threads.\n", elems_);
    cv_.SignalAll();
}
void BGThreads::Resize(int size) {
    {
        printf(DBG "Resize from %d to %d.\n", size_, size);
        MutexLock l(&mu_);
        ThreadItem *old_ = slots_;
        ThreadItem *new_ = new ThreadItem[size];
        int new_elems_ = 0;
        if (old_ == NULL) {
            goto OUT;
        }

        for (int i = 0; i < elems_; i++) {
            if (old_[i].state_ == Pending) {
                new_[new_elems_].thread_ = old_[i].thread_;
                new_[new_elems_].args_ = old_[i].args_;
                new_[new_elems_].args_->islot = new_elems_;
                new_elems_++;
            }
        }
        delete[] old_;

    OUT:
        slots_ = new_;
        size_ = size;
        elems_ = new_elems_;
    }
}



void* BGThreads::Wrapper(void *arg) {
    wraparg *w = (wraparg *)arg;
    BGThreads *bg = w->BGptr;
    bg->mutex_.Lock();
    printf("1 thread create lock & wait.\n");
    bg->cv_.Wait();
    bg->mutex_.Unlock();

    printf("1 thread is signaled.\n");
    bg->slots_[w->islot].ret_ = w->func(w->arg);
    bg->slots_[w->islot].state_ = Finished;
    bg->elems_--;
    bg->bg_cv_.Signal();

    printf(DBG "Thread %d stops. Ret: %d.\n", DBGCounts++, bg->slots_[w->islot].ret_);

    delete w;

    return NULL;
}
