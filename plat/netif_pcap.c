#if defined(NET_DRIVER_PCAP)
#include "netif_pcap.h"
#include "net_plat.h"

void recv_thread(void *arg)
{
    plat_printf("recv thread is running...\n");

    while (1)
    {
        sys_sleep(100);
    }
}

void xmit_thread(void *arg)
{
    plat_printf("xmit thread is running...\n");

    while (1)
    {
        sys_sleep(100);
    }
}

net_err_t netif_pcap_open(void)
{
    sys_thread_create(recv_thread, (void *)0);
    sys_thread_create(xmit_thread, (void *)0);
    return NET_ERR_OK;
}

#endif