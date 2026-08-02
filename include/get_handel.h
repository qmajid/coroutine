#include <coroutine>
#include <iostream>

struct Task {
    struct promise_type {
        Task get_return_object() {
            auto h = std::coroutine_handle<promise_type>::from_promise(*this);
            return Task{h};
        }

        std::suspend_always initial_suspend() { return {}; }

        std::suspend_always final_suspend() noexcept { return {}; }

        void return_void() {}

        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}

    ~Task() {
        if (handle) {
            handle.destroy();
        }
    }

    void resume() {
        if (handle && !handle.done()) {
            handle.resume();
        }
    }
};
struct TaskPeromisType {
    struct promise_type {
        TaskPeromisType get_return_object() {
            auto h = std::coroutine_handle<promise_type>::from_promise(*this);

            return TaskPeromisType{h};
        }

        std::suspend_always initial_suspend() { return {}; }

        std::suspend_always final_suspend() noexcept { return {}; }

        void return_void() {}

        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> h;

    TaskPeromisType(std::coroutine_handle<promise_type> h) : h(h) {}

    ~TaskPeromisType() {
        if (h) {
            h.destroy();
        }
    }

    std::coroutine_handle<> get_void_handle() const { return h; }
};