#include <stdio.h>
#include "sys_plat.h"
#include "echo/tcp_echo_client.h"
#include "echo/tcp_echo_server.h"
#include "net.h"
#include "netif_pcap.h"
#include "dbg.h"
#include "nlist.h"
#include "mblock.h"
#include "pktbuf.h"

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

void pktbuf_test() {
    pktbuf_t *buf = pktbuf_alloc(2000);
    for (int i = 0; i < 10; i++) {
        pktbuf_add_header(buf, 37, 1);
    }
    for (int i = 0; i < 10; i++) {
        pktbuf_remove_header(buf, 37);
    }
    // pktbuf_add_header(buf, 300, 0);
    pktbuf_free(buf);

    // 大小的调整，先变大变小
    buf = pktbuf_alloc(0); // 大小为0
    pktbuf_resize(buf, 32);
    pktbuf_resize(buf, 288);
    pktbuf_resize(buf, 4922);
    pktbuf_resize(buf, 1921);
    pktbuf_resize(buf, 288);
    pktbuf_resize(buf, 32);
    pktbuf_resize(buf, 0);
    pktbuf_free(buf);

    buf = pktbuf_alloc(689);
    pktbuf_t *sbuf = pktbuf_alloc(892);
    pktbuf_join(buf, sbuf);
    pktbuf_free(buf);

    // 小包的连接测试并调整连续性.先合并一些小的包，以形成很多个小包的连接
    // 然后再调整连续性，可以使链的连接在不断变短
    buf = pktbuf_alloc(32);
    pktbuf_join(buf, pktbuf_alloc(4));
    pktbuf_join(buf, pktbuf_alloc(16));
    pktbuf_join(buf, pktbuf_alloc(54));
    pktbuf_join(buf, pktbuf_alloc(32));
    pktbuf_join(buf, pktbuf_alloc(38));
    pktbuf_set_cont(buf, 44);  // 合并成功，簇变短
    pktbuf_set_cont(buf, 60);  // 合并成功，簇变短
    pktbuf_set_cont(buf, 64);  // 合并成功，簇变短
    pktbuf_set_cont(buf, 128); // 合并成功，簇变短
    pktbuf_set_cont(buf, 135); // 失败，超过128
    pktbuf_free(buf);

    // 准备一些不同大小的包链，方便后面读写测试
    buf = pktbuf_alloc(32);
    pktbuf_join(buf, pktbuf_alloc(4));
    pktbuf_join(buf, pktbuf_alloc(16));
    pktbuf_join(buf, pktbuf_alloc(54));
    pktbuf_join(buf, pktbuf_alloc(32));
    pktbuf_join(buf, pktbuf_alloc(38));
    pktbuf_join(buf, pktbuf_alloc(512));

    static uint16_t temp[1000];
    static uint16_t read_temp[1000];

    // 初始化数据空间
    for (int i = 0; i < 1024; i++) {
        temp[i] = i;
    }
    // 读写测试。写超过1包的数据，然后读取
    pktbuf_reset_acc(buf);
    pktbuf_write(buf, (uint8_t *)temp, pktbuf_total(buf)); // 16位的读写
    memset(read_temp, 0, sizeof(read_temp));
    pktbuf_reset_acc(buf);
    pktbuf_read(buf, (uint8_t *)read_temp, pktbuf_total(buf));
    if (memcmp(temp, read_temp, pktbuf_total(buf)) != 0) {
        pktbuf_free(buf);
        printf("not equal.");
        exit(-1);
    }

    // 定位读写，不超过1个块
    memset(read_temp, 0, sizeof(read_temp));
    pktbuf_seek(buf, 18 * 2);
    pktbuf_read(buf, (uint8_t *)read_temp, 56);
    if (memcmp(temp + 18, read_temp, 56) != 0) {
        printf("not equal.");
        exit(-1);
    }

    // 定位跨一个块的读写测试, 从170开始读，读56
    memset(read_temp, 0, sizeof(read_temp));
    pktbuf_seek(buf, 85 * 2);
    pktbuf_read(buf, (uint8_t *)read_temp, 256);
    if (memcmp(temp + 85, read_temp, 256) != 0) {
        printf("not equal.");
        exit(-1);
    }

    // 数据的复制
    pktbuf_t *dst = pktbuf_alloc(1024);
    pktbuf_seek(buf, 200);      // 从200处开始读
    pktbuf_seek(dst, 600);      // 从600处开始写
    pktbuf_copy(dst, buf, 222); // 复制122个字节

    // 重新定位到600处开始读
    memset(read_temp, 0, sizeof(read_temp));
    pktbuf_seek(dst, 600);
    pktbuf_read(dst, (uint8_t *)read_temp, 222);   // 读222个字节
    if (memcmp(temp + 100, read_temp, 222) != 0) { // temp+100，实际定位到200字节偏移处
        printf("not equal.");
        exit(-1);
    }
    pktbuf_free(dst);

    pktbuf_reset_acc(buf);
    pktbuf_fill(buf, 53, pktbuf_total(buf));
    pktbuf_reset_acc(buf);
    memset(read_temp, 0, sizeof(read_temp));
    pktbuf_read(buf, (uint8_t *)read_temp, pktbuf_total(buf));

    pktbuf_free(buf); // 可以进去调试，在退出函数前看下所有块是否全部释放完毕
}

void basic_test() {
    nlist_test();
    mblock_test();
    pktbuf_test();
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