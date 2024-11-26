//
// Created by idoscharf on 12/06/24.
//

#ifndef OS_EX2_2_SCHEDULER_H
#define OS_EX2_2_SCHEDULER_H


#include "Thread.h"
#include "UThreadError.h"
#include <list>
#include <unordered_set>
#include <queue>
#include <memory>
#include <sys/time.h>
#include <string>

#define RET_SUCCESS 0
#define RET_END (-3)

class Scheduler {
public:
    Scheduler(unsigned int max_threads, size_t stack_size, int quantum_usecs);
    ~Scheduler();

    void schedule();

    int create_thread(thread_entry_point entry_point);
    int remove_thread(int tid);
    int block_thread(int tid);
    int resume_thread(int tid);
    int sleep_current_thread(int sleep_quanta);
    int get_current_tid() const;
    int get_total_runtime() const;
    int get_thread_runtime(int tid) const;


private:
    std::unique_ptr<Thread> *_threads;
    int _current_tid;
    std::list<int> _ready_queue;
    std::unordered_set<int> _blocked_tids;
    std::priority_queue<int, std::vector<int>, std::greater<int>> _available_tids;
    struct itimerval _timer;
    sigset_t _mask;

    const size_t _max_threads;
    const size_t _stack_size;
    int _total_runtime;

    void restart_timer();
    void mask_signal() const;
    void unmask_signal() const;
    void assert_tid_valid(int tid) const;
};


#endif //OS_EX2_2_SCHEDULER_H
