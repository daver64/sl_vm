# SL_VM - Cross-Platform Virtual Memory Allocator

A modern, thread-safe, cross-platform virtual memory allocator implemented as an STB-style header-only library.

## 🚀 Features

- **Cross-Platform**: Windows (VirtualAlloc) and Unix (mmap) support
- **Thread-Safe**: All operations protected by internal synchronization  
- **Modern C++**: RAII wrapper class with move semantics
- **Error Handling**: Comprehensive error codes, no assertions
- **Memory Tracking**: Built-in allocation statistics and tracking
- **Page-Aligned**: All allocations automatically page-aligned
- **Header-Only**: Easy integration, just include the header

## 📦 Quick Start

```cpp
#define SL_VM_IMPLEMENTATION
#include "sl_vm.h"
#include <iostream>

int main() {
    // RAII approach (recommended)
    sl_vm::VirtualMemory memory(1024 * 1024); // 1MB
    
    if (memory) {
        std::cout << "Allocated " << memory.size() << " bytes at " 
                  << memory.get() << std::endl;
        
        // Use the memory...
        char* buffer = static_cast<char*>(memory.get());
        buffer[0] = 'H';
        buffer[1] = 'i';
        
        // Memory automatically freed when 'memory' goes out of scope
    }
    
    return 0;
}
```

## 🔧 Installation

1. Copy `sl_vm.h` to your project
2. In **exactly one** source file:
   ```cpp
   #define SL_VM_IMPLEMENTATION
   #include "sl_vm.h"
   ```
3. In other files, just include normally:
   ```cpp
   #include "sl_vm.h"
   ```

## 📚 Documentation

