//
// Created by idoscharf on 12/06/24.
//
#include "uthreads.h"
#include "Scheduler.h"
#include <csignal>
#include <iostream>

#define LIBERR_RETVAL (-1)

std::unique_ptr<Scheduler> scheduler;
struct sigaction sa;

void timer_handler(int signum) {
    scheduler->schedule();
}

void clean_exit(int exit_status) {
    scheduler.reset();
    exit(exit_status);
}

int handle_error(const UThreadError& error) {
    switch (error.type) {
        case LIBRARY_ERROR:
            std::cerr << "thread library error: " << error.message << std::endl;
            return LIBERR_RETVAL;
        case SYSTEM_ERROR:
            std::cerr << "system error: " << error.message << std::endl;
            clean_exit(EXIT_FAILURE);
            return EXIT_FAILURE;
        default:
            return EXIT_FAILURE;
    }
}

int uthread_init(int quantum_usecs) {
    try {
        scheduler.reset(new Scheduler(MAX_THREAD_NUM, STACK_SIZE, quantum_usecs));
    }
    catch (const UThreadError &error) {
        return handle_error(error);
    }
    catch (const std::bad_alloc &e) {
        return handle_error(UThreadError(SYSTEM_ERROR,
                                         "Failed to create scheduler."));
    }

    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, nullptr)) {
        return handle_error(UThreadError(SYSTEM_ERROR,
                                         "Failed to install signal action."));
    }

    return RET_SUCCESS;
}

int uthread_spawn(thread_entry_point entry_point) {
    try {
        return scheduler->create_thread(entry_point);
    }
    catch (const UThreadError &error) {
        return handle_error(error);
    }
}

int uthread_terminate(int tid) {
    int retval;

    try {
        retval = scheduler->remove_thread(tid);
    }
    catch (const UThreadError &error) {
        return handle_error(error);
    }

    if (retval == RET_END) {
        clean_exit(EXIT_SUCCESS);
    }

    return retval;
}

int uthread_get_tid() {
    try {
        return scheduler->get_current_tid();
    }
    catch (const UThreadError &error) {
        return handle_error(error);
    }
}

int uthread_get_total_quantums() {
    try {
        return scheduler->get_total_runtime();
    }
    catch (const UThreadError &error) {
        return handle_error(error);
    }
}

int uthread_get_quantums(int tid) {
    try {
        return scheduler->get_thread_runtime(tid);
    }
    catch (const UThreadError &error) {
        return handle_error(error);
    }
}

int uthread_block(int tid) {
    try {
        return scheduler->block_thread(tid);
    }
    catch (const UThreadError &error) {
        return handle_error(error);
    }
}

int uthread_resume(int tid) {
    try {
        return scheduler->resume_thread(tid);
    }
    catch (const UThreadError &error) {
        return handle_error(error);
    }
}

int uthread_sleep(int num_quantums) {
    try {
        return scheduler->sleep_current_thread(num_quantums);
    }
    catch (const UThreadError &error) {
        return handle_error(error);
    }
}

