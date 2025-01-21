#ifndef MBLOCKER_H
#define MBLOCKER_H

#include "nlist.h"
#include "nlocker.h"

typedef struct _mblock_t {
    void *start;
    nlist_t free_list;
    nlocker_t locker;
    sys_sem_t alloc_sem;
} mblock_t;

net_err_t mblock_init(mblock_t *mblock, void *mem, int cnt, int blksize, nlocker_type_t share_type);
void *mblock_alloc(mblock_t *mblock, int ms);
int mblock_free_cnt(mblock_t *mblock);
void mblock_free(mblock_t *mblock, void *block);
void mblock_destroy(mblock_t *mblock);

#endif