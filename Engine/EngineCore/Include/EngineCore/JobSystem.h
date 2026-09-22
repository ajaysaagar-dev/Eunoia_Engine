#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <memory>

// ============================================================================
// EngineCore::JobSystem — Thread pool for asynchronous background tasks
// ============================================================================

namespace EngineCore {

class JobSystem {
public:
    static JobSystem& Get() {
        static JobSystem instance;
        return instance;
    }

    explicit JobSystem(size_t numThreads = 0) {
        if (numThreads == 0) {
            numThreads = std::max(1u, std::thread::hardware_concurrency());
            if (numThreads > 1) numThreads -= 1; // Leave one core for main/render thread
        }
        m_workers.reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i) {
            m_workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_queueMutex);
                        m_condition.wait(lock, [this]() {
                            return m_stop.load() || !m_tasks.empty();
                        });
                        if (m_stop.load() && m_tasks.empty()) {
                            return;
                        }
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                        m_activeJobs++;
                    }

                    try {
                        task();
                    } catch (...) {
                        // Suppress job exceptions from terminating worker threads
                    }

                    {
                        std::unique_lock<std::mutex> lock(m_queueMutex);
                        m_activeJobs--;
                    }
                    m_waitCondition.notify_all();
                }
            });
        }
    }

    ~JobSystem() {
        Stop();
    }

    // Submit a task with return value via std::future
    template<typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (m_stop.load()) {
                throw std::runtime_error("JobSystem is stopped");
            }
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_condition.notify_one();
        return res;
    }

    // Fire-and-forget submission
    void Execute(std::function<void()> job) {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (m_stop.load()) return;
            m_tasks.push(std::move(job));
        }
        m_condition.notify_one();
    }

    // Wait until all queued jobs finish
    void WaitAll() {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_waitCondition.wait(lock, [this]() {
            return m_tasks.empty() && m_activeJobs == 0;
        });
    }

    void Stop() {
        m_stop.store(true);
        m_condition.notify_all();
        m_waitCondition.notify_all();
        for (std::thread& worker : m_workers) {
            if (worker.joinable()) {
                worker.detach();
            }
        }
        m_workers.clear();
    }

    size_t WorkerCount() const { return m_workers.size(); }

private:
    std::vector<std::thread>          m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex                        m_queueMutex;
    std::condition_variable           m_condition;
    std::condition_variable           m_waitCondition;
    std::atomic<bool>                 m_stop{false};
    size_t                            m_activeJobs{0};
};

} // namespace EngineCore
