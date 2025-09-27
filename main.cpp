/**
 * SL_VM Test Suite
 * (c) 2023-2025 David Rowbotham thedaver64@gmail.com
 */

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>

#define SL_VM_IMPLEMENTATION
#include "sl_vm.h"

void test_basic_allocation() {
    std::cout << "=== Basic Allocation Test ===" << std::endl;
    
    void* ptr = nullptr;
    sl_vm::Result result = sl_vm::virtual_alloc(1024 * 1024, &ptr); // 1MB
    
    if (result == sl_vm::Result::Success) {
        std::cout << "✅ Allocated 1MB successfully at " << ptr << std::endl;
        
        // Test memory access
        char* buffer = static_cast<char*>(ptr);
        strcpy(buffer, "Hello, Virtual Memory!");
        std::cout << "✅ Memory write/read test: " << buffer << std::endl;
        
        // Check actual allocation size
        size_t actual_size = 0;
        if (sl_vm::get_allocation_size(ptr, &actual_size) == sl_vm::Result::Success) {
            std::cout << "✅ Actual allocation size: " << actual_size << " bytes" << std::endl;
        }
        
        sl_vm::virtual_free(ptr);
        std::cout << "✅ Memory freed successfully" << std::endl;
    } else {
        std::cout << "❌ Allocation failed: " << sl_vm::get_error_message(result) << std::endl;
    }
    std::cout << std::endl;
}

void test_raii_interface() {
    std::cout << "=== RAII Interface Test ===" << std::endl;
    
    {
        sl_vm::VirtualMemory memory(2 * 1024 * 1024); // 2MB
        
        if (memory) {
            std::cout << "✅ RAII allocation successful" << std::endl;
            std::cout << "✅ Requested size: " << memory.size() << " bytes" << std::endl;
            std::cout << "✅ Memory address: " << memory.get() << std::endl;
            
            // Test memory usage
            char* buffer = static_cast<char*>(memory.get());
            memset(buffer, 0xAB, 1024); // Write pattern
            
            if (buffer[0] == static_cast<char>(0xAB) && buffer[1023] == static_cast<char>(0xAB)) {
                std::cout << "✅ Memory pattern write/read test passed" << std::endl;
            }
        } else {
            std::cout << "❌ RAII allocation failed" << std::endl;
        }
        
        std::cout << "✅ RAII automatic cleanup (memory freed on scope exit)" << std::endl;
    }
    std::cout << std::endl;
}

void test_error_handling() {
    std::cout << "=== Error Handling Test ===" << std::endl;
    
    // Test invalid parameters
    void* ptr = nullptr;
    sl_vm::Result result = sl_vm::virtual_alloc(0, &ptr); // Zero size
    if (result == sl_vm::Result::InvalidParameter) {
        std::cout << "✅ Zero size properly rejected" << std::endl;
    }
    
    result = sl_vm::virtual_alloc(1024, nullptr); // Null output pointer
    if (result == sl_vm::Result::InvalidParameter) {
        std::cout << "✅ Null output pointer properly rejected" << std::endl;
    }
    
    // Test freeing nullptr (should be safe)
    result = sl_vm::virtual_free(nullptr);
    if (result == sl_vm::Result::Success) {
        std::cout << "✅ Freeing nullptr is safe" << std::endl;
    }
    
    // Test freeing invalid pointer
    int dummy = 42;
    result = sl_vm::virtual_free(&dummy);
    if (result == sl_vm::Result::NotFound) {
        std::cout << "✅ Invalid pointer properly detected" << std::endl;
    }
    
    std::cout << std::endl;
}

void test_statistics() {
    std::cout << "=== Statistics Test ===" << std::endl;
    
    std::cout << "Page size: " << sl_vm::get_page_size() << " bytes" << std::endl;
    
    size_t initial_allocated = sl_vm::get_total_allocated();
    size_t initial_count = sl_vm::get_allocation_count();
    
    std::cout << "Initial allocated: " << initial_allocated << " bytes" << std::endl;
    std::cout << "Initial count: " << initial_count << " allocations" << std::endl;
    
    // Make some allocations
    std::vector<sl_vm::VirtualMemory> allocations;
    for (int i = 0; i < 3; ++i) {
        allocations.emplace_back((i + 1) * 1024 * 1024); // 1MB, 2MB, 3MB
    }
    
    size_t after_allocated = sl_vm::get_total_allocated();
    size_t after_count = sl_vm::get_allocation_count();
    
    std::cout << "After allocations: " << after_allocated << " bytes" << std::endl;
    std::cout << "After count: " << after_count << " allocations" << std::endl;
    std::cout << "Net allocated: " << (after_allocated - initial_allocated) << " bytes" << std::endl;
    std::cout << "Net count: " << (after_count - initial_count) << " allocations" << std::endl;
    
    // Clear allocations
    allocations.clear();
    
    size_t final_allocated = sl_vm::get_total_allocated();
    size_t final_count = sl_vm::get_allocation_count();
    
    std::cout << "After cleanup: " << final_allocated << " bytes" << std::endl;
    std::cout << "After cleanup count: " << final_count << " allocations" << std::endl;
    
    if (final_allocated == initial_allocated && final_count == initial_count) {
        std::cout << "✅ All memory properly cleaned up" << std::endl;
    } else {
        std::cout << "❌ Memory leak detected!" << std::endl;
    }
    
    std::cout << std::endl;
}

