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

static inline int cur_blk_remain(pktbuf_t *buf) {
    return buf->cur_blk ? (buf->cur_blk->data + buf->cur_blk->size - buf->blk_offset) : 0;
}

static void pktbuf_pos_move_forward(pktbuf_t *buf, int size) {
    dbg_assert(buf->pos + size <= buf->total_size, "size error");

    buf->pos += size;
    buf->blk_offset += size;

    pktblk_t *cur = buf->cur_blk;
    dbg_assert(buf->blk_offset <= cur->data + cur->size, "size error");
    if (buf->blk_offset == cur->data + cur->size) {
        buf->cur_blk = pktbuf_next_blk(cur);
        buf->blk_offset = buf->cur_blk ? buf->cur_blk->data : (uint8_t *)0;
    }
}

#if DBG_DISP_ENABLED(DBG_LEVEL_INFO)
static void display_check_buf(pktbuf_t *buf) {
    if (!buf) {
        dbg_error(DBG_BUF, "invalid buf. buf == 0");
        return;
    }

    printf("check buf (%p), size: %d\n", buf, buf->total_size);
    int total_size = 0, index = 0;
    for (pktblk_t *blk = pktbuf_first_blk(buf); blk; blk = pktbuf_next_blk(blk)) {
        printf("id: %d, ", index++);

        if (blk->data < blk->payload || blk->data >= blk->payload + PKTBUF_BLK_SIZE) {
            dbg_error(DBG_BUF, "wrong block data address");
        }

        int pre_size = (int)(blk->data - blk->payload);
        printf("pre: %d bytes, ", pre_size);

        int used_size = blk->size;
        printf("used: %d bytes, ", used_size);

        int free_size = cur_blk_tail_free(blk);
        printf("free: %d bytes\n", free_size);

        int blk_total = pre_size + used_size + free_size;
        if (blk_total != PKTBUF_BLK_SIZE) {
            dbg_error(DBG_BUF, "bad block size. %d != %d", blk_total, PKTBUF_BLK_SIZE);
        }

        total_size += used_size;
    }

    if (total_size != buf->total_size) {
        dbg_error(DBG_BUF, "wrong buf size. %d != %d", total_size, buf->total_size);
    }
}
#else
#define display_check_buf(buf)
#endif

net_err_t pktbuf_init(void) {
    dbg_info(DBG_BUF, "init pktbuf list.");
    net_err_t err = nlocker_init(&locker, NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_BUF, "init locker error");
        return err;
    }

    err = mblock_init(&block_list, block_buffer, PKTBUF_BLK_CNT, sizeof(pktblk_t), NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_BUF, "init block_list error");
        nlocker_destroy(&locker);
        return err;
    }

    err = mblock_init(&pktbuf_list, pktbuf_buffer, PKTBUF_BUF_CNT, sizeof(pktbuf_t), NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_BUF, "init pktbuf_list error");
        nlocker_destroy(&locker);
        return err;
    }

    dbg_info(DBG_BUF, "init done.");
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

