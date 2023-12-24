/**
 * (c) 2023 David Rowbotham thedaver64@gmail.com
 */

// test
#include <cstdio>
#include <cstring>
#define VIRTUAL_ALLOC_IMPLEMENTATION
#include "sl_vm.h"

int main(int argc, char *argv[])
{
    // quick and dirty test. allocate a big chunk, set the last byte to something,
    // then read it back. 
    void *mem = virtual_alloc(1024 * 1024 * 1024);
    memset(mem,0,1024 * 1024 * 1024);
    (((unsigned char*)mem)[(1024 * 1024 * 1024) - 1])=42;
    unsigned char lb=(((unsigned char*)mem)[(1024 * 1024 * 1024) - 1]);
    printf("last bit=%u\n",lb);
    virtual_free(mem);
}