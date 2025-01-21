#include "tcp_echo_server.h"
#include <WinSock2.h>
#include "sys_plat.h"

void tcp_echo_server_start(int port) {
    printf("tcp server start, port: %d\n", port);

    WSADATA wsdata;
    WSAStartup(MAKEWORD(2, 2), &wsdata);

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        printf("open socket error");
        goto end;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(s, &server_addr, sizeof server_addr) < 0) {
        printf("connect error");
        goto end;
    }

    listen(s, 5);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof client_addr;
        SOCKET client = accept(s, &client_addr, &addr_len);
        if (client < 0) {
            printf("accept error");
            break;
        }

        printf("tcp echo server, connect ip : %s, port : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        char buffer[128];
        ssize_t size;
        while ((size = recv(client, buffer, sizeof buffer, 0)) > 0) {
            printf("recv bytes : %d\n", (int)size);
            send(client, buffer, size, 0);
        }

        closesocket(client);
    }

end:
    closesocket(s);
}