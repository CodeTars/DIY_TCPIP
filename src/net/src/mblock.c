#include "mblock.h"
#include "dbg.h"

net_err_t mblock_init(mblock_t *mblock, void *mem, int cnt, int blksize, nlocker_type_t share_type)
{
    dbg_assert(blksize > sizeof(nlist_node_t), "block size error"); // 保证每个块能容纳一个node

    mblock->start = mem;
    uint8_t *buf = mem;
    nlist_init(&mblock->free_list);
    for (int i = 0; i < cnt; i++, buf += blksize)
    {
        nlist_node_t *node = (nlist_node_t *)buf;
        nlist_insert_first(&mblock->free_list, node);
    }

    nlocker_init(&mblock->locker, share_type);

    if (share_type != NLOCKER_NONE)
    {
        if ((mblock->alloc_sem = sys_sem_create(cnt)) == SYS_SEM_INVALID)
        {
            dbg_error(DBG_MBLOCK, "create sem failed");
            nlocker_destroy(&mblock->locker);
            return NET_ERR_SYS;
        }
    }
    return NET_ERR_OK;
}

void *mblock_alloc(mblock_t *mblock, int ms)
{
    if (mblock->locker.type != NLOCKER_NONE && ms >= 0)
    {
        sys_sem_wait(mblock->alloc_sem, ms);
    }

    int cnt = mblock_free_cnt(mblock);
    if (cnt > 0)
    {
        nlocker_lock(&mblock->locker);
        nlist_node_t *node = nlist_remove_first(&mblock->free_list);
        nlocker_unlock(&mblock->locker);
        return (void *)node;
    }
    return (void *)0;
}

int mblock_free_cnt(mblock_t *mblock)
{
    nlocker_lock(&mblock->locker);
    int cnt = nlist_count(&mblock->free_list);
    nlocker_unlock(&mblock->locker);
    return cnt;
}

void mblock_free(mblock_t *mblock, void *block)
{
    nlocker_lock(&mblock->locker);
    nlist_insert_first(&mblock->free_list, block);
    if (mblock->locker.type != NLOCKER_NONE)
    {
        sys_sem_notify(mblock->alloc_sem);
    }
    nlocker_unlock(&mblock->locker);
}

void mblock_destroy(mblock_t *mblock)
{
    if (mblock->locker.type != NLOCKER_NONE)
    {
        sys_sem_free(mblock->alloc_sem);
        nlocker_destroy(&mblock->locker);
    }
}