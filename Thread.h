//
// Created by idoscharf on 12/06/24.
//

#ifndef OS_EX2_2_THREAD_H
#define OS_EX2_2_THREAD_H


#include <cstdlib>
#include <csetjmp>
#include <memory>

typedef void (*thread_entry_point)(void);

class Thread {
public:
//    Constructor for main thread (does not explicitly initialize stack and PC)
    Thread();

//    Constructor for a new thread
    Thread(size_t stack_size, thread_entry_point entry_point);

    void run();
    void block();
    void resume();
    void sleep(int quanta);
    void yield();

    void decrement_sleep();

    bool is_running() const;
    bool is_blocked() const;
    bool is_sleeping() const;
    int get_runtime() const;

    sigjmp_buf context;

private:
    bool _is_running;
    bool _is_blocked;
    int _q_sleep;
    int _q_run;

    std::unique_ptr<char> stack;
};


#endif //OS_EX2_2_THREAD_H