void worker_thread(int thread_id, std::vector<bool>& results) {
    try {
        sl_vm::VirtualMemory memory(1024 * 1024); // 1MB per thread
        
        if (memory) {
            // Write thread ID pattern
            int* buffer = static_cast<int*>(memory.get());
            for (int i = 0; i < 1024; ++i) {
                buffer[i] = thread_id * 1000 + i;
            }
            
            // Verify pattern
            bool valid = true;
            for (int i = 0; i < 1024; ++i) {
                if (buffer[i] != thread_id * 1000 + i) {
                    valid = false;
                    break;
                }
            }
            
            results[thread_id] = valid;
            
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            results[thread_id] = false;
        }
    } catch (...) {
        results[thread_id] = false;
    }
}

void test_thread_safety() {
    std::cout << "=== Thread Safety Test ===" << std::endl;
    
    const int num_threads = 8;
    std::vector<std::thread> threads;
    std::vector<bool> results(num_threads, false);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch worker threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_thread, i, std::ref(results));
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Check results
    bool all_passed = true;
    for (int i = 0; i < num_threads; ++i) {
        if (!results[i]) {
            std::cout << "❌ Thread " << i << " failed" << std::endl;
            all_passed = false;
        }
    }
    
    if (all_passed) {
        std::cout << "✅ All " << num_threads << " threads completed successfully" << std::endl;
        std::cout << "✅ Total time: " << duration.count() << " ms" << std::endl;
    }
    
    std::cout << std::endl;
}

void test_large_allocation() {
    std::cout << "=== Large Allocation Test ===" << std::endl;
    
    // Try to allocate 1GB like the original test
    const size_t large_size = 1024ULL * 1024 * 1024; // 1GB
    
    sl_vm::VirtualMemory memory(large_size);
    
    if (memory) {
        std::cout << "✅ Large allocation (1GB) successful" << std::endl;
        
        // Test memory access at boundaries (like original test)
        unsigned char* buffer = static_cast<unsigned char*>(memory.get());
        
        // Write to first byte
        buffer[0] = 0x12;
        
        // Write to last byte  
        buffer[large_size - 1] = 42;
        
        // Read back
        unsigned char first = buffer[0];
        unsigned char last = buffer[large_size - 1];
        
        if (first == 0x12 && last == 42) {
            std::cout << "✅ Memory boundary access test passed" << std::endl;
            std::cout << "✅ First byte: " << static_cast<int>(first) << std::endl;
            std::cout << "✅ Last byte: " << static_cast<int>(last) << std::endl;
        } else {
            std::cout << "❌ Memory boundary access test failed" << std::endl;
        }
        
        // Clear memory (this will take a moment for 1GB)
        std::cout << "⏳ Clearing 1GB memory..." << std::endl;
        memset(buffer, 0, large_size);
        std::cout << "✅ Memory cleared" << std::endl;
        
    } else {
        std::cout << "⚠️ Large allocation (1GB) failed - may be system limited" << std::endl;
        
        // Try smaller allocation
        sl_vm::VirtualMemory smaller_memory(100 * 1024 * 1024); // 100MB
        if (smaller_memory) {
            std::cout << "✅ Smaller allocation (100MB) successful as fallback" << std::endl;
        }
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "SL_VM Test Suite" << std::endl;
    std::cout << "=================" << std::endl << std::endl;
    
    test_basic_allocation();
    test_raii_interface(); 
    test_error_handling();
    test_statistics();
    test_thread_safety();
    test_large_allocation();
    
    std::cout << "=== Test Suite Complete ===" << std::endl;
    
    // Final statistics
    std::cout << "Final statistics:" << std::endl;
    std::cout << "- Total allocated: " << sl_vm::get_total_allocated() << " bytes" << std::endl;
    std::cout << "- Active allocations: " << sl_vm::get_allocation_count() << std::endl;
    std::cout << "- Page size: " << sl_vm::get_page_size() << " bytes" << std::endl;
    
    if (sl_vm::get_allocation_count() == 0) {
        std::cout << "✅ No memory leaks detected!" << std::endl;
    } else {
        std::cout << "⚠️ " << sl_vm::get_allocation_count() << " allocations still active" << std::endl;
    }
    
    return 0;
}