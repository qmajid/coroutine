#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "coroutine_task_queue.h"

inline CoroutineTaskQueue::Task addCoroutineTask(CoroutineTaskQueue& queue,
                                                 std::function<void()> callback) {
    callback();
    // Yield back to queue so other tasks can run.
    co_await queue.requeue();
}

inline CoroutineTaskQueue::Task addCoroutineTask(
    CoroutineTaskQueue& queue, std::function<void(const std::vector<uint8_t>&)> callback,
    std::vector<uint8_t> data) {
    callback(data);
    // Yield back to queue so other tasks can run.
    co_await queue.requeue();
}

template <class T>
inline CoroutineTaskQueue::Task addCoroutineTask(CoroutineTaskQueue& queue,
                                                 std::function<void(T)> callback, T data) {
    callback(data);
    // Yield back to queue so other tasks can run.
    co_await queue.requeue();
}

template <typename ResultType>
inline CoroutineTaskQueue::Task addCoroutineTaskWithResult(
    CoroutineTaskQueue& queue, std::function<ResultType()> func,
    std::promise<ResultType>& resultPromise) {
    queue.submit([func = std::move(func), &resultPromise]() mutable {
        try {
            resultPromise.set_value(func());
        } catch (...) {
            resultPromise.set_exception(std::current_exception());
        }
    });
    co_return;
}
//-------------------------------------------------
namespace {
    std::mutex coutMutex;

    void safePrint(const std::string& message) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << message << '\n';
    }

    // CoroutineTaskQueue::Task processBatches(CoroutineTaskQueue &queue, int
    // taskId,
    //                                         int batches, int workMsPerBatch) {
    //   for (int i = 1; i <= batches; ++i) {
    //     safePrint("[coroutine " + std::to_string(taskId) + "] batch " +
    //               std::to_string(i) + " started on thread " +
    //               std::to_string(
    //                   std::hash<std::thread::id>{}(std::this_thread::get_id())));

    //     // Simulate CPU/IO work.
    //     std::this_thread::sleep_for(std::chrono::milliseconds(workMsPerBatch));

    //     // Yield back to queue so other tasks can run.
    //     co_await queue.requeue();
    //   }

    //   safePrint("[coroutine " + std::to_string(taskId) + "] finished");
    // }

    // CoroutineTaskQueue::Task taskWithDataCallback(
    //     CoroutineTaskQueue &queue, const std::vector<uint8_t> &data,
    //     const std::function<void(const std::vector<uint8_t> &)> &onData) {
    //   onData(data);
    //   co_await queue.requeue();
    // }

    // CoroutineTaskQueue::Task
    // taskWithNoArgCallback(CoroutineTaskQueue &queue,
    //                       const std::function<void()> &onRun) {
    //   onRun();
    //   co_await queue.requeue();
    // }

}  // namespace

int main() {
    CoroutineTaskQueue queue(4);

    // 1) addCoroutineTask with no-arg callback.
    queue.enqueue(addCoroutineTask(queue, [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        safePrint("[addCoroutineTask<void()>] hello from callback");
    }));

    // 2) addCoroutineTask with vector<uint8_t> callback.
    std::vector<uint8_t> packet{10, 20, 30, 40};
    queue.enqueue(addCoroutineTask(
        queue,
        [](const std::vector<uint8_t>& data) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            safePrint("[addCoroutineTask<vector<uint8_t>>] size=" + std::to_string(data.size()) +
                      ", first=" + std::to_string(static_cast<int>(data.front())));
        },
        packet));

    // 3) addCoroutineTask<T> template overload with a simple int callback.
    queue.enqueue(addCoroutineTask<int>(
        queue,
        [](int value) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            safePrint("[addCoroutineTask<int>] value=" + std::to_string(value));
        },
        123));

    // 4) addCoroutineTaskWithResult with a simple function.
    std::promise<int> resultPromise;
    std::future<int> resultFuture = resultPromise.get_future();
    queue.enqueue(addCoroutineTaskWithResult<int>(queue, []() { return 7 * 6; }, resultPromise));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    safePrint("[addCoroutineTaskWithResult] result=" + std::to_string(resultFuture.get()));

    // Wait a bit so coroutine jobs can complete before shutdown.
    std::this_thread::sleep_for(std::chrono::seconds(2));
    queue.shutdown();
    return 0;
}