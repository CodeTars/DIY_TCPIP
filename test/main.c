#include <stdio.h>
#include "sys_plat.h"
#include "echo/tcp_echo_client.h"
#include "echo/tcp_echo_server.h"
#include "net.h"
#include "netif_pcap.h"
#include "dbg.h"
#include "nlist.h"

static sys_sem_t sem;
static int count = 0;
static sys_mutex_t mutex;
static uint8_t buffer[100];
static int write_index, read_index;
static sys_sem_t read_sem, write_sem;

/**
 * 线程1
 */
void thread1_entry(void *arg)
{
    for (int i = 0; i < 2 * sizeof(buffer); i++)
    {
        buffer[write_index++] = i;
        sys_sem_notify(read_sem);
        sys_sem_wait(write_sem, 0);
        if (write_index == sizeof(buffer))
        {
            write_index = 0;
        }
        printf("thread 1 write data: %d\n", i);
    }

    while (1)
    {
        plat_printf("this is thread 1: %s\n", (char *)arg);
        sys_sleep(1000);
        sys_sem_notify(sem);
        sys_sleep(1000);
    }
}

/**
 * 线程2
 */
void thread2_entry(void *arg)
{
    for (int i = 0; i < 2 * sizeof(buffer); i++)
    {
        sys_sem_wait(read_sem, 0);

        uint8_t data = buffer[read_index++];
        sys_sem_notify(write_sem);
        if (read_index == sizeof(buffer))
        {
            read_index = 0;
        }
        printf("thread 2 read data: %d\n", data);
        sys_sleep(200);
    }
    while (1)
    {
        sys_sem_wait(sem, 0);
        plat_printf("this is thread 2: %s\n", (char *)arg);
    }
}

net_err_t netdev_init()
{
    netif_pcap_open();
    return NET_ERR_OK;
}

typedef struct _tnode_t
{
    int id;
    nlist_node_t node;
} tnode_t;

void nlist_test()
{
#define NODE_CNT 4

    tnode_t node[NODE_CNT];
    nlist_t list;
    nlist_node_t *p;

    nlist_init(&list);

    plat_printf("insert first\n");
    for (int i = 0; i < NODE_CNT; i++)
    {
        node[i].id = i;
        nlist_insert_first(&list, &node[i].node);
    }
    nlist_for_each(p, &list)
    {
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }

    plat_printf("remove first\n");
    for (int i = 0; i < NODE_CNT; i++)
    {
        nlist_node_t *p = nlist_remove_first(&list);
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }

    plat_printf("insert last\n");
    for (int i = 0; i < NODE_CNT; i++)
    {
        node[i].id = i;
        nlist_insert_last(&list, &node[i].node);
    }
    nlist_for_each(p, &list)
    {
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }

    plat_printf("remove last\n");
    for (int i = 0; i < NODE_CNT; i++)
    {
        nlist_node_t *p = nlist_remove_last(&list);
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }

    plat_printf("insert after\n");
    for (int i = 0; i < NODE_CNT; i++)
    {
        nlist_insert_after(&list, nlist_first(&list), &node[i].node);   
    }
    nlist_for_each(p, &list)
    {
        tnode_t *tnode = nlist_entry(p, tnode_t, node);
        printf("id = %d\n", tnode->id);
    }
}

void basic_test()
{
    nlist_test();
}

int main(void)
{
    // sem = sys_sem_create(0);
    // read_sem = sys_sem_create(0);
    // write_sem = sys_sem_create(sizeof(buffer));
    // // mutex = sys_mutex_create();
    // // sys_thread_create(thread1_entry, "AAAA");
    // // sys_thread_create(thread2_entry, "BBBB");

    // // tcp_echo_client_start(friend0_ip, 5000);
    // // tcp_echo_server_start(6000);

#define DBG_TEST DBG_LEVEL_INFO
    // dbg_info(DBG_TEST, "info");
    // dbg_warning(DBG_TEST, "warning");
    // dbg_error(DBG_TEST, "error");
    // dbg_assert(1 + 1 == 2, "failed");
    // dbg_assert(1 + 1 == 3, "failed");

    // net_init();

    // netdev_init();

    // net_start();

    basic_test();

    return 0;
}