#include "fixq.h"
#include "dbg.h"

net_err_t fixq_init(fixq_t *q, void **buf, int size, nlocker_type_t type)
{
    q->size = size;
    q->cnt = 0;
    q->in = 0;
    q->out = 0;
    q->buf = (void **)0;
    q->send_sem = SYS_SEM_INVALID;
    q->recv_sem = SYS_SEM_INVALID;
    net_err_t err = nlocker_init(&q->locker, type);
    if (err < 0)
    {
        dbg_error(DBG_FIXQ, "locker init failed");
        return err;
    }

    q->send_sem = sys_sem_create(size);
    if (q->send_sem == SYS_SEM_INVALID)
    {
        dbg_error(DBG_FIXQ, "create sem failed");
        nlocker_destroy(&q->locker);
        return NET_ERR_SYS;
    }

    q->recv_sem = sys_sem_create(0);
    if (q->recv_sem == SYS_SEM_INVALID)
    {
        dbg_error(DBG_FIXQ, "create sem failed");
        sys_sem_free(q->send_sem);
        nlocker_destroy(&q->locker);
        return NET_ERR_SYS;
    }

    q->buf = buf;
    return NET_ERR_OK;
}

net_err_t fixq_send(fixq_t *q, void *msg, int tmo)
{
    nlocker_lock(&q->locker);
    if (tmo < 0 && q->cnt >= q->size)
    {
        nlocker_unlock(&q->locker);
        return NET_ERR_FULL;
    }
    nlocker_unlock(&q->locker);

    if (sys_sem_wait(q->send_sem, tmo) < 0)
    {
        return NET_ERR_TMO;
    }
    nlocker_lock(&q->locker);
    q->buf[q->in++] = msg;
    if (q->in == q->size)
    {
        q->in = 0;
    }
    q->cnt++;
    nlocker_unlock(&q->locker);

    sys_sem_notify(q->recv_sem);
    return NET_ERR_OK;
}

void *fixq_recv(fixq_t *q, int tmo)
{
    nlocker_lock(&q->locker);
    if (tmo < 0 && q->cnt == 0)
    {
        nlocker_unlock(&q->locker);
        return (void *)0;
    }
    nlocker_unlock(&q->locker);

    if (sys_sem_wait(q->recv_sem, tmo) < 0)
    {
        return (void *)0;
    }

    nlocker_lock(&q->locker);
    void *msg = q->buf[q->out++];
    if (q->out == q->size)
    {
        q->out = 0;
    }
    q->cnt--;
    nlocker_unlock(&q->locker);

    sys_sem_notify(q->send_sem);
    return msg;
}

void fixq_destroy(fixq_t * q)
{
    nlocker_destroy(&q->locker);
    sys_sem_free(q->send_sem);
    sys_sem_free(q->recv_sem);
}

int fixq_count(fixq_t *q)
{
    nlocker_lock(&q->locker);
    int cnt = q->cnt;
    nlocker_unlock(&q->locker);
    return cnt;
}