//
// Created by idoscharf on 12/06/24.
//

#include "Thread.h"
#include <csignal>

//region setup
#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
            : "=g" (ret)
            : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}


#endif
//endregion setup

Thread::Thread() : context(), _is_running(true), _is_blocked(false), _q_sleep(0),
    _q_run(1), stack(nullptr) {
    sigsetjmp(context, 1);
}

Thread::Thread(size_t stack_size, thread_entry_point entry_point) : context(),
    _is_running(false), _is_blocked(false), _q_sleep(0), _q_run(0),
    stack(new char[stack_size]) {
    auto sp = (address_t) stack.get() + stack_size - sizeof(address_t);
    auto pc = (address_t) entry_point;
    sigsetjmp(context, 1);
    (context->__jmpbuf)[JB_SP] = translate_address(sp);
    (context->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&context->__saved_mask);
}

void Thread::run() {
    _is_running = true;
    _q_run++;
}

void Thread::block() {
    _is_blocked = true;
    _is_running = false;
}

void Thread::resume() {
    _is_blocked = false;
}

void Thread::sleep(int quanta) {
    _q_sleep = quanta;
    _is_running = false;
}

void Thread::decrement_sleep() {
    _q_sleep--;
}

bool Thread::is_running() const {
    return _is_running;
}

bool Thread::is_blocked() const {
    return _is_blocked;
}

bool Thread::is_sleeping() const {
    return _q_sleep > 0;
}

int Thread::get_runtime() const {
    return _q_run;
}

void Thread::yield() {
    _is_running = false;
}
