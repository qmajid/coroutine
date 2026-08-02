#include <coroutine>
#include <iostream>
#include <optional>

struct Generator {
    struct promise_type {
        int current_value;

        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }

        std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(int value) {
            current_value = value;
            return {};
        }

        void return_void() {}

        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    explicit Generator(std::coroutine_handle<promise_type> h) : handle(h) {}

    ~Generator() {
        if (handle) {
            handle.destroy();
        }
    }

    bool next() {
        if (!handle || handle.done()) {
            return false;
        }

        handle.resume();

        return !handle.done();
    }

    int value() const { return handle.promise().current_value; }
};
