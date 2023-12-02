#include <sys/mman.h>
#include <map>
#include <cassert>
#include <cstdio>
std::map<void *, size_t> allocmap;
void *virtual_alloc(const size_t numbytes)
{
    void *pointer = mmap(nullptr, numbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
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
        int result = munmap(pointer, numbytes);
        assert(result == 0);
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