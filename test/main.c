#include <stdio.h>
#include "sys_plat.h"

int main(void)
{
    pcap_t *pcap = pcap_device_open(netdev0_phy_ip, netdev0_hwaddr);
    if (pcap)
    {
        for (int counter = 0;;)
        {
            printf("begin test: %d\n", counter++);
            uint8_t buffer[1024] = {"hello world!"};
            // for (int i = 0; i < sizeof buffer; i++)
            // {
            //     buffer[i] = i;
            // }
            struct pcap_pkthdr *pkthdr;
            const uint8_t *pkt_data;
            if (pcap_next_ex(pcap, &pkthdr, &pkt_data) != 1)
            {
                continue;
            }

            int len = pkthdr->len > sizeof buffer ? sizeof buffer : pkthdr->len;
            memcpy(buffer, pkt_data, len);

            buffer[0] = 'h';
            buffer[1] = 'i';

            if (pcap_inject(pcap, buffer, sizeof buffer) == -1)
            {
                printf("pcap send: send packet failed %s \n", pcap_geterr(pcap));
                break;
            }
        }
    }
    else
    {
        //
    }

    return 0;
}