static net_err_t pktbuf_add_blocks(pktbuf_t *buf, int size, int add_front) {
    while (size) {
        int cur_size = size > PKTBUF_BLK_SIZE ? PKTBUF_BLK_SIZE : size;
        pktblk_t *block = pktblock_alloc();
        if (!block) {
            dbg_error(DBG_BUF, "no buffer for alloc (size:%d)", size);
            return NET_ERR_MEM;
        }
        block->size = cur_size;
        buf->total_size += cur_size;
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
    return NET_ERR_OK;
}

pktbuf_t *pktbuf_alloc(int size) {
    pktbuf_t *buf = mblock_alloc(&pktbuf_list, -1);
    if (!buf) {
        dbg_error(DBG_BUF, "no free pkt buffer");
        return (pktbuf_t *)0;
    }

    nlist_node_init(&buf->node);
    nlist_init(&buf->blk_list);
    net_err_t err = pktbuf_add_blocks(buf, size, 1);
    if (err < 0) {
        pktbuf_free(buf);
        return (pktbuf_t *)0;
    }

    pktbuf_reset_acc(buf);

    dbg_assert(buf->total_size == size, "wrong total size");
    display_check_buf(buf);
    return buf;
}

static void pktbuf_free_blocks(pktbuf_t *buf, pktblk_t *blk) {
    while (blk) {
        pktblk_t *next_blk = pktbuf_next_blk(blk);
        blk->size = 0;
        blk->data = (uint8_t *)0;
        nlist_remove(&buf->blk_list, &blk->node);
        mblock_free(&block_list, blk);
        blk = next_blk;
    }
}

void pktbuf_free(pktbuf_t *buf) {
    buf->total_size = 0;
    pktbuf_free_blocks(buf, pktbuf_first_blk(buf));
    // nlist_init(&buf->blk_list);
    mblock_free(&pktbuf_list, buf);
}

net_err_t pktbuf_add_header(pktbuf_t *buf, int size, int cont) {
    pktblk_t *block = pktbuf_first_blk(buf);
    int resv_size = (int)(block->data - block->payload);
    if (size <= resv_size) {
        block->size += size;
        block->data -= size;
        buf->total_size += size;
        display_check_buf(buf);
        return NET_ERR_OK;
    }
    if (cont) {
        if (size > PKTBUF_BLK_SIZE) {
            dbg_error(DBG_BUF, "add header continuously, too big size, %d > %d", size, PKTBUF_BLK_SIZE);
            return NET_ERR_SIZE;
        }
    } else {
        size -= resv_size;
        block->size += resv_size;
        block->data -= resv_size;
        buf->total_size += resv_size;
    }

    net_err_t err = pktbuf_add_blocks(buf, size, 1);
    if (err < 0) {
        dbg_error(DBG_BUF, "no free buffer (%d)", size);
        return err;
    }
    display_check_buf(buf);
    return NET_ERR_OK;
}

net_err_t pktbuf_remove_header(pktbuf_t *buf, int size) {
    pktblk_t *block = pktbuf_first_blk(buf);
    while (size) {
        pktblk_t *next_block = pktbuf_next_blk(block);
        if (size < block->size) {
            block->data += size;
            block->size -= size;
            buf->total_size -= size;
            break;
        }

        int cur_size = block->size;
        size -= cur_size;
        buf->total_size -= cur_size;

        nlist_remove_first(&buf->blk_list);
        mblock_free(&block_list, block);

        block = next_block;
    }

    display_check_buf(buf);
    return NET_ERR_OK;
}

net_err_t pktbuf_resize(pktbuf_t *buf, int to_size) {
    net_err_t err = NET_ERR_OK;
    if (to_size == buf->total_size) {
        err = NET_ERR_OK;
    } else if (buf->total_size == 0) {
        err = pktbuf_add_blocks(buf, to_size, 0);
    } else if (to_size == 0) {
        buf->total_size = 0;
        pktbuf_free_blocks(buf, pktbuf_first_blk(buf));
        err = NET_ERR_OK;
    } else if (to_size > buf->total_size) {
        int inc_size = to_size - buf->total_size;
        pktblk_t *block = pktbuf_last_blk(buf);
        int remain_size = cur_blk_tail_free(block);
        if (inc_size <= remain_size) {
            block->size += inc_size;
            buf->total_size += inc_size;
            err = NET_ERR_OK;
        } else {
            block->size += remain_size;
            buf->total_size += remain_size;
            inc_size -= remain_size;
            err = pktbuf_add_blocks(buf, inc_size, 0);
        }
    } else if (to_size < buf->total_size) {
        int total_size = 0;
        pktblk_t *tail_block;
        for (tail_block = pktbuf_first_blk(buf); tail_block; tail_block = pktbuf_next_blk(tail_block)) {
            total_size += tail_block->size;
            if (total_size >= to_size) {
                break;
            }
        }

        int dec_size = total_size - to_size;
        tail_block->size -= dec_size;
        buf->total_size = to_size;

        tail_block = pktbuf_next_blk(tail_block);
        pktbuf_free_blocks(buf, tail_block);
        err = NET_ERR_OK;
    }

    display_check_buf(buf);
    return err;
}

net_err_t pktbuf_join(pktbuf_t *dst, pktbuf_t *src) {
    nlist_join(&dst->blk_list, &src->blk_list);
    dst->total_size += src->total_size;
    pktbuf_free(src);

    dbg_info(DBG_BUF, "join result:");
    display_check_buf(dst);
    return NET_ERR_OK;
}

net_err_t pktbuf_set_cont(pktbuf_t *buf, int size) {
    if (size > buf->total_size) {
        dbg_error(DBG_BUF, "to big size (%d) > buf's total size (%d)", size, buf->total_size);
        return NET_ERR_SIZE;
    }
    if (size > PKTBUF_BLK_SIZE) {
        dbg_error(DBG_BUF, "to big size (%d) > PKTBUF_BLK_SIZE (%d)", size, PKTBUF_BLK_SIZE);
        return NET_ERR_SIZE;
    }

    pktblk_t *first_blk = pktbuf_first_blk(buf);
    if (size <= first_blk->size) {
        display_check_buf(buf);
        return NET_ERR_OK;
    }

    uint8_t *dst = first_blk->payload;
    for (int i = 0; i < first_blk->size; i++) {
        *dst++ = first_blk->data[i];
    }
    first_blk->data = first_blk->payload;

    pktblk_t *blk = pktbuf_next_blk(first_blk);
    int remain_size = size - first_blk->size;
    while (remain_size) {
        int cur_size = min(remain_size, blk->size);
        memcpy(dst, blk->data, cur_size);

        blk->data += cur_size;
        blk->size -= cur_size;

        dst += cur_size;
        first_blk->size += cur_size;
        remain_size -= cur_size;

        if (blk->size == 0) {
            pktblk_t *next_blk = pktbuf_next_blk(blk);
            nlist_remove(&buf->blk_list, &blk->node);
            mblock_free(&block_list, blk);
            blk = next_blk;
        }
    }

    display_check_buf(buf);
    return NET_ERR_OK;
}

void pktbuf_reset_acc(pktbuf_t *buf) {
    if (buf) {
        buf->pos = 0;
        buf->cur_blk = pktbuf_first_blk(buf);
        buf->blk_offset = buf->cur_blk ? buf->cur_blk->data : (uint8_t *)0;
    }
}

net_err_t pktbuf_write(pktbuf_t *buf, uint8_t *src, int size) {
    if (!src || !size) {
        return NET_ERR_PARAM;
    }

    if (buf->total_size - buf->pos < size) {
        dbg_error(DBG_BUF, "size error, %d < %d", buf->total_size - buf->pos, size);
        return NET_ERR_SIZE;
    }

    while (size) {
        int cur_blk_remain_size = cur_blk_remain(buf);
        int write_size = min(size, cur_blk_remain_size);

        memcpy(buf->blk_offset, src, write_size);
        src += write_size;
        size -= write_size;

        pktbuf_pos_move_forward(buf, write_size);
    }

    return NET_ERR_OK;
}

net_err_t pktbuf_read(pktbuf_t *buf, uint8_t *dst, int size) {
    if (!dst || !size) {
        return NET_ERR_OK;
    }

    int buf_remain_size = buf->total_size - buf->pos;
    if (buf_remain_size < size) {
        dbg_error(DBG_BUF, "size error, %d < %d", buf_remain_size, size);
        return NET_ERR_SIZE;
    }

    while (size) {
        int cur_blk_remain_size = cur_blk_remain(buf);
        int read_size = min(size, cur_blk_remain_size);

        memcpy(dst, buf->blk_offset, read_size);
        dst += read_size;
        size -= read_size;

        pktbuf_pos_move_forward(buf, read_size);
    }
}

net_err_t pktbuf_seek(pktbuf_t *buf, int offset) {
    if (offset == buf->pos) {
        return NET_ERR_OK;
    }

    if (offset < 0 || offset >= buf->total_size) {
        dbg_error(DBG_BUF, "wrong position");
        return NET_ERR_SIZE;
    }

    if (offset < buf->pos) {
        pktbuf_reset_acc(buf);
    }

    int move_bytes = offset - buf->pos;
    while (move_bytes) {
        int blk_remain_bytes = cur_blk_remain(buf);
        int cur_move = min(blk_remain_bytes, move_bytes);

        move_bytes -= cur_move;
        pktbuf_pos_move_forward(buf, cur_move);
    }

    return NET_ERR_OK;
}