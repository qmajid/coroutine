#include "coroutine_task_queue.h"

#include <stdexcept>

CoroutineTaskQueue::CoroutineTaskQueue(std::size_t workerCount) {
    if (workerCount == 0) {
        workerCount = 1;
    }

    workers_.reserve(workerCount);
    for (std::size_t i = 0; i < workerCount; ++i) {
        workers_.emplace_back([this](const std::stop_token& stopToken) { workerLoop(stopToken); });
    }
}

CoroutineTaskQueue::~CoroutineTaskQueue() { shutdown(); }

void CoroutineTaskQueue::enqueue(Task task) {
    std::coroutine_handle<> handle = task.release();
    if (!handle) {
        return;
    }
    enqueue(handle);
}

void CoroutineTaskQueue::enqueue(std::function<void()> callback) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shuttingDown_) {
            throw std::runtime_error("CoroutineTaskQueue is shutting down");
        }
        jobs_.push(Job{.callback = std::move(callback), .coroutine = {}});
    }
    condition_.notify_one();
}

void CoroutineTaskQueue::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shuttingDown_) {
            return;
        }
        shuttingDown_ = true;
    }

    condition_.notify_all();
    for (auto& worker : workers_) {
        worker.request_stop();
    }
    workers_.clear();

    std::queue<Job> remaining;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        remaining.swap(jobs_);
    }
    while (!remaining.empty()) {
        Job& job = remaining.front();
        if (job.coroutine) {
            job.coroutine.destroy();
        }
        remaining.pop();
    }
}

void CoroutineTaskQueue::enqueue(std::coroutine_handle<> coroutine) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shuttingDown_) {
            coroutine.destroy();
            return;
        }
        jobs_.push(Job{.callback = std::nullopt, .coroutine = coroutine});
    }
    condition_.notify_one();
}

void CoroutineTaskQueue::workerLoop(const std::stop_token& stopToken) {
    while (!stopToken.stop_requested()) {
        Job job;
        if (!popJob(job)) {
            break;
        }

        try {
            if (job.callback.has_value()) {
                job.callback.value()();
                continue;
            }

            if (!job.coroutine) {
                continue;
            }

            job.coroutine.resume();
            if (job.coroutine.done()) {
                job.coroutine.destroy();
            }
        } catch (...) {
            // Keep worker thread alive when user task throws.
        }
    }
}

bool CoroutineTaskQueue::popJob(Job& job) {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this]() { return shuttingDown_ || !jobs_.empty(); });
    if (jobs_.empty()) {
        return false;
    }

    job = std::move(jobs_.front());
    jobs_.pop();
    return true;
}
