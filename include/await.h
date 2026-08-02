#ifndef AWAITE_H
#define AWAITE_H

#pragma once

#include <coroutine>
#include <iostream>

struct MyAwaitable {
    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        std::cout << "Suspended\n";
        h.resume();
    }

    int await_resume() { return 42; }
};

struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }

        std::suspend_never initial_suspend() { return {}; }

        std::suspend_never final_suspend() noexcept { return {}; }

        void return_void() {}

        void unhandled_exception() {}
    };
};
class AWait {
  public:
    AWait();
    ~AWait();
    Task test();

  private:
};

#endif