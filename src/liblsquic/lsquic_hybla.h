/* Copyright (c) 2024 - Mostafa Karami, Colin Finkbeiner, Mohamed E. Najd */
/* Code Available under MIT License */
/*
 * lsquic_hybla.h -- hybla congestion control protocol.
 */

#ifndef LSQUIC_HYBLA_H
#define LSQUIC_HYBLA_H

#include "lsquic_shared_support.h"

struct lsquic_conn;

struct lsquic_hybla{
    bool  hybla_en;
    uint32_t   snd_cwnd_cents; /* Keeps increment values when it is <1, <<7 */
    uint32_t  rho;        /* Rho parameter, integer part  */
    uint32_t   rho2;       /* Rho * Rho, integer part */
    uint32_t   rho_3ls;        /* Rho parameter, <<3 */
    uint32_t   rho2_7ls;       /* Rho^2, <<7 */
    uint32_t   minrtt_us;      /* Minimum smoothed round trip time value seen */

    unsigned long   hy_cwnd;
    unsigned long   hy_tcp_cwnd;
    unsigned long   hy_ssthresh;


    const struct lsquic_conn
                   *cu_conn;            /* Used for logging */
    const struct lsquic_rtt_stats
                   *cu_rtt_stats;
}

LSQUIC_EXTERN const struct cong_ctl_if lsquic_cong_hybla_if;

#endif