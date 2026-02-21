/**
 * @file test_threadpool.cpp
 * @brief Unit tests for the Threadpool class.
 */

#include "Threadpool.h"
#include <iostream>
#include <vector>
#include <chrono>

int main() {
    std::cout << "Testing Threadpool..." << std::endl;
    
    // Create a pool with 4 worker threads
    Threadpool pool(4);
    std::vector<std::future<int>> results;

    auto start_time = std::chrono::steady_clock::now();

    // Enqueue 8 tasks
    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.add_task([i] {
                std::cout << "Task " << i << " executing in thread " 
                          << std::this_thread::get_id() << std::endl;
                          
                // Simulate heavy work (1 second)
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                return i * i;
            })
        );
    }

    // Wait for all results
    for(auto && result: results) {
        std::cout << "Result: " << result.get() << std::endl; // get() blocks until task is done
    }
    
    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    // 8 tasks taking 1s each on 4 threads should take ~2 seconds total
    std::cout << "Total time: " << elapsed.count() << " seconds." << std::endl;
    
    if (elapsed.count() < 3.0) {
        std::cout << "✓ Threadpool works correctly in parallel!" << std::endl;
        return 0;
    } else {
        std::cerr << "✗ Threadpool did not execute in parallel." << std::endl;
        return 1;
    }
}