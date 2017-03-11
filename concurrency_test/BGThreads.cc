
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

BGThreads::BGThreads() :
    size_(0), elems_(0), slots_(0),
    ctrl_cv_(&ctrl_lock), cv_(&mutex_), shouldstop_(false) {
    Resize(4);
    CtrlArg *c = new CtrlArg(this);
    PthreadCall("start ctrl thread", pthread_create(&ctrl_, NULL,  &BGThreads::CtrlWork, (void *)c));
}


BGThreads::~BGThreads() { //need to wait for the threads.
//    while (elems_ > 0) {
//        printf(DBG "Destory BG: %d elems left.\n", elems_);
//        cv_.SignalAll();
//        sleep(1);
//    }
    printf(DBG "Destory BG: %d elems left.\n", elems_);
    {
        MutexLock l(&mu_);
        shouldstop_ = true;
    }
    ctrl_cv_.Signal();
    PthreadCall("wait for ctrl thread", pthread_join(ctrl_, NULL));
    delete[] slots_;
}


int BGThreads::AddSchedule(function_t func, void *arg) {
    int i;
    wraparg *w;
    if (shouldstop_) {
        return -1;
    }
    printf(DBG "Add is called: elems_: %d size: %d\n", elems_, size_);
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
        slots_[i].num_ = elems_;
    }
    PthreadCall("start thread", pthread_create(&slots_[i].thread_, NULL,  &BGThreads::Wrapper, (void *)w));
    return slots_[i].num_;
}
void BGThreads::StartAll() {
    MutexLock l(&mutex_);
    printf(DBG "StartAll.\n");
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
                new_[new_elems_].num_ = old_[i].num_;
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
    int num = bg->slots_[w->islot].num_;
    int ret_tmp;
    PthreadCall("detach self", pthread_detach(pthread_self()));
    printf("thread %d is created.\n", num);
    bg->mutex_.Lock();
    printf("thread %d is locked & wait.\n", num);
    bg->cv_.Wait();
    bg->mutex_.Unlock();


    printf("thread %d is signaled.\n", num);
    ret_tmp = w->func(w->arg);
    {
        MutexLock l(&(bg->mu_));
        bg->slots_[w->islot].ret_ = ret_tmp;
        bg->slots_[w->islot].state_ = Finished;
        bg->elems_--;
    }


    printf("thread %d stops. Ret: %d.\n", num, bg->slots_[w->islot].ret_);

    delete w;

    return NULL;
}

void* BGThreads::CtrlWork(void *arg) {
    CtrlArg *t = (CtrlArg *)arg;
    BGThreads *bg = t->BGptr;
    printf(DBG "ctrl thread is create!\n");
    while (1) {
        while (bg->elems_ <= 0) {
            if (bg->shouldstop_) {
                goto OUT;
            }
            printf(DBG "ctrl thread is sleeping.\n");
            bg->ctrl_cv_.Wait();
        }
        bg->mutex_.Lock();
        bg->cv_.SignalAll();
        bg->mutex_.Unlock();
        sleep(1);
        printf(DBG "ctrl thread ITR elems %d\n", bg->elems_);
    }
OUT:
    delete t;
    return NULL;
}
