#include <cstdio>
#include "sl_vm.h"

int main(int argc, char *argv[])
{
    void *mem = virtual_alloc(1024*1024*1024);
    printf("mem alloc@ %p\n",mem);
    virtual_free(mem);
}