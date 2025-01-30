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