#include "netif.h"
#include "mblock.h"
#include "dbg.h"

static netif_t netif_buffer[NETIF_DEV_CNT];
static mblock_t netif_mblock;
static nlist_t netif_list;
static netif_t *netif_default;

net_err_t netif_init(void) {
    dbg_info(DBG_NETIF, "init netif");

    nlist_init(&netif_list);
    mblock_init(&netif_mblock, (void *)netif_buffer, NETIF_DEV_CNT, sizeof(netif_t), NLOCKER_NONE);

    netif_default = (netif_t *)0;

    dbg_info(DBG_NETIF, "init done.");
    return NET_ERR_OK;
}

netif_t *netif_open(const char *dev_name) {
    netif_t *netif = (netif_t *)mblock_alloc(&netif_mblock, -1);
    if (!netif) {
        dbg_error(DBG_NETIF, "no netif");
        return netif;
    }

    ipaddr_set_any(&netif->ipaddr);
    ipaddr_set_any(&netif->netmask);
    ipaddr_set_any(&netif->gateway);
    netif->mtu = 0;
    netif->type = NETIF_TYPE_NONE;
    nlist_node_init(&netif->node);

    strncpy(netif->name, dev_name, NETIF_NAME_SIZE);
    netif->name[NETIF_NAME_SIZE - 1] = '\0';

    net_err_t err = fixq_init(&netif->in_q, netif->in_q_buf, NETIF_INQ_SIZE, NLOCKER_THREAD);
    if (err < 0) {
        dbg_error(DBG_NETIF, "netif in_q init error, err: %d", err);
        return (netif_t *)0;
    }

    err = fixq_init(&netif->out_q, netif->out_q_buf, NETIF_OUTQ_SIZE, NLOCKER_THREAD);
    if (err < 0) {
        fixq_destroy(&netif->in_q);
        dbg_error(DBG_NETIF, "netif in_q init error, err: %d", err);
        return (netif_t *)0;
    }

    netif->state = NETIF_OPENED;
    nlist_insert_last(&netif_list, &netif->node);

    return netif;
}
