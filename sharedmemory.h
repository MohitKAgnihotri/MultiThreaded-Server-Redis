//
// Created by 310165137 on 11/02/2021.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdio.h>       /* standard I/O functions.              */
#include <stdlib.h>      /* malloc(), free() etc.                */
#include <string.h>
#include <sys/types.h>   /* various type definitions.            */
#include <sys/ipc.h>     /* general SysV IPC structures          */
#include <sys/msg.h>     /* message queue functions and structs. */

#ifndef SERVER_SHAREDMEMORY_H
#define SERVER_SHAREDMEMORY_H

void *create_shared_memory(size_t size);
#define SHM_SIZE 0x20000
#define SHARED_MEM_REQ_OFFSET 0

void sharedmem_writeDatabase(void* ptr, void *req, int msglen);
void sharedmem_ReadDatabase(void* ptr, void *req, int msglen);

#endif //SERVER_SHAREDMEMORY_H
