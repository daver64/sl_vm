#include <sys/mman.h>
#include <map>
std::map<void*, size_t> allocmap;
void *virtual_alloc(const size_t numbytes)
{
    void *pointer = mmap(nullptr,numbytes,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,0,0);
    assert(pointer);
    allocmap[pointer] = numbytes;
    return pointer;
}
void virtual_free(void *pointer)
{
    assert(pointer);
    size_t numbytes=alocmap[pointer];
    if(numbytes>0)
    {
        VirtualFree(pointer,numbytes,MEM_DECOMMIT);
        allocmap.erase(pointer);
    }
}