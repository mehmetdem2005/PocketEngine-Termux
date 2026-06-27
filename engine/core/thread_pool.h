// ============================================================
//  pocket/engine/core/thread_pool.h
//  Fixed-size thread pool with work-stealing queues
// ============================================================
#pragma once

#include "core/types.h"
#include "core/log.h"

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <vector>

namespace pk {

class ThreadPool {
public:
    explicit ThreadPool(usize threadCount = 0)
        : m_stop(false) {
        if (threadCount == 0) {
            threadCount = std::thread::hardware_concurrency();
            if (threadCount == 0) threadCount = 4;
            // Cap on phone - 8 max
            threadCount = std::min<usize>(threadCount, 8);
        }
        m_workers.reserve(threadCount);
        for (usize i = 0; i < threadCount; ++i) {
            m_workers.emplace_back([this, i] { workerMain(i); });
        }
        PK_LOG_INFO("Thread", "ThreadPool started with %zu workers", threadCount);
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto& w : m_workers) {
            if (w.joinable()) w.join();
        }
    }

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Submit a callable returning a future
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<R> fut = task->get_future();

        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (m_stop) {
                throw std::runtime_error("ThreadPool: submit on stopped pool");
            }
            m_tasks.emplace_back([task]() { (*task)(); });
        }
        m_cv.notify_one();
        return fut;
    }

    // Parallel for: split [0, n) into chunks, run on all workers
    template <typename Body>
    void parallelFor(usize n, usize grainSize, Body&& body) {
        if (n == 0) return;
        std::atomic<usize> next{0};
        Vec<std::future<void>> futs;
        futs.reserve(m_workers.size());

        auto worker = [&]() {
            while (true) {
                usize start = next.fetch_add(grainSize, std::memory_order_relaxed);
                if (start >= n) break;
                usize end = std::min<usize>(start + grainSize, n);
                for (usize i = start; i < end; ++i) body(i);
            }
        };

        for (usize i = 0; i + 1 < m_workers.size(); ++i) {
            futs.push_back(submit(worker));
        }
        worker();
        for (auto& f : futs) f.get();
    }

    usize workerCount() const noexcept { return m_workers.size(); }

private:
    void workerMain(usize /*id*/) {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(m_mutex);
                m_cv.wait(lk, [this] { return m_stop || !m_tasks.empty(); });
                if (m_stop && m_tasks.empty()) return;
                task = std::move(m_tasks.front());
                m_tasks.pop_front();
            }
            task();
        }
    }

    std::vector<std::thread>     m_workers;
    std::deque<std::function<void()>> m_tasks;
    std::mutex                   m_mutex;
    std::condition_variable      m_cv;
    bool                         m_stop;
};

// Global engine thread pool
ThreadPool& globalThreadPool() noexcept;

} // namespace pk
