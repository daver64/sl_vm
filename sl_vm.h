/**
 * SL_VM - Cross-Platform Virtual Memory Allocator
 * (c) 2023-2025 David Rowbotham thedaver64@gmail.com
 * 
 * STB-style header-only library for cross-platform virtual memory allocation.
 * Define SL_VM_IMPLEMENTATION in exactly one source file before including.
 * 
 * Features:
 * - Thread-safe allocation tracking
 * - Proper error handling (no asserts in production)
 * - Page-aligned allocations
 * - Cross-platform (Windows/Unix/Linux/macOS)
 * - RAII wrapper class available
 * 
 * Usage:
 *   #define SL_VM_IMPLEMENTATION
 *   #include "sl_vm.h"
 * 
 * See manual.md for detailed documentation.
 */

#pragma once

#include <cstddef>

namespace sl_vm {

    // Error codes returned by allocation functions
    enum class Result {
        Success = 0,
        OutOfMemory,
        InvalidParameter,
        SystemError,
        NotFound
    };

    // Get human-readable error message
    const char* get_error_message(Result result);

    // Core allocation functions
    Result virtual_alloc(size_t numbytes, void** out_ptr);
    Result virtual_free(void* block);
    
    // Query functions
    Result get_allocation_size(void* block, size_t* out_size);
    size_t get_page_size();
    size_t get_total_allocated();
    size_t get_allocation_count();

    // RAII wrapper class for automatic cleanup
    class VirtualMemory {
    public:
        VirtualMemory() = default;
        explicit VirtualMemory(size_t size);
        ~VirtualMemory();
        
        // Move semantics
        VirtualMemory(VirtualMemory&& other) noexcept;
        VirtualMemory& operator=(VirtualMemory&& other) noexcept;
        
        // Disable copy semantics
        VirtualMemory(const VirtualMemory&) = delete;
        VirtualMemory& operator=(const VirtualMemory&) = delete;
        
        // Access
        void* get() const { return ptr_; }
        size_t size() const { return size_; }
        explicit operator bool() const { return ptr_ != nullptr; }
        
        // Manual control
        Result allocate(size_t size);
        void release();
        
    private:
        void* ptr_ = nullptr;
        size_t size_ = 0;
    };

    // Legacy C-style interface (deprecated but maintained for compatibility)
    void* virtual_alloc_legacy(size_t numbytes);  // Returns nullptr on failure
    void virtual_free_legacy(void* block);       // Safe to call with nullptr

} // namespace sl_vm

#ifdef SL_VM_IMPLEMENTATION

#include <unordered_map>
#include <mutex>
#include <atomic>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #include <cerrno>
#endif

namespace sl_vm {

    // Thread-safe allocation tracking
    static std::mutex g_alloc_mutex;
    static std::unordered_map<void*, size_t> g_allocmap;
    static std::atomic<size_t> g_total_allocated{0};
    static std::atomic<size_t> g_allocation_count{0};

    // Platform-specific page size caching
    static size_t g_page_size = 0;

    const char* get_error_message(Result result) {
        switch (result) {
            case Result::Success:           return "Success";
            case Result::OutOfMemory:       return "Out of memory";
            case Result::InvalidParameter:  return "Invalid parameter";
            case Result::SystemError:       return "System error";
            case Result::NotFound:          return "Allocation not found";
            default:                        return "Unknown error";
        }
    }

    size_t get_page_size() {
        if (g_page_size == 0) {
#ifdef _WIN32
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            g_page_size = si.dwPageSize;
#else
            g_page_size = static_cast<size_t>(getpagesize());
#endif
        }
        return g_page_size;
    }

    size_t get_total_allocated() {
        return g_total_allocated.load();
    }

    size_t get_allocation_count() {
        return g_allocation_count.load();
    }

    // Helper function to round up to page size
    static size_t round_up_to_page_size(size_t size) {
        const size_t page_size = get_page_size();
        return ((size + page_size - 1) / page_size) * page_size;
    }

