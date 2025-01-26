#ifndef NET_ERR_H
#define NET_ERR_H

typedef enum _net_err_t {
    NET_ERR_OK = 0,
    NET_ERR_SYS = -1,
    NET_ERR_FULL = -2,
    NET_ERR_TMO = -3,
    NET_ERR_MEM = -4,
    NET_ERR_SIZE = -5,
} net_err_t;

#endif