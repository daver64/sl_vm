# SL_VM Manual - Cross-Platform Virtual Memory Allocator

## Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [Installation](#installation)
4. [Quick Start](#quick-start)
5. [API Reference](#api-reference)
6. [RAII Interface](#raii-interface)
7. [Error Handling](#error-handling)
8. [Thread Safety](#thread-safety)
9. [Performance Considerations](#performance-considerations)
10. [Migration Guide](#migration-guide)
11. [Examples](#examples)
12. [Platform Notes](#platform-notes)

## Overview

SL_VM is a cross-platform virtual memory allocator designed as an STB-style header-only library. It provides a unified interface for virtual memory allocation across Windows and Unix-like systems (Linux, macOS, BSD variants).

Unlike standard malloc/new, SL_VM allocates memory directly from the operating system's virtual memory subsystem, providing:
- Large block allocations without heap fragmentation
- Memory that's guaranteed to be page-aligned
- Direct OS memory management bypass
- Better control over memory commitment and protection

## Features

- **Cross-Platform**: Supports Windows (VirtualAlloc) and Unix (mmap)
- **Thread-Safe**: All operations are protected by internal synchronization
- **Error Handling**: Comprehensive error reporting without assertions
- **RAII Support**: Modern C++ wrapper class for automatic cleanup
- **Memory Tracking**: Built-in allocation tracking and statistics
- **Page Alignment**: All allocations are automatically page-aligned
- **Header-Only**: Easy integration with STB-style inclusion

## Installation

SL_VM is a header-only library. Simply:

1. Copy `sl_vm.h` to your project
2. In exactly **one** source file, define the implementation:

```cpp
#define SL_VM_IMPLEMENTATION
#include "sl_vm.h"
```

3. In other files, just include the header:

```cpp
#include "sl_vm.h"
```

### CMake Integration

```cmake
# Add to your CMakeLists.txt
target_include_directories(your_target PRIVATE path/to/sl_vm)
```

## Quick Start

### Modern C++ Interface (Recommended)

```cpp
#define SL_VM_IMPLEMENTATION
#include "sl_vm.h"
#include <iostream>

int main() {
    // RAII allocation - automatically freed on scope exit
    sl_vm::VirtualMemory memory(1024 * 1024); // 1MB
    
    if (memory) {
        std::cout << "Allocated " << memory.size() << " bytes\n";
        
        // Use the memory
        char* buffer = static_cast<char*>(memory.get());
        buffer[0] = 'H';
        buffer[1] = 'i';
        buffer[2] = '\0';
        
        std::cout << "Message: " << buffer << std::endl;
    }
    
    return 0;
}
```

### C-Style Interface

```cpp
#define SL_VM_IMPLEMENTATION
#include "sl_vm.h"
#include <iostream>

int main() {
    void* ptr = nullptr;
    sl_vm::Result result = sl_vm::virtual_alloc(1024 * 1024, &ptr);
    
    if (result == sl_vm::Result::Success) {
        std::cout << "Allocation successful\n";
        
        // Use the memory...
        
        // Don't forget to free!
        sl_vm::virtual_free(ptr);
    } else {
        std::cout << "Allocation failed: " 
                  << sl_vm::get_error_message(result) << std::endl;
    }
    
    return 0;
}
```

## API Reference

### Core Functions

#### `sl_vm::Result virtual_alloc(size_t numbytes, void** out_ptr)`

Allocates virtual memory from the operating system.

**Parameters:**
- `numbytes`: Size in bytes to allocate (must be > 0)
- `out_ptr`: Pointer to receive the allocated memory address

**Returns:**
- `Result::Success`: Allocation successful, `*out_ptr` contains valid pointer
- `Result::InvalidParameter`: Invalid input (nullptr out_ptr, zero size, overflow)
- `Result::OutOfMemory`: System out of memory
- `Result::SystemError`: Other system error

**Notes:**
- Memory is automatically rounded up to page boundaries
- Memory is zero-initialized on most systems
- All allocations are page-aligned

#### `sl_vm::Result virtual_free(void* block)`

Frees previously allocated virtual memory.

**Parameters:**
- `block`: Pointer returned by `virtual_alloc()` (nullptr is safe)

**Returns:**
- `Result::Success`: Memory freed successfully
- `Result::NotFound`: Block was not allocated by this library
- `Result::SystemError`: System error during deallocation

### Query Functions

#### `sl_vm::Result get_allocation_size(void* block, size_t* out_size)`

Retrieves the size of an allocated block.

```cpp
void* ptr = nullptr;
if (sl_vm::virtual_alloc(1000, &ptr) == sl_vm::Result::Success) {
    size_t actual_size = 0;
    sl_vm::get_allocation_size(ptr, &actual_size);
    std::cout << "Actual size: " << actual_size << " bytes\n"; // Likely 4096 (page size)
}
```

#### `size_t get_page_size()`

Returns the system page size in bytes.

#### `size_t get_total_allocated()`

Returns total bytes currently allocated by the library.

#### `size_t get_allocation_count()`

Returns the number of active allocations.

### Error Handling

#### `const char* get_error_message(Result result)`

Returns human-readable error message for a result code.

```cpp
sl_vm::Result result = sl_vm::virtual_alloc(SIZE_MAX, &ptr);
if (result != sl_vm::Result::Success) {
    std::cerr << "Error: " << sl_vm::get_error_message(result) << std::endl;
}
```

## RAII Interface

The `VirtualMemory` class provides automatic memory management:

### Constructors and Destructor

```cpp
// Default constructor (no allocation)
sl_vm::VirtualMemory memory;

// Allocate on construction
sl_vm::VirtualMemory memory(1024 * 1024);

// Move constructor
sl_vm::VirtualMemory memory2 = std::move(memory);

// Destructor automatically frees memory
```

### Methods

#### `Result allocate(size_t size)`

Manually allocate memory (frees any existing allocation first).

#### `void release()`

Manually free memory.

#### `void* get() const`

Get raw pointer to allocated memory.

#### `size_t size() const`

Get requested allocation size.

#### `explicit operator bool() const`

Check if memory is allocated.

### Example Usage

```cpp
void process_large_data() {
    sl_vm::VirtualMemory buffer(10 * 1024 * 1024); // 10MB
    
    if (!buffer) {
        throw std::runtime_error("Failed to allocate memory");
    }
    
    // Use buffer.get() for raw access
    std::memset(buffer.get(), 0, buffer.size());
    
    // Memory automatically freed when buffer goes out of scope
}
```

## Error Handling

SL_VM uses explicit error codes instead of exceptions or assertions:

```cpp
void safe_allocation_example() {
    void* ptr = nullptr;
    sl_vm::Result result = sl_vm::virtual_alloc(1024, &ptr);
    
    switch (result) {
        case sl_vm::Result::Success:
            // Use ptr safely
            sl_vm::virtual_free(ptr);
            break;
            
        case sl_vm::Result::OutOfMemory:
            // Handle out of memory
            break;
            
        case sl_vm::Result::InvalidParameter:
            // Handle invalid parameters
            break;
            
        case sl_vm::Result::SystemError:
            // Handle system errors
            break;
    }
}
```

## Thread Safety

All SL_VM functions are thread-safe:

- Multiple threads can allocate/free simultaneously
- Internal tracking is protected by mutexes
- Statistics are updated atomically
- No external synchronization required

```cpp
// Safe to call from multiple threads
void worker_thread() {
    sl_vm::VirtualMemory buffer(1024 * 1024);
    // Use buffer...
    // Automatic cleanup on thread exit
}
```

## Performance Considerations

### Memory Alignment

All allocations are automatically aligned to system page boundaries (typically 4KB):

```cpp
void* ptr = nullptr;
sl_vm::virtual_alloc(1000, &ptr);  // Actually allocates 4096 bytes (one page)

size_t actual_size = 0;
sl_vm::get_allocation_size(ptr, &actual_size);
std::cout << actual_size << std::endl;  // Prints 4096
```

### Best Practices

1. **Large Allocations**: SL_VM is optimized for large allocations (>= page size)
2. **Batch Operations**: Prefer fewer large allocations over many small ones
3. **RAII Usage**: Use `VirtualMemory` class to prevent leaks
4. **Error Checking**: Always check return codes

### Performance Characteristics

- **Allocation**: O(1) system call + O(1) map insertion
- **Deallocation**: O(1) system call + O(1) map lookup/removal  
- **Query**: O(1) map lookup
- **Memory Overhead**: ~24 bytes per allocation (map entry)

## Migration Guide

### From Original SL_VM

The new version maintains backward compatibility:

```cpp
// Old code (still works with SL_VM_ENABLE_LEGACY_GLOBALS)
#define SL_VM_ENABLE_LEGACY_GLOBALS
#define SL_VM_IMPLEMENTATION
#include "sl_vm.h"

void* ptr = virtual_alloc(1024);  // Old function
virtual_free(ptr);                // Old function

// New recommended approach
void* ptr2 = nullptr;
if (sl_vm::virtual_alloc(1024, &ptr2) == sl_vm::Result::Success) {
    sl_vm::virtual_free(ptr2);
}
```

### From malloc/new

```cpp
// Old malloc approach
void* ptr = malloc(1024 * 1024);
if (ptr) {
    // use ptr
    free(ptr);
}

// SL_VM approach
sl_vm::VirtualMemory memory(1024 * 1024);
if (memory) {
    // use memory.get()
    // automatic cleanup
}
```

## Examples

### Basic Usage

```cpp
#define SL_VM_IMPLEMENTATION
#include "sl_vm.h"
#include <iostream>
#include <cstring>

int main() {
    // Allocate 1MB
    sl_vm::VirtualMemory buffer(1024 * 1024);
    
    if (buffer) {
        std::cout << "Allocated " << buffer.size() << " bytes\n";
        std::cout << "Actual size: ";
        
        size_t actual_size = 0;
        if (sl_vm::get_allocation_size(buffer.get(), &actual_size) == sl_vm::Result::Success) {
            std::cout << actual_size << " bytes\n";
        }
        
        // Write some data
        char* data = static_cast<char*>(buffer.get());
        std::strcpy(data, "Hello, Virtual Memory!");
        std::cout << "Data: " << data << std::endl;
    }
    
    return 0;
}
```

### Error Handling Example

```cpp
#include "sl_vm.h"
#include <iostream>

bool allocate_safely(size_t size, void** out_ptr) {
    sl_vm::Result result = sl_vm::virtual_alloc(size, out_ptr);
    
    if (result != sl_vm::Result::Success) {
        std::cerr << "Allocation failed: " 
                  << sl_vm::get_error_message(result) << std::endl;
        return false;
    }
    
    return true;
}

int main() {
    void* ptr = nullptr;
    
    // Try to allocate reasonable amount
    if (allocate_safely(1024 * 1024, &ptr)) {
        std::cout << "Success!\n";
        sl_vm::virtual_free(ptr);
    }
    
    // Try to allocate unreasonable amount
    if (!allocate_safely(SIZE_MAX, &ptr)) {
        std::cout << "Expected failure for SIZE_MAX\n";
    }
    
    return 0;
}
```

### Statistics Example

```cpp
#include "sl_vm.h"
#include <iostream>

void print_stats() {
    std::cout << "Total allocated: " << sl_vm::get_total_allocated() << " bytes\n";
    std::cout << "Active allocations: " << sl_vm::get_allocation_count() << "\n";
    std::cout << "Page size: " << sl_vm::get_page_size() << " bytes\n";
}

int main() {
    print_stats();
    
    {
        sl_vm::VirtualMemory mem1(1000);
        sl_vm::VirtualMemory mem2(2000);
        
        std::cout << "After allocations:\n";
        print_stats();
    }
    
    std::cout << "After cleanup:\n";
    print_stats();
    
    return 0;
}
```

## Platform Notes

### Windows
- Uses `VirtualAlloc()` with `MEM_COMMIT | MEM_RESERVE`
- Memory is immediately committed (physical memory allocated)
- Page size typically 4KB or 64KB depending on system

### Unix/Linux/macOS
- Uses `mmap()` with `MAP_PRIVATE | MAP_ANONYMOUS`
- Memory may be lazily allocated (copy-on-write)
- Page size typically 4KB, but can vary (use `get_page_size()`)

### Memory Initialization
- Windows: Memory is zero-initialized
- Unix: Memory is zero-initialized
- Both platforms guarantee clean memory

### Limitations
- Minimum allocation is one page (typically 4KB)
- Maximum allocation is platform-dependent
- Memory cannot be resized (allocate new block instead)

### Error Codes by Platform

| Error | Windows Cause | Unix Cause |
|-------|---------------|------------|
| `OutOfMemory` | `ERROR_NOT_ENOUGH_MEMORY` | `ENOMEM` |
| `SystemError` | Other Windows errors | Other errno values |

## Troubleshooting

### Common Issues

1. **Linker Errors**: Ensure `SL_VM_IMPLEMENTATION` is defined in exactly one source file
2. **Large Allocations Fail**: Check system virtual memory limits
3. **Memory Not Freed**: Use RAII or ensure `virtual_free()` is called
4. **Thread Issues**: Library is thread-safe, no additional synchronization needed

### Debug Tips

```cpp
// Enable detailed error information
void debug_allocation(size_t size) {
    void* ptr = nullptr;
    sl_vm::Result result = sl_vm::virtual_alloc(size, &ptr);
    
    std::cout << "Allocation of " << size << " bytes: ";
    
    if (result == sl_vm::Result::Success) {
        size_t actual_size = 0;
        sl_vm::get_allocation_size(ptr, &actual_size);
        std::cout << "SUCCESS (actual: " << actual_size << " bytes)\n";
        sl_vm::virtual_free(ptr);
    } else {
        std::cout << "FAILED (" << sl_vm::get_error_message(result) << ")\n";
    }
}
```

For additional help, please refer to the examples in the repository or open an issue on GitHub.