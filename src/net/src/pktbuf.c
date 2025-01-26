#include "pktbuf.h"
#include "nlocker.h"
#include "dbg.h"
#include "mblock.h"

static nlocker_t locker;
static mblock_t block_list;
static pktblk_t block_buffer[PKTBUF_BLK_CNT];
static mblock_t pktbuf_list;
static pktbuf_t pktbuf_buffer[PKTBUF_BUF_CNT];

static int cur_blk_tail_free(pktblk_t *blk) {
    return (int)((blk->payload + PKTBUF_BLK_SIZE) - (blk->data + blk->size));
}

#if DBG_DISP_ENABLED(DBG_LEVEL_INFO)
static void display_check_buf(pktbuf_t *buf) {
    if (!buf) {
        dbg_error(DBG_PKTBUF, "invalid buf. buf == 0");
        return;
    }

    printf("check buf (%p), size: %d\n", buf, buf->total_size);
    int total_size = 0, index = 0;
    for (pktblk_t *blk = pktbuf_first_blk(buf); blk; blk = pktbuf_next_blk(blk)) {
        printf("id: %d, ", index++);

        if (blk->data < blk->payload || blk->data >= blk->payload + PKTBUF_BLK_SIZE) {
            dbg_error(DBG_PKTBUF, "wrong block data address");
        }

        int pre_size = (int)(blk->data - blk->payload);
        printf("pre: %d bytes, ", pre_size);

        int used_size = blk->size;
        printf("used: %d bytes, ", used_size);

        int free_size = cur_blk_tail_free(blk);
        printf("free: %d bytes\n", free_size);

        int blk_total = pre_size + used_size + free_size;
        if (blk_total != PKTBUF_BLK_SIZE) {
            dbg_error(DBG_PKTBUF, "bad block size. %d != %d", blk_total, PKTBUF_BLK_SIZE);
        }

        total_size += used_size;
    }

    if (total_size != buf->total_size) {
        dbg_error(DBG_PKTBUF, "wrong buf size. %d != %d", total_size, buf->total_size);
    }
}
#else
#define display_check_buf(buf)
#endif

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
            pktbuf_free(buf);
            return;
        }
        block->size = cur_size;
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
    nlist_node_init(&buf->node);

    pktblock_alloc_list(buf, size, 1);
    display_check_buf(buf);
    return buf;
}

void pktbuf_free(pktbuf_t *buf) {
    for (pktblk_t *blk = pktbuf_first_blk(buf); blk;) {
        pktblk_t *next_blk = pktbuf_next_blk(blk);
        mblock_free(&block_list, blk);
        blk = next_blk;
    }
    mblock_free(&pktbuf_list, buf);
}