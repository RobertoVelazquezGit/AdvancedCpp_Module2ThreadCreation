#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

// Original from example
//class ThreadGuard {
//private:
//    std::thread& thread_;
//
//public:
//    explicit ThreadGuard(std::thread& t) : thread_(t) {}
//
//    ~ThreadGuard() {
//        if (thread_.joinable()) {
//            thread_.join();
//        }
//    }
//
//    // Prevent copying
//    ThreadGuard(const ThreadGuard&) = delete;
//    ThreadGuard& operator=(const ThreadGuard&) = delete;
//};

class ThreadGuard {
private:
    std::thread* thread_;

public:
    explicit ThreadGuard(std::thread& t)
        : thread_(&t) {
    }

    ~ThreadGuard() {
        if (thread_ && thread_->joinable())
            thread_->join();
    }

    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

    // Explicitly define move operations because declaring a copy constructor
    // (even as = delete) prevents the compiler from automatically generating
    // the move constructor and move assignment operator.
    ThreadGuard(ThreadGuard&& other) noexcept
        : thread_(other.thread_)
    {
        other.thread_ = nullptr;
    }

    ThreadGuard& operator=(ThreadGuard&& other) noexcept
    {
        if (this != &other) {
            thread_ = other.thread_;
            other.thread_ = nullptr;
        }
        return *this;
    }
};

void riskAnalyzer(int analyzerId, const std::vector<double>& data,
    double& result) {
    try {
        if (data.empty()) {
            throw std::runtime_error("No data to analyze");
        }

        double sum = 0.0;
        for (double value : data) {
            sum += value * value;  // Risk calculation simulation
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        result = sum / data.size();
        std::cout << "Analyzer " << analyzerId << " calculated risk: "
            << result << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Analyzer " << analyzerId << " error: "
            << e.what() << std::endl;
        result = -1.0;
    }
}

int main() {
    std::vector<double> results(3);  // contains 3 elements
    std::vector<std::thread> threads;

    // Create data segments
    std::vector<std::vector<double>> segments = {
        {1.0, 2.0, 3.0, 4.0},
        {5.0, 6.0, 7.0},
        {}  // Empty segment to test exception handling
    };

    // Create threads with RAII management
    for (int i = 0; i < 3; ++i) {
		threads.emplace_back(riskAnalyzer, i, std::cref(segments[i]),  // constant reference becuase of riskAnalyzer function signature const std::vector<double>& data
            std::ref(results[i]));
    }

    // ThreadGuard ensures proper cleanup
    std::vector<ThreadGuard> guards;
	// Does not work guards.reserve(threads.size()); 
    for (auto& t : threads) {
        guards.emplace_back(t);
    }

    // Guards automatically join threads when going out of scope
    return 0;
}
