#include "tcp_echo_client.h"
#include "sys_plat.h"
#include <WinSock2.h>

/**
 * tcp回显客户端
 */
int tcp_echo_client_start(const char *ip, int port)
{
    printf("tcp echo client, ip : %s, port : %d\n", ip, port);

    WSADATA wsdata;
    WSAStartup(MAKEWORD(2, 2), &wsdata);

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        printf("tcp echo client : open socket error\n");
        goto end;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if (connect(s, (const struct sockaddr *)&server_addr, sizeof server_addr) < 0)
    {
        printf("connect error\n");
        goto end;
    }

    char buffer[128];
    printf(">>");
    while (fgets(buffer, sizeof buffer, stdin))
    {
        if (send(s, buffer, sizeof buffer - 1, 0) <= 0)
        {
            printf("write error\n");
            goto end;
        }
        memset(buffer, 0, sizeof buffer);
        int len = recv(s, buffer, sizeof buffer - 1, 0);
        if (len <= 0)
        {
            printf("read error\n");
            goto end;
        }
        printf("%s\n", buffer);
        printf(">>");
    }

end:
    return -1;
}