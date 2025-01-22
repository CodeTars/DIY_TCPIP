#include <stdio.h>
#include "sys_plat.h"
#include "echo/tcp_echo_client.h"
#include "echo/tcp_echo_server.h"
#include "net.h"
#include "netif_pcap.h"
#include "dbg.h"
#include "nlist.h"
#include "mblock.h"

static sys_sem_t sem;
static int count = 0;
static sys_mutex_t mutex;
static uint8_t buffer[100];

net_err_t netdev_init() {
    netif_pcap_open();
    return NET_ERR_OK;
}

typedef struct _tnode_t {
    int id;
    nlist_node_t node;
} tnode_t;

void nlist_test() {
#define NODE_CNT 4

    tnode_t node[NODE_CNT];
    nlist_t list;
    nlist_node_t *p;

    nlist_init(&list);

    plat_printf("insert first\n");
    for (int i = 0; i < NODE_CNT; i++) {
        node[i].id = i;
        nlist_insert_first(&list, &node[i].node);
    }
    nlist_for_each(p, &list) {
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }

    plat_printf("remove first\n");
    for (int i = 0; i < NODE_CNT; i++) {
        nlist_node_t *p = nlist_remove_first(&list);
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }

    plat_printf("insert last\n");
    for (int i = 0; i < NODE_CNT; i++) {
        node[i].id = i;
        nlist_insert_last(&list, &node[i].node);
    }
    nlist_for_each(p, &list) {
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }

    plat_printf("remove last\n");
    for (int i = 0; i < NODE_CNT; i++) {
        nlist_node_t *p = nlist_remove_last(&list);
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }

    plat_printf("insert after\n");
    for (int i = 0; i < NODE_CNT; i++) {
        nlist_insert_after(&list, nlist_first(&list), &node[i].node);
    }
    nlist_for_each(p, &list) {
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }
}

void mblock_test() {
    uint8_t buffer[10 * 100];
    mblock_t mblock;
    mblock_init(&mblock, buffer, 10, 100, NLOCKER_THREAD);

    void *temp[10];
    for (int i = 0; i < 10; i++) {
        temp[i] = mblock_alloc(&mblock, 0);
        printf("block: %p, free count: %d\n", temp[i], mblock_free_cnt(&mblock));
    }

    for (int i = 0; i < 10; i++) {
        mblock_free(&mblock, temp[i]);
        printf("free count: %d\n", mblock_free_cnt(&mblock));
    }

    mblock_destroy(&mblock);
}

void basic_test() {
    nlist_test();
    mblock_test();
}

int main(void) {
    // // tcp_echo_client_start(friend0_ip, 5000);
    // // tcp_echo_server_start(6000);

    // #define DBG_TEST DBG_LEVEL_INFO
    // dbg_info(DBG_TEST, "info");
    // dbg_warning(DBG_TEST, "warning");
    // dbg_error(DBG_TEST, "error");
    // dbg_assert(1 + 1 == 2, "failed");
    // dbg_assert(1 + 1 == 3, "failed");

    net_init();

    basic_test();

    netdev_init();

    net_start();

    while (1) {
        sys_sleep(100);
    }

    return 0;
}