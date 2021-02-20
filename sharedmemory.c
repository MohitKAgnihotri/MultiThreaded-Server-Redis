//
// Created by 310165137 on 11/02/2021.
//

#include "sharedmemory.h"

void *create_shared_memory(size_t size) {
    // Our memory buffer will be readable and writable:
    int protection = PROT_READ | PROT_WRITE;

    // The buffer will be shared (meaning other processes can access it), but
    // anonymous (meaning third-party processes cannot obtain an address for it),
    // so only this process and its children will be able to use it:
    int visibility = MAP_SHARED | MAP_ANONYMOUS;

    // The remaining parameters to `mmap()` are not important for this use case,
    // but the manpage for `mmap` explains their purpose.
    void *ptr = mmap(NULL, size, protection, visibility, -1, 0);
    if (ptr)
    {
        memset(ptr,0x00,size);
    }
    return ptr;
}

void sharedmem_writeDatabase(void* ptr, void *req, int msglen)
{
    if (ptr && req)
    {
        memcpy(ptr+SHARED_MEM_CACHE_OFFSET+1, req,msglen);
    }
}

void sharedmem_ReadDatabase(void* ptr, void *req, int msglen)
{
    if (ptr && req)
    {
        memcpy(req, ptr+SHARED_MEM_CACHE_OFFSET+1, msglen);
    }
}

void sharedmem_writeWhiteList(void* ptr, void *req, int msglen)
{
    if (ptr && req)
    {
        memcpy(ptr+SHARED_MEM_WHITELIST_OFFSET+1, req,msglen);
    }
}

void sharedmem_readWhiteList(void* ptr, void *req, int msglen)
{
    if (ptr && req)
    {
        memcpy(req, ptr+SHARED_MEM_WHITELIST_OFFSET+1, msglen);
    }
}