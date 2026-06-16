#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>

std::mutex dataMutex;
std::vector<double> sharedData;
std::atomic<int> processedCount{ 0 };

void dataProcessor(int processorId, int itemCount) {

    if(processorId == 2) {
		int n = 0;  
	}   

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1.0, 100.0);

    for (int i = 0; i < itemCount; ++i) {
        double value = dis(gen);

        {
            std::lock_guard<std::mutex> lock(dataMutex);
            sharedData.push_back(value);
        }

        processedCount.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Processor " << processorId << " completed\n";
}

int main() {
    std::vector<std::thread> threads;

    // Create 5 threads, each processing 20 items
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(dataProcessor, i, 20);
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Total processed: " << processedCount.load() << std::endl;
    std::cout << "Data size: " << sharedData.size() << std::endl;

    return 0;
}