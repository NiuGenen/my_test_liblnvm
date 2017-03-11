#ifndef BGTHREAD_H
#define BGTHREAD_H

#include <pthread.h>
#define MyBarrier() __asm__ __volatile__("" ::: "memory")

class CondVar;

class Mutex {
 public:
  Mutex();
  ~Mutex();

  void Lock();
  void Unlock();
  void AssertHeld() { }

 private:
  friend class CondVar;
  pthread_mutex_t mu_;

  // No copying
  Mutex(const Mutex&);
  void operator=(const Mutex&);
};

class CondVar {
 public:
  explicit CondVar(Mutex* mu);
  ~CondVar();
  void Wait();
//  void Wait_time
  void Signal();
  void SignalAll();
 private:
  pthread_cond_t cv_;
  Mutex* mu_;
};

class MutexLock {
 public:
  explicit MutexLock(Mutex *mu)
      : mu_(mu)  {
    this->mu_->Lock();
  }
  ~MutexLock(){
       this->mu_->Unlock(); 
  }

 private:
  Mutex *const mu_;
  // No copying allowed
  MutexLock(const MutexLock&);
  void operator=(const MutexLock&);
};



class BGThreads{
public:
    typedef pthread_t ThreadType;
    typedef void (*initializer_t)();
    typedef int (*function_t)(void*);
    struct wraparg{
        function_t func;
        void* arg;
        int islot;
        BGThreads *BGptr;
        wraparg(int i, function_t f, void *a, BGThreads *bg) : islot(i), func(f), arg(a), BGptr(bg){
        }
    };
    BGThreads(): size_(0), elems_(0), slots_(0), bg_cv_(&mutex_),cv_(&mutex_){
        Resize(4);
    }
    ~BGThreads();

    void AddSchedule(function_t func, void* arg);
    void StartAll();
private:
    enum ThreadState{
        Empty = 0x01,
        Pending = 0x02,
        Finished = 0x03
    };

    struct ThreadItem{
        ThreadType thread_;
        int ret_;
        int num_;
        ThreadState state_;
        wraparg *args_; //is not deleted by destructor. 
        ThreadItem() 
        : state_(Empty){ }
        ~ThreadItem(){ }
    };

    void Resize(int size);
    static void* Wrapper(void* arg);

    Mutex mu_;//Lock for BGThread's private meta
    

    Mutex mutex_;//Lock for Threads' concurrency.
    CondVar cv_;
    CondVar bg_cv_;//CondVar for BGThread to wait for all threads stop.

    ThreadItem *slots_;
    int size_;
    int elems_;
    int err_;
};

#endif