    Result virtual_alloc(size_t numbytes, void** out_ptr) {
        if (out_ptr == nullptr) {
            return Result::InvalidParameter;
        }
        
        *out_ptr = nullptr;
        
        if (numbytes == 0) {
            return Result::InvalidParameter;
        }

        // Check for potential overflow
        const size_t max_size = SIZE_MAX / 2;
        if (numbytes > max_size) {
            return Result::InvalidParameter;
        }

        // Round up to page size for efficiency
        const size_t aligned_size = round_up_to_page_size(numbytes);

        void* block = nullptr;

#ifdef _WIN32
        block = VirtualAlloc(nullptr, aligned_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (block == nullptr) {
            DWORD error = GetLastError();
            return (error == ERROR_NOT_ENOUGH_MEMORY || error == ERROR_OUTOFMEMORY) 
                   ? Result::OutOfMemory : Result::SystemError;
        }
#else
        block = mmap(nullptr, aligned_size, PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (block == MAP_FAILED) {
            return (errno == ENOMEM) ? Result::OutOfMemory : Result::SystemError;
        }
#endif

        // Thread-safe tracking
        {
            std::lock_guard<std::mutex> lock(g_alloc_mutex);
            g_allocmap[block] = aligned_size;
        }
        
        g_total_allocated += aligned_size;
        g_allocation_count++;
        
        *out_ptr = block;
        return Result::Success;
    }

    Result virtual_free(void* block) {
        if (block == nullptr) {
            return Result::Success; // Free of nullptr is always safe
        }

        size_t allocation_size = 0;
        
        // Thread-safe lookup and removal
        {
            std::lock_guard<std::mutex> lock(g_alloc_mutex);
            auto it = g_allocmap.find(block);
            if (it == g_allocmap.end()) {
                return Result::NotFound;
            }
            allocation_size = it->second;
            g_allocmap.erase(it);
        }

        // Platform-specific deallocation
#ifdef _WIN32
        if (!VirtualFree(block, 0, MEM_RELEASE)) {
            // Re-add to map on failure
            std::lock_guard<std::mutex> lock(g_alloc_mutex);
            g_allocmap[block] = allocation_size;
            return Result::SystemError;
        }
#else
        if (munmap(block, allocation_size) != 0) {
            // Re-add to map on failure
            std::lock_guard<std::mutex> lock(g_alloc_mutex);
            g_allocmap[block] = allocation_size;
            return Result::SystemError;
        }
#endif

        g_total_allocated -= allocation_size;
        g_allocation_count--;
        
        return Result::Success;
    }

    Result get_allocation_size(void* block, size_t* out_size) {
        if (block == nullptr || out_size == nullptr) {
            return Result::InvalidParameter;
        }

        std::lock_guard<std::mutex> lock(g_alloc_mutex);
        auto it = g_allocmap.find(block);
        if (it == g_allocmap.end()) {
            return Result::NotFound;
        }
        
        *out_size = it->second;
        return Result::Success;
    }

    // RAII VirtualMemory implementation
    VirtualMemory::VirtualMemory(size_t size) {
        allocate(size);
    }

    VirtualMemory::~VirtualMemory() {
        release();
    }

    VirtualMemory::VirtualMemory(VirtualMemory&& other) noexcept 
        : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }

    VirtualMemory& VirtualMemory::operator=(VirtualMemory&& other) noexcept {
        if (this != &other) {
            release();
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    Result VirtualMemory::allocate(size_t size) {
        release(); // Free any existing allocation
        
        void* new_ptr = nullptr;
        Result result = virtual_alloc(size, &new_ptr);
        if (result == Result::Success) {
            ptr_ = new_ptr;
            size_ = size;
        }
        return result;
    }

    void VirtualMemory::release() {
        if (ptr_ != nullptr) {
            virtual_free(ptr_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }

    // Legacy C-style interface
    void* virtual_alloc_legacy(size_t numbytes) {
        void* ptr = nullptr;
        Result result = virtual_alloc(numbytes, &ptr);
        return (result == Result::Success) ? ptr : nullptr;
    }

    void virtual_free_legacy(void* block) {
        virtual_free(block); // Already handles nullptr safely
    }

} // namespace sl_vm

// Maintain backward compatibility with old global functions
#ifdef SL_VM_ENABLE_LEGACY_GLOBALS
void* virtual_alloc(size_t numbytes) {
    return sl_vm::virtual_alloc_legacy(numbytes);
}

void virtual_free(void* block) {
    sl_vm::virtual_free_legacy(block);
}
#endif

#endif // SL_VM_IMPLEMENTATION
