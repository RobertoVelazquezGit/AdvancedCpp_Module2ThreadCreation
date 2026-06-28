// ============================================================================
// Basic Multithreading Lab - Solution Files
// Complete implementation of basic multithreading concepts
// ============================================================================

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <numeric>
#include <string>

using namespace std;

// Global variables for demonstration
mutex printMutex;  // For thread-safe printing
atomic<int> taskCounter{ 0 };  // Atomic counter for tasks

/**
 * Simple calculation function that each thread will execute
 * Calculates sum of numbers from start to end
 */
int calculateSum(int start, int end) {
    int sum = 0;
    for (int i = start; i <= end; i++) {
        sum += i;
    }

    // Simulate some processing time
    this_thread::sleep_for(chrono::milliseconds(100));

    return sum;
}

/**
 * Thread-safe printing function
 * Uses mutex to ensure clean output when multiple threads print
 */
void safePrint(const string& message) {
    // SOLUTION: Thread-safe printing using lock_guard
    lock_guard<mutex> lock(printMutex);
    cout << message << endl;
}

/**
 * Worker function that each thread will execute
 * Demonstrates basic thread operations and synchronization
 */
void workerFunction(int threadId, int start, int end) {
    // SOLUTION: Complete worker function implementation

    // 1. Print starting message
    safePrint("Thread " + to_string(threadId) + " starting - processing range " +
        to_string(start) + " to " + to_string(end));

    // 2. Calculate the sum
    int result = calculateSum(start, end);

    // 3. Increment atomic counter
    taskCounter.fetch_add(1);  // or taskCounter++

    // 4. Print result
    safePrint("Thread " + to_string(threadId) + " completed - sum = " + to_string(result));
}

int main() {
    cout << "=== Basic Multithreading Lab ===" << endl;
    cout << "Learning thread creation, lifecycle management, and synchronization" << endl;

    // SOLUTION: Create and manage threads
    const int NUM_THREADS = 4;
    const int RANGE_PER_THREAD = 250;  // Each thread processes 250 numbers

    // Create vector to store thread objects
    vector<thread> threads;

    cout << "\nStarting " << NUM_THREADS << " worker threads..." << endl;

    // SOLUTION: Create and launch threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int start = i * RANGE_PER_THREAD + 1;
        int end = (i + 1) * RANGE_PER_THREAD;

        threads.emplace_back(workerFunction, i, start, end);
        // Alternative: threads.push_back(thread(workerFunction, i, start, end));
    }

    cout << "All threads started. Waiting for completion..." << endl;

    // SOLUTION: Wait for all threads to complete
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    cout << "\nAll threads completed!" << endl;
    cout << "Total tasks completed: " << taskCounter.load() << endl;

    // SOLUTION: Demonstrate thread detaching (BONUS)
    cout << "\nStarting detached thread..." << endl;

    thread detachedThread([]() {
        safePrint("Detached thread running independently!");
        this_thread::sleep_for(chrono::milliseconds(150));
        safePrint("Detached thread finished!");
        });

    // Detach the thread to run independently
    detachedThread.detach();

    // Give detached thread a chance to run
    this_thread::sleep_for(chrono::milliseconds(200));

    cout << "Main thread finishing. Detached thread may still be running." << endl;

    return 0;
}

/*
VALIDATION CHECKLIST COMPLETED:
- Thread-safe printing implemented using mutex and lock_guard

- Worker function processes data and updates atomic counter correctly

- Multiple threads created using std::thread constructor with proper parameters

- All threads joined properly to ensure completion before main() exits

- Thread lifecycle management demonstrates both join() and detach() patterns

- Atomic operations used correctly for shared counter access

- No race conditions in output - all text prints cleanly

- Program demonstrates core multithreading concepts without complexity

EXPECTED OUTPUT EXAMPLE:
=== Basic Multithreading Lab ===
Learning thread creation, lifecycle management, and synchronization

Starting 4 worker threads...
All threads started. Waiting for completion...
Thread 0 starting - processing range 1 to 250
Thread 1 starting - processing range 251 to 500
Thread 2 starting - processing range 501 to 750
Thread 3 starting - processing range 751 to 1000
Thread 0 completed - sum = 31375
Thread 1 completed - sum = 93875
Thread 2 completed - sum = 156375
Thread 3 completed - sum = 218875

All threads completed!
Total tasks completed: 4

Starting detached thread...
Detached thread running independently!
Detached thread finished!
Main thread finishing. Detached thread may still be running.
*/