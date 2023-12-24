/**
 * (c) 2023 David Rowbotham thedaver64@gmail.com
 */

#pragma once
void *virtual_alloc(const size_t numbytes);
void virtual_free(void *block);

#ifdef VIRTUAL_ALLOC_IMPLEMENTATION
#include <map>
#include <cstdio>
static std::map<void *, size_t> allocmap;
static bool map_contains(void *block)
{
    if (allocmap.find(block) != allocmap.end())
    {
        return true;
    }
    return false;
}
#ifdef _WIN32
#include <windows.h>
#include <cassert>
void *virtual_alloc(const size_t numbytes)
{
    void *block = VirtualAlloc(NULL, numbytes, MEM_COMMIT, PAGE_READWRITE);
    assert(block);
    allocmap[block] = numbytes;
    return block;
}
void virtual_free(void *block)
{
    assert(block);
    assert(map_contains(block));
    size_t numbytes = allocmap[block];
    if (numbytes > 0)
    {
        int result = VirtualFree(block, 0, MEM_RELEASE);
        assert(result != 0);
        allocmap.erase(block);
    }
}
#else
#include <sys/mman.h>
#include <cassert>
void *virtual_alloc(const size_t numbytes)
{
    void *block = mmap(nullptr, numbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    assert(block);
    allocmap[block] = numbytes;
    return block;
}
void virtual_free(void *block)
{
    assert(block);
    assert(map_contains(block));
    size_t numbytes = allocmap[block];
    if (numbytes > 0)
    {
        int result = munmap(block, numbytes);
        assert(result == 0);
        allocmap.erase(block);
    }
}
#endif
#endif
