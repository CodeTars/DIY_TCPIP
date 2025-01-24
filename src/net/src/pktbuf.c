#include "pktbuf.h"
#include "nlocker.h"
#include "dbg.h"
#include "mblock.h"

static nlocker_t locker;
static mblock_t block_list;
static pktblk_t block_buffer[PKTBUF_BLK_CNT];
static mblock_t pktbuf_list;
static pktbuf_t pktbuf_buffer[PKTBUF_BUF_CNT];

net_err_t pktbuf_init(void) {
    dbg_info(DBG_PKTBUF, "init pktbuf list.");
    net_err_t err = nlocker_init(&locker, NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_PKTBUF, "init locker error");
        return err;
    }

    err = mblock_init(&block_list, block_buffer, PKTBUF_BLK_CNT, sizeof(pktblk_t), NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_PKTBUF, "init block_list error");
        nlocker_destroy(&locker);
        return err;
    }

    err = mblock_init(&pktbuf_list, pktbuf_buffer, PKTBUF_BUF_CNT, sizeof(pktbuf_t), NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_PKTBUF, "init pktbuf_list error");
        nlocker_destroy(&locker);
        return err;
    }

    dbg_info(DBG_PKTBUF, "init done.");
    return NET_ERR_OK;
}

static pktblk_t *pktblock_alloc(void) {
    pktblk_t *block = mblock_alloc(&block_list, -1);
    if (block) {
        block->size = 0;
        block->data = (uint8_t *)0;
        nlist_node_init(&block->node);
    }
    return block;
}

static void pktblock_alloc_list(pktbuf_t *buf, int size, int add_front) {
    nlist_init(&buf->blk_list);
    while (size) {
        int cur_size = size > PKTBUF_BLK_SIZE ? PKTBUF_BLK_SIZE : size;
        pktblk_t *block = pktblock_alloc();
        if (!block) {
            dbg_error(DBG_PKTBUF, "no buffer for alloc (size:%d)", size);
            return;
        }
        block->size = size;
        if (add_front) {
            // 头插法
            block->data = block->payload + PKTBUF_BLK_SIZE - cur_size;
            nlist_insert_first(&buf->blk_list, &block->node);
        } else {
            // 尾插法
            block->data = block->payload;
            nlist_insert_last(&buf->blk_list, &block->node);
        }
        size -= cur_size;
    }
}

pktbuf_t *pktbuf_alloc(int size) {
    pktbuf_t *buf = mblock_alloc(&pktbuf_list, -1);
    if (!buf) {
        dbg_error(DBG_PKTBUF, "no free pkt buffer");
        return (pktbuf_t *)0;
    }
    buf->total_size = size;

    pktblock_alloc_list(buf, size, 0);
    return buf;
}

void pktbuf_free(pktbuf_t *buf) {
    // todo
}