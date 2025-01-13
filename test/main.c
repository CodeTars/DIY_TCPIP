#include <stdio.h>
#include "sys_plat.h"

int main(void)
{
    pcap_t *pcap = pcap_device_open(netdev0_phy_ip, netdev0_hwaddr);
    if (pcap)
    {
        for (int counter = 0;; counter++)
        {
            printf("begin test: %d\n", counter);
            uint8_t buffer[1024] = {"hello world!"};
            // for (int i = 0; i < sizeof buffer; i++)
            // {
            //     buffer[i] = i;
            // }
            if (pcap_inject(pcap, buffer, sizeof buffer) == -1)
            {
                printf("pcap send: send packet failed %s \n", pcap_geterr(pcap));
                break;
            }
            sys_sleep(10);
        }
    }
    else
    {
        //
    }

    return 0;
}