#ifndef NETIF_H
#define NETIF_H
#include "net_cfg.h"
#include "ipaddr.h"
#include "fixq.h"

typedef struct _netif_hwaddr_t {
    uint8_t len;
    uint8_t addr[NETIF_HWADDR_SIZE];
} netif_hwaddr_t;

// 网络接口类型
typedef enum _netif_type_t {
    NETIF_TYPE_NONE = 0,
    NETIF_TYPE_ETHER,
    NETIF_TYPE_LOOP,

    NETIF_TYPE_SIZE,
} netif_type_t;

typedef struct _netif_t {
    char name[NETIF_NAME_SIZE];

    netif_hwaddr_t hwaddr;
    ipaddr_t ipaddr;
    ipaddr_t netmask;
    ipaddr_t gateway;

    enum {
        NETIF_CLOSED,
        NETIF_OPENED,
        NETIF_ACTIVE,
    } state;

    netif_type_t type;
    int mtu;

    nlist_node_t node;

    fixq_t in_q;
    void *in_q_buf[NETIF_INQ_SIZE];
    fixq_t out_q;
    void *out_q_buf[NETIF_OUTQ_SIZE];

} netif_t;

#endif