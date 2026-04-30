#pragma once

#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class CoroutineTaskQueue {
public:
  class Task {
  public:
    struct promise_type {
      Task get_return_object() noexcept {
        return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
      }
      std::suspend_always initial_suspend() const noexcept { return {}; }
      std::suspend_always final_suspend() const noexcept { return {}; }
      void return_void() const noexcept {}
      void unhandled_exception() const { std::terminate(); }
    };

    Task() = default;
    Task(Task &&other) noexcept : handle_(std::exchange(other.handle_, {})) {}
    Task &operator=(Task &&other) noexcept {
      if (this == &other) {
        return *this;
      }
      reset();
      handle_ = std::exchange(other.handle_, {});
      return *this;
    }
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    ~Task() { reset(); }

  private:
    explicit Task(std::coroutine_handle<promise_type> handle)
        : handle_(handle) {}
    friend class CoroutineTaskQueue;

    std::coroutine_handle<> release() noexcept {
      return std::exchange(handle_, {});
    }
    void reset() {
      if (handle_) {
        handle_.destroy();
        handle_ = {};
      }
    }

    std::coroutine_handle<promise_type> handle_{};
  };

  class RequeueAwaitable {
  public:
    explicit RequeueAwaitable(CoroutineTaskQueue &queue) : queue_(queue) {}
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) const {
      queue_.enqueue(handle);
    }
    void await_resume() const noexcept {}

  private:
    CoroutineTaskQueue &queue_;
  };

  explicit CoroutineTaskQueue(
      std::size_t workerCount = std::thread::hardware_concurrency());
  ~CoroutineTaskQueue();

  CoroutineTaskQueue(const CoroutineTaskQueue &) = delete;
  CoroutineTaskQueue &operator=(const CoroutineTaskQueue &) = delete;

  // Enqueue a fire-and-forget coroutine task.
  void enqueue(Task task);

  // Enqueue any callable for concurrent execution and receive its result.
  template <typename Func, typename... Args>
  auto submit(Func &&func, Args &&...args)
      -> std::future<
          std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>> {
    using Result =
        std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

    auto boundTask =
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    auto promise = std::make_shared<std::promise<Result>>();
    std::future<Result> future = promise->get_future();

    enqueue([promise, boundTask = std::move(boundTask)]() mutable {
      try {
        if constexpr (std::is_void_v<Result>) {
          boundTask();
          promise->set_value();
        } else {
          promise->set_value(boundTask());
        }
      } catch (...) {
        promise->set_exception(std::current_exception());
      }
    });

    return future;
  }

  // Enqueue a regular callback.
  void enqueue(std::function<void()> callback);

  // Coroutine can use `co_await queue.requeue();` to yield back to queue.
  RequeueAwaitable requeue() { return RequeueAwaitable(*this); }

  void shutdown();

private:
  struct Job {
    std::optional<std::function<void()>> callback;
    std::coroutine_handle<> coroutine;
  };

  void enqueue(std::coroutine_handle<> coroutine);
  void workerLoop(const std::stop_token &stopToken);
  bool popJob(Job &job);

  std::mutex mutex_;
  std::condition_variable condition_;
  std::queue<Job> jobs_;
  std::vector<std::jthread> workers_;
  bool shuttingDown_{false};
};
