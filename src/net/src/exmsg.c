#include "exmsg.h"
#include "net_plat.h"
#include "dbg.h"
#include "fixq.h"
#include "mblock.h"

static void *msgtbl[EXMSG_CNT];
static fixq_t msg_queue;              // 消息队列
static exmsg_t msg_buffer[EXMSG_CNT]; // 消息块
static mblock_t msg_block;            // 消息块管理器

net_err_t exmsg_netif_in(void) {
    exmsg_t *msg = mblock_alloc(&msg_block, 0);
    if (!msg) {
        dbg_warning(DBG_EXMSG, "no free buffer");
        return NET_ERR_MEM;
    }

    static int id = 0;
    msg->id = ++id;

    net_err_t err = fixq_send(&msg_queue, msg, 0);
    if (err < 0) {
        dbg_warning(DBG_EXMSG, "queue full");
        mblock_free(&msg_block, msg);
        return err;
    }

    return NET_ERR_OK;
}

net_err_t exmsg_init(void) {
    dbg_info(DBG_EXMSG, "init exmsg");

    net_err_t err = fixq_init(&msg_queue, msgtbl, EXMSG_CNT, EXMSG_LOCKER);
    if (err < 0) {
        dbg_error(DBG_EXMSG, "init queue failed");
        return err;
    }

    err = mblock_init(&msg_block, msg_buffer, EXMSG_CNT, sizeof(exmsg_t), EXMSG_LOCKER);
    if (err < 0) {
        dbg_error(DBG_EXMSG, "init mblock failed");
        return err;
    }

    dbg_info(DBG_EXMSG, "init done.");
    return NET_ERR_OK;
}

static void work_thread(void *arg) {
    dbg_info(DBG_EXMSG, "exmsg is running....\n");

    while (1) {
        sys_sleep(100);
        exmsg_t *msg = (exmsg_t *)fixq_recv(&msg_queue, 0);
        if (!msg) {
            dbg_error(DBG_EXMSG, "recv msg error!");
        } else {
            printf("recv a msg (%p), type: %d, id: %d\n", msg, msg->type, msg->id);
        }
        mblock_free(&msg_block, msg);
    }
}

net_err_t exmsg_start(void) {
    sys_thread_t thread = sys_thread_create(work_thread, (void *)0);
    if (thread == SYS_THREAD_INVALID) {
        return NET_ERR_SYS;
    }
    return NET_ERR_OK;
}