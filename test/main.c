#include <stdio.h>
#include "sys_plat.h"
#include "echo/tcp_echo_client.h"
#include "echo/tcp_echo_server.h"
#include "net.h"
#include "netif_pcap.h"

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

    net_init();

    netdev_init();

    net_start();

    while (1)
    {
        sys_sleep(100);
    }

    return 0;
}