#include "ipaddr.h"

void ipaddr_set_any(ipaddr_t *ip) {
    ip->q_addr = 0;
}