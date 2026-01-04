/**
 * @brief Lightweight Thread Pool using C++17 std::async
 *
 * A simple thread pool wrapper that limits concurrent std::async tasks.
 * Unlike traditional thread pools with fixed worker threads, this creates
 * futures on demand and limits the number of concurrent executions.
 *
 * Features:
 * - Dynamic task submission
 * - Limit on concurrent tasks (not total threads)
 * - Wait for all tasks to complete
 * - Get results from futures
 *
 * Usage:
 *   ThreadPool pool(4);  // max 4 concurrent tasks
 *   auto future1 = pool.submit([]() { return 42; });
 *   auto future2 = pool.submit([]() { return "hello"; });
 *   pool.wait();  // Wait for all tasks
 *   int result = future1.get();
 *
 * === Updated: 2026-01-05 ===
 */

#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <atomic>
#include <thread>

namespace parallel {

/**
 * @brief Lightweight thread pool using std::async with concurrency limit
 *
 * This implementation uses a semaphore to limit concurrent executions.
 * Tasks are submitted and immediately launched via std::async, but the
 * semaphore ensures only N tasks run concurrently.
 */
class ThreadPool {
public:
    using Task = std::function<void()>;

    /**
     * @brief Construct a thread pool with max concurrency
     * @param max_concurrent Maximum number of concurrent tasks
     */
    explicit ThreadPool(size_t max_concurrent = std::thread::hardware_concurrency())
        : max_concurrent_(max_concurrent)
        , stopped_(false)
    {
        if (max_concurrent_ == 0) {
            max_concurrent_ = 1;
        }
    }

    ~ThreadPool() {
        stop();
    }

    /**
     * @brief Submit a task to the pool
     * @tparam F Function type
     * @tparam Args Argument types
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return std::future for the result
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));

        // Create a packaged task
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        // Submit as void task
        submit_void([task]() {
            try {
                (*task)();
            } catch (...) {
                // Exception is captured in the future
            }
        });

        return result;
    }

    /**
     * @brief Submit a void task
     */
    void submit_void(Task task) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Enqueue the task
        pending_tasks_.push(std::move(task));
        cv_.notify_one();

        // Try to launch a worker if needed
        try_spawn_worker();
    }

    /**
     * @brief Wait for all submitted tasks to complete
     */
    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() {
            return stopped_ || (pending_tasks_.empty() && active_workers_ == 0);
        });
    }

    /**
     * @brief Get number of active workers
     */
    size_t active_count() const {
        return active_workers_.load();
    }

    /**
     * @brief Get number of pending tasks
     */
    size_t pending_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_tasks_.size();
    }

    /**
     * @brief Get max concurrency
     */
    size_t max_concurrency() const {
        return max_concurrent_;
    }

    /**
     * @brief Stop accepting new tasks and wait for active tasks
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

private:
    void try_spawn_worker() {
        // Only spawn if we haven't reached max concurrency
        while (active_workers_ < max_concurrent_ && !pending_tasks_.empty() && !stopped_) {
            Task task = std::move(pending_tasks_.front());
            pending_tasks_.pop();
            active_workers_++;

            // Spawn a worker thread for this task
            std::thread worker([this, task = std::move(task)]() mutable {
                // Execute the task
                try {
                    if (task) {
                        task();
                    }
                } catch (...) {
                    // Swallow exceptions
                }

                // Worker done
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    active_workers_--;
                }
                cv_.notify_all();

                // Try to process more tasks
                try_spawn_worker_impl();
            });

            worker.detach();
        }
    }

    // Separate function to be called from worker thread context
    void try_spawn_worker_impl() {
        std::lock_guard<std::mutex> lock(mutex_);
        try_spawn_worker();
    }

    size_t max_concurrent_;
    std::atomic<bool> stopped_;
    std::atomic<size_t> active_workers_{0};
    std::queue<Task> pending_tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace parallel

#endif // THREAD_POOL_HPP
