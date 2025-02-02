#include "ipaddr.h"

void ipaddr_set_any(ipaddr_t *ip) {
    ip->q_addr = 0;
}

net_err_t ipaddr_from_str(ipaddr_t *dst, const char *src) {
    if (!dst || !src) {
        return NET_ERR_PARAM;
    }

    dst->type = IPADDR_V4;
    dst->q_addr = 0;

    char c;
    uint8_t now = 0;
    uint8_t *p = dst->a_addr;
    while (c = *src++) {
        if (c >= '0' && c <= '9') {
            now = now * 10 + (c - '0');
        } else if (c == '.') {
            *p++ = now;
            now = 0;
        } else {
            return NET_ERR_PARAM;
        }
    }

    *p++ = now;
    if (p > dst->a_addr + 4) {
        return NET_ERR_PARAM;
    }
    return NET_ERR_OK;
}