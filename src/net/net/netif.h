#ifndef NETIF_H
#define NETIF_H
#include "net_cfg.h"
#include "ipaddr.h"
#include "fixq.h"
#include "nlist.h"

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

struct _netif_t;
typedef struct _netif_ops_t {
    net_err_t (*open)(struct _netif_t *netif, void *data);
    void (*close)(struct _netif_t *netif);
    net_err_t (*xmit)(struct _netif_t *netif);
} netif_ops_t;

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

    const netif_ops_t *ops;
    void *ops_data;

    nlist_node_t node;

    fixq_t in_q;
    void *in_q_buf[NETIF_INQ_SIZE];
    fixq_t out_q;
    void *out_q_buf[NETIF_OUTQ_SIZE];

} netif_t;

net_err_t netif_init(void);
netif_t *netif_open(const char *dev_name, const netif_ops_t *driver, void *driver_data);
net_err_t netif_set_addr(netif_t *netif, ipaddr_t *ip, ipaddr_t *netmask, ipaddr_t *gateway);
net_err_t netif_set_hwaddr(netif_t *netif, const uint8_t *hwaddr, int len);

#endif