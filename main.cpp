#include <iostream>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>

template <class Func>
class ProtectedQueue {
private:
    std::queue<std::function<Func>> protected_queue_;
    std::mutex mtx_;
    std::condition_variable con_var_;
public:
    explicit ProtectedQueue()= default;
    void push_queue(const std::function<Func>& func) {
        std::unique_lock<std::mutex> s_lock{ mtx_ };
        protected_queue_.push(func);
        con_var_.notify_one();
    }
    std::function<Func> get_queue() {
        std::unique_lock<std::mutex> s_lock{ mtx_ };
        con_var_.wait(s_lock);
        return protected_queue_.front();
    }
};


template <class Func>
class PoolThread {
private:
    std::vector<std::thread> thread_pool;
    std::weak_ptr<ProtectedQueue<Func>> w_ptr_queue;
    std::mutex mtx;
    void work() {
        if (auto strong_ptr = w_ptr_queue.lock())
            strong_ptr->get_queue()();
    }
public:
    explicit PoolThread(const size_t processor_count, const  std::shared_ptr<ProtectedQueue<Func>> &sh_ptr) : w_ptr_queue(sh_ptr) {
        thread_pool.reserve(processor_count);
        for (std::vector<std::thread>::size_type i = 0; i < processor_count; ++i) {
            thread_pool.push_back(std::thread (&PoolThread::work, this));
        }
    }
    void submit(const std::function<Func>& func) {
        if (auto strong_ptr = w_ptr_queue.lock())
            strong_ptr->push_queue(func);
    }
    virtual ~PoolThread() { for (auto&& i : thread_pool) { i.join(); } }
};

void print_num() {
    std::cout << "1" << std::endl;
}

int main() {
    const auto processor_count = std::thread::hardware_concurrency();
    auto pool = std::make_unique<PoolThread<void()>>(processor_count, std::make_shared<ProtectedQueue<void()>>());
    for (size_t i = 0; i < processor_count; ++i) {
        if (!(i % 2))
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        pool->submit(print_num);
    }
    
    return 0;
}