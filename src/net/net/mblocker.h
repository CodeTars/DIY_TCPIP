#ifndef MBLOCKER_H
#define MBLOCKER_H

#include "nlist.h"
#include "nlocker.h"

typedef struct _mlocker
{
    void *start;
    nlist_t free_list;
    nlocker_t locker;
    sys_sem_t alloc_sem;
} mlocker_t;

#endif