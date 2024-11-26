//
// Created by idoscharf on 12/06/24.
//

#include "Scheduler.h"
#include <algorithm>
#include <stdexcept>
#include <csignal>

#define SECOND 1000000

Scheduler::Scheduler(unsigned int max_threads, size_t stack_size, int quantum_usecs) :
        _threads(new std::unique_ptr<Thread>[max_threads]), _current_tid(0), _ready_queue(),
        _blocked_tids(), _available_tids(), _timer(), _mask(), _max_threads(max_threads),
        _stack_size(stack_size), _total_runtime(1) {
    if (quantum_usecs <= 0) {
        throw UThreadError(LIBRARY_ERROR,
                           "Quantum length must be a positive integer.");
    }

    try {
        _threads[0].reset(new Thread());
    }
    catch (const std::bad_alloc &e) {
        throw UThreadError(e, SYSTEM_ERROR,
                           "Failed to allocate memory for main thread.");
    }

    _timer.it_value.tv_usec = quantum_usecs % SECOND;
    _timer.it_value.tv_sec = quantum_usecs / SECOND;
    _timer.it_interval.tv_usec = quantum_usecs % SECOND;
    _timer.it_interval.tv_sec = quantum_usecs / SECOND;
    restart_timer();

    sigemptyset(&_mask);
    sigaddset(&_mask, SIGVTALRM);

    for (unsigned int i = 1; i < max_threads; i++) {
        _available_tids.push((int) i);
    }
}

void Scheduler::schedule() {
    _total_runtime++;

    int next_tid = _current_tid;

    if (!_ready_queue.empty()) {
        next_tid = _ready_queue.front();
        _ready_queue.pop_front();
    }

    _threads[next_tid]->run();

    for (auto it = _blocked_tids.begin(); it != _blocked_tids.end(); ) {
        int tid = *it;
        if (_threads[tid]->is_sleeping()) {
            _threads[tid]->decrement_sleep();
        }
        else if (!_threads[tid]->is_blocked()) {
            it = _blocked_tids.erase(it);
            _ready_queue.push_back(tid);
        }
        else {
            it++;
        }
    }

    if (next_tid == _current_tid) {
        return;
    }

    if (_threads[_current_tid] && _threads[_current_tid]->is_running()) {
        _ready_queue.push_back(_current_tid);
    }

    int res = 0;
    if (_threads[_current_tid]) {
        _threads[_current_tid]->yield();
        res = sigsetjmp(_threads[_current_tid]->context, 1);
    }
    if (res == 0) {
        _current_tid = next_tid;
        siglongjmp(_threads[_current_tid]->context, 1);
    }
}

int Scheduler::create_thread(thread_entry_point entry_point) {
    if (entry_point == nullptr) {
        throw UThreadError(LIBRARY_ERROR,
                           "Expected not-null entry point.");
    }

    if (_available_tids.empty()) {
        throw UThreadError(LIBRARY_ERROR,
                           "Thread library is full. Cannot create a new thread.");
    }

    mask_signal();

    int tid = _available_tids.top();
    try {
        _threads[tid].reset(new Thread(_stack_size, entry_point));
    }
    catch (const std::bad_alloc &e) {
        throw UThreadError(e, SYSTEM_ERROR,
                           "Failed to allocate memory for a new thread.");
    }
    _available_tids.pop();
    _ready_queue.push_back(tid);

    unmask_signal();

    return tid;
}

int Scheduler::remove_thread(int tid) {
    assert_tid_valid(tid);

    mask_signal();

    if (tid == 0) {
        return RET_END;
    }

    if (!_threads[tid]) {
        unmask_signal();
        throw UThreadError(LIBRARY_ERROR,
                           "Cannot terminate thread. No thread has tid " +
                           std::to_string(tid));
    }

    _available_tids.push(tid);

    if (_threads[tid]->is_blocked() || _threads[tid]->is_sleeping()) {
        _blocked_tids.erase(tid);
    }
    else if (tid != _current_tid) {
        auto ready_it = std::find(_ready_queue.begin(), _ready_queue.end(), tid);
        if (ready_it != _ready_queue.end()) {
            _ready_queue.erase(ready_it);
        }
    }

    _threads[tid].reset();

    if (tid == _current_tid) {
        restart_timer();
        unmask_signal();
        schedule();
    }

    unmask_signal();

    return RET_SUCCESS;
}

