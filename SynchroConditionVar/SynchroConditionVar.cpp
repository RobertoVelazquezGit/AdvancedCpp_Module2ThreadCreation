#include <condition_variable>
#include <queue>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <cmath>
#include <atomic>

class ThreadSafeQueue {
private:
    mutable std::mutex mutex_;
    std::queue<double> queue_;
    std::condition_variable condition_;
    bool finished_ = false;

public:
    void push(double item) {
        //std::cout << "push\n";
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(item);
        condition_.notify_one();
    }

    bool pop(double& item) {
        //std::cout << "pop waiting\n";
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty() || finished_; });

        //std::cout << "pop awakened\n";

        if (queue_.empty()) {
            return false;  // No more items and producer finished
        }

        item = queue_.front();
        queue_.pop();
        return true;
    }

    void finish() {
        std::lock_guard<std::mutex> lock(mutex_);
        finished_ = true;
        condition_.notify_all();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};

static std::mutex coutMutex;

void producer(ThreadSafeQueue& queue, int producerId, int itemsPerProducer)
{
    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Producer " << producerId << " START\n";
    }

    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dist(1.0, 1000.0);

    for (int i = 0; i < itemsPerProducer; ++i)
    {
        queue.push(dist(gen));

        // Simula trabajo de producción
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Producer " << producerId << " END\n";
    }
}

void consumer(ThreadSafeQueue& queue,
    int consumerId,
    std::atomic<int>& processedItems)
{

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Consumer " << consumerId << " START\n";
    }

    double value;

    while (queue.pop(value))
    {
        // Simula trabajo de procesamiento
        volatile double result = std::sqrt(value);

        ++processedItems;

        std::this_thread::sleep_for(std::chrono::microseconds(80));
    }

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Consumer " << consumerId << " END\n";
    }
}

int main()
{
    const int producerCount = 4;
    const int consumerCount = 2;
    const int itemsPerProducer = /*5000*/100;

    ThreadSafeQueue queue;

    std::atomic<int> processedItems = 0;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // steady_clock is a monotonic clock, which means it cannot be adjusted and is not affected by changes in the system time.
    // This makes it suitable for measuring intervals.  
	auto start = std::chrono::steady_clock::now();  

    // Crear consumidores
    for (int i = 0; i < consumerCount; ++i)
    {
        consumers.emplace_back(
            consumer,
            std::ref(queue),
            i,
            std::ref(processedItems));
    }

    // Crear productores
    for (int i = 0; i < producerCount; ++i)
    {
        producers.emplace_back(
            producer,
            std::ref(queue),
            i,
            itemsPerProducer);
    }

    // Esperar productores
    for (auto& t : producers)
        t.join();

    // Ya no habrá más elementos
    queue.finish();

    // Esperar consumidores
    for (auto& t : consumers)
        t.join();

    auto end = std::chrono::steady_clock::now();

    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\nStatistics\n";
    std::cout << "----------\n";
    std::cout << "Producers      : " << producerCount << '\n';
    std::cout << "Consumers      : " << consumerCount << '\n';
    std::cout << "Items produced : "
        << producerCount * itemsPerProducer << '\n';
    std::cout << "Items consumed : "
        << processedItems.load() << '\n';
    std::cout << "Execution time : "
        << elapsed.count() << " ms\n";

    return 0;
}