/**
 * (c) 2023 David Rowbotham thedaver64@gmail.com
*/


// test
#include <cstdio>
#include "sl_vm.h"

int main(int argc, char *argv[])
{
    virtual_stats();
    void *mem = virtual_alloc(1024 * 1024 * 1024);
    virtual_stats();
    virtual_free(mem);
    virtual_stats();
}