void Scheduler::restart_timer() {
    mask_signal();

    if (setitimer(ITIMER_VIRTUAL, &_timer, nullptr)) {
        throw UThreadError(SYSTEM_ERROR, "Failed to restart timer.");
    }

    unmask_signal();
}

int Scheduler::get_current_tid() const {
    mask_signal();

    int retval = _current_tid;

    unmask_signal();

    return retval;
}

int Scheduler::get_total_runtime() const {
    mask_signal();

    int retval = _total_runtime;

    unmask_signal();

    return retval;
}

int Scheduler::get_thread_runtime(int tid) const {
    assert_tid_valid(tid);

    mask_signal();

    if (!_threads[tid]) {
        unmask_signal();
        throw UThreadError(LIBRARY_ERROR,
                           "Cannot get quanta. No thread exists with tid " +
                           std::to_string(tid));
    }

    int retval = _threads[tid]->get_runtime();

    unmask_signal();

    return retval;
}

void Scheduler::mask_signal() const {
    if (sigprocmask(SIG_BLOCK, &_mask, nullptr)) {
        throw UThreadError(SYSTEM_ERROR, "Failed to _mask signal.");
    }
}

void Scheduler::unmask_signal() const {
    if (sigprocmask(SIG_UNBLOCK, &_mask, nullptr)) {
        throw UThreadError(SYSTEM_ERROR, "Failed to unmask signal.");
    }
}

Scheduler::~Scheduler() {
    delete[] _threads;
}

int Scheduler::block_thread(int tid) {
    assert_tid_valid(tid);

    if (tid == 0) {
        throw UThreadError(LIBRARY_ERROR, "Cannot block main thread.");
    }

    mask_signal();

    if(!_threads[tid]) {
        unmask_signal();
        throw UThreadError(LIBRARY_ERROR,
                           "Cannot block. No thread exists with tid " +
                           std::to_string(tid));
    }

    if (_threads[tid]->is_running()) {
        _threads[tid]->block();
        _blocked_tids.insert(tid);
        restart_timer();
        unmask_signal();
        schedule();
    }
    else {
        _threads[tid]->block();
        _blocked_tids.insert(tid);
        auto ready_pos = std::find(_ready_queue.begin(), _ready_queue.end(), tid);
        if (ready_pos != _ready_queue.end()) {
            _ready_queue.erase(ready_pos);
        }
    }

    unmask_signal();

    return RET_SUCCESS;
}

int Scheduler::resume_thread(int tid) {
    assert_tid_valid(tid);

    mask_signal();

    if (!_threads[tid]) {
        unmask_signal();
        throw UThreadError(LIBRARY_ERROR,
                           "Cannot resume. No thread exists with tid " +
                           std::to_string(tid));
    }

    if (_threads[tid]->is_blocked()) {
        _threads[tid]->resume();
        if (!_threads[tid]->is_sleeping()) {
            _blocked_tids.erase(tid);
            _ready_queue.push_back(tid);
        }
    }

    unmask_signal();

    return RET_SUCCESS;
}

int Scheduler::sleep_current_thread(int sleep_quanta) {
    if (sleep_quanta <= 0) {
        throw UThreadError(LIBRARY_ERROR,
                           "Sleep quanta must be a positive integer.");
    }

    mask_signal();

    if (_current_tid == 0) {
        unmask_signal();
        throw UThreadError(LIBRARY_ERROR, "Main thread cannot sleep.");
    }

    _threads[_current_tid]->sleep(sleep_quanta);
    _blocked_tids.insert(_current_tid);

    restart_timer();
    unmask_signal();
    schedule();

    return RET_SUCCESS;
}

void Scheduler::assert_tid_valid(int tid) const {
    if (tid < 0 || tid >= (int) _max_threads) {
        throw UThreadError(LIBRARY_ERROR, "Invalid tid.");
    }
}
