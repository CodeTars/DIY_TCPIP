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