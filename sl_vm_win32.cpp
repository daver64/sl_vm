#include <windows.h>
#include <cassert>
void *virtual_alloc(const size_t numbytes)
{
    void *pointer = VirtualAlloc(NULL, numbytes, MEM_COMMIT, PAGE_READWRITE);
    assert(pointer);
    return pointer;
}
void virtual_free(void *pointer)
{
    assert(pointer);
    VirtualFree(pointer,0,MEM_RELEASE);
}