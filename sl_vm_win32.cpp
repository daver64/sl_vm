#include <windows.h>
#include <cassert>
#include <cstdio>
std::map<void *, size_t> allocmap;
void *virtual_alloc(const size_t numbytes)
{
    void *pointer = VirtualAlloc(NULL, numbytes, MEM_COMMIT, PAGE_READWRITE);
    assert(pointer);
    allocmap[pointer] = numbytes;
    return pointer;
}
void virtual_free(void *pointer)
{
    assert(pointer);
    size_t numbytes = allocmap[pointer];
    if (numbytes > 0)
    {
        int result = VirtualFree(pointer, 0, MEM_RELEASE);
        assert(result != 0);
        allocmap.erase(pointer);
    }
}
void virtual_stats()
{
    printf("num allocations=%zu\n", allocmap.size());
    for (auto alloc : allocmap)
    {
        printf("allocation address = %p\t:allocation size = %zu\n",
               alloc.first, alloc.second);
    }
}