- **[Manual](manual.md)** - Complete API reference and examples
- **[Legacy Migration](#legacy-compatibility)** - Upgrading from the old version

## 🎯 Use Cases

- Large memory allocations without heap fragmentation
- Memory-mapped file alternatives
- High-performance computing with virtual memory
- Systems programming requiring direct OS memory management
- Applications needing page-aligned memory buffers

## 🔄 API Overview

### Modern C++ Interface (Recommended)

```cpp
// RAII allocation
sl_vm::VirtualMemory memory(size);

// Manual control
void* ptr = nullptr;
sl_vm::Result result = sl_vm::virtual_alloc(size, &ptr);
if (result == sl_vm::Result::Success) {
    sl_vm::virtual_free(ptr);
}
```

### Error Handling

```cpp
void* ptr = nullptr;
sl_vm::Result result = sl_vm::virtual_alloc(1024 * 1024, &ptr);

switch (result) {
    case sl_vm::Result::Success:
        // Success - use ptr
        sl_vm::virtual_free(ptr);
        break;
    case sl_vm::Result::OutOfMemory:
        std::cerr << "Out of memory!" << std::endl;
        break;
    case sl_vm::Result::InvalidParameter:
        std::cerr << "Invalid parameters!" << std::endl;
        break;
    default:
        std::cerr << "Error: " << sl_vm::get_error_message(result) << std::endl;
}
```

### Statistics and Queries

```cpp
std::cout << "Page size: " << sl_vm::get_page_size() << " bytes" << std::endl;
std::cout << "Total allocated: " << sl_vm::get_total_allocated() << " bytes" << std::endl;
std::cout << "Active allocations: " << sl_vm::get_allocation_count() << std::endl;

// Query specific allocation size
size_t actual_size = 0;
sl_vm::get_allocation_size(ptr, &actual_size);
```

## 🔒 Thread Safety

All functions are thread-safe and can be called concurrently from multiple threads:

```cpp
// Safe to call from multiple threads
void worker_thread(int thread_id) {
    sl_vm::VirtualMemory buffer(1024 * 1024);
    if (buffer) {
        // Process data...
        std::cout << "Thread " << thread_id << " got buffer" << std::endl;
    }
    // Automatic cleanup
}
```

## ⚡ Performance

- **O(1)** allocation and deallocation
- **Page-aligned** memory (typically 4KB boundaries)
- **Zero-overhead** abstraction over OS virtual memory APIs
- **Automatic size rounding** to page boundaries for efficiency

Example allocation behavior:
```cpp
void* ptr = nullptr;
sl_vm::virtual_alloc(1000, &ptr);  // Requests 1000 bytes

size_t actual_size = 0;
sl_vm::get_allocation_size(ptr, &actual_size);
// actual_size will be 4096 (rounded up to page size)
```

## 🔧 Build Example

### CMake
```cmake
cmake_minimum_required(VERSION 3.10)
project(my_project)

set(CMAKE_CXX_STANDARD 11)

add_executable(my_app 
    main.cpp
    # other source files...
)

# SL_VM is header-only, just ensure it's in include path
target_include_directories(my_app PRIVATE path/to/sl_vm)
```

### Manual Compilation
```bash
# Linux/macOS
g++ -std=c++11 -O2 main.cpp -o my_app

# Windows (MSVC)
cl /EHsc /O2 main.cpp
```

## 🔄 Legacy Compatibility

For backward compatibility with the original API:

```cpp
#define SL_VM_ENABLE_LEGACY_GLOBALS
#define SL_VM_IMPLEMENTATION
#include "sl_vm.h"

// Old functions still work
void* ptr = virtual_alloc(1024);
virtual_free(ptr);
```

**Migration recommendations:**
- Use `sl_vm::VirtualMemory` for RAII
- Check error codes instead of relying on assertions
- Migrate to namespaced functions when possible

## 🆚 Comparison with Alternatives

| Feature | SL_VM | malloc/new | mmap/VirtualAlloc |
|---------|-------|------------|-------------------|
| Cross-platform | ✅ | ✅ | ❌ |
| Thread-safe tracking | ✅ | ❌ | ❌ |
| Page alignment | ✅ | ❌ | ✅ |
| RAII wrapper | ✅ | ❌ | ❌ |
| Error handling | ✅ | ❌ | ✅ |
| Header-only | ✅ | ❌ | ❌ |

## 🐛 Troubleshooting

**Common issues:**

1. **Linker errors**: Ensure `SL_VM_IMPLEMENTATION` is defined in exactly one source file
2. **Large allocations fail**: Check system virtual memory limits (`ulimit -v` on Unix)
3. **Memory leaks**: Use RAII or ensure every `virtual_alloc()` has matching `virtual_free()`

**Debug tips:**
```cpp
// Check allocation statistics
std::cout << "Active allocations: " << sl_vm::get_allocation_count() << std::endl;
std::cout << "Total memory: " << sl_vm::get_total_allocated() << " bytes" << std::endl;
```

## 📝 License

(c) 2023-2025 David Rowbotham (thedaver64@gmail.com)

## 🤝 Contributing

Issues and pull requests welcome! Please ensure:
- Cross-platform compatibility (Windows/Unix)
- Thread safety is maintained  
- Comprehensive error handling
- Documentation updates for API changes

## 📈 Changelog

### Version 2.0 (September 2025)
- ✨ **NEW**: Thread-safe allocation tracking
- ✨ **NEW**: Modern C++ RAII wrapper class  
- ✨ **NEW**: Comprehensive error handling (no more assertions)
- ✨ **NEW**: Memory statistics and query functions
- ✨ **NEW**: Proper mmap error checking (MAP_FAILED)
- 🔧 **IMPROVED**: Page-aligned allocations for better performance
- 🔧 **IMPROVED**: Namespace organization (sl_vm::)
- 🔧 **IMPROVED**: STB-style implementation (SL_VM_IMPLEMENTATION)
- 🛡️ **FIXED**: Race conditions in multi-threaded environments
- 🛡️ **FIXED**: Memory leak potential with allocation tracking
- 📚 **DOCS**: Complete manual and API reference

### Version 1.0 (December 2023)
- Initial STB-style header-only implementation
- Basic Windows/Unix cross-platform support