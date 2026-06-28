// Compile (GCC/Clang): g++ -std=gnu++20 -O2 -pthread task4_benchmark.cpp -o bench
// Compile (MSVC):      cl /std:c++20 /O2 task4_benchmark.cpp

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace bench {

    // ---------- Utilities ----------
    std::mutex ioMutex;

    // Thread-safe print
    template <class... Args>
	void ts_print(Args&&... args) {  // universal reference 
        std::lock_guard<std::mutex> lock(ioMutex);
		(std::cout << ... << args) << std::endl;  // fold expression (C++17)    
		// () parentheses to return a reference to output stream, and then << operator is applied to each argument in the pack  
    }

    // Deterministic RNG per thread
    inline thread_local std::mt19937 rng{
        [] {
            static std::mt19937 seeder{12345};
            static std::mutex m;
            std::lock_guard<std::mutex> lock(m);
            return std::mt19937{seeder()};
        }()
    };

    // RAII benchmark timer
    class PerformanceBenchmark {
        std::chrono::high_resolution_clock::time_point start_;
        std::string name_;
    public:
        explicit PerformanceBenchmark(std::string name)
            : start_(std::chrono::high_resolution_clock::now()), name_(std::move(name)) {
        }
        ~PerformanceBenchmark() {
            const auto end = std::chrono::high_resolution_clock::now();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
            ts_print(name_, " took: ", ms, " ms");
        }
    };

    // ---------- Workload helpers ----------
    inline double work_fn(double v) {
        double x = std::sqrt(v * v + 1.0);
        return std::log1p(x);
    }

    std::vector<double> make_data(int n) {
        std::uniform_real_distribution<> dis(1.0, 10.0);
        std::vector<double> data(n);
        std::generate(data.begin(), data.end(), [&] { return dis(rng); });
        return data;
    }

    // ---------- Implementations ----------

    // Single-threaded
    double processSingleThreaded(const std::vector<double>& data) {
        double acc = 0.0;
        for (double v : data) acc += work_fn(v);
        return acc;
    }

    // Multi-threaded
    double processMultiThreaded(const std::vector<double>& data, unsigned threads) {
        if (threads == 0) threads = 1;
        const unsigned parts = std::min<unsigned>(
            threads, std::max(1u, std::thread::hardware_concurrency())
        );

        std::vector<double> partial(parts, 0.0);
        std::vector<std::thread> ths;
        ths.reserve(parts);

        const size_t n = data.size();
        const size_t base = n / parts;
        const size_t rem = n % parts;

        auto worker = [&](unsigned idx, size_t begin, size_t end) {
            double acc = 0.0;
            for (size_t i = begin; i < end; ++i) acc += work_fn(data[i]);
            partial[idx] = acc;
            };

        size_t start = 0;
        for (unsigned p = 0; p < parts; ++p) {
            size_t len = base + (p < rem ? 1 : 0);
            size_t finish = start + len;
            ths.emplace_back(worker, p, start, finish);
            start = finish;
        }

        for (auto& t : ths) t.join();

        return std::accumulate(partial.begin(), partial.end(), 0.0);
    }

    // ---------- Benchmark harness ----------
    void runBenchmark(int dataSize, unsigned threadsForMulti /* 0 = auto */) {
        auto data = make_data(dataSize);

        double singleSum = 0.0;
        {
            PerformanceBenchmark b("Single-threaded processing");
            singleSum = processSingleThreaded(data);
        }
        ts_print("[single] checksum = ", singleSum);

        const unsigned autoThreads =
            threadsForMulti == 0 ? std::max(1u, std::thread::hardware_concurrency()) : threadsForMulti;

        double multiSum = 0.0;
        {
            PerformanceBenchmark b(std::string("Multi-threaded processing (")
                + std::to_string(autoThreads) + " threads)");
            multiSum = processMultiThreaded(data, autoThreads);
        }
        ts_print("[multi ] checksum = ", multiSum);

        const double diff = std::abs(singleSum - multiSum);
        ts_print("Checksum delta = ", diff);
    }

} // namespace bench

// ---------- Choose scenarios ----------
int main() {
    using namespace bench;
    const int DATA_SIZE = 2'000'000;

    runBenchmark(DATA_SIZE, 0);   // auto
    runBenchmark(DATA_SIZE, 1);
    runBenchmark(DATA_SIZE, 2);
    runBenchmark(DATA_SIZE, 4);
    runBenchmark(DATA_SIZE, 8);
    runBenchmark(DATA_SIZE, 16);

    return 0;
}