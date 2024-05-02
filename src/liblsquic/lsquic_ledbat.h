
#ifndef LSQUIC_LEDBAT_H
#define LSQUIC_LEDBAT_H

struct lsquic_conn;

struct owd_circ_buf {
	uint32_t *buffer;
	u8 first;
	u8 next;
	u8 len;
	u8 min;
};



struct lsquic_ledbat{
	uint32_t last_rollover;
	uint32_t remote_hz;
	uint32_t remote_ref_time;
	uint32_t local_ref_time;
	uint32_t snd_cwnd_cnt;	
	uint32_t last_ack;
	struct owd_circ_buf base_history;
	struct owd_circ_buf noise_filter;
	uint32_t flag;
	const struct lsquic_conn
                   *cu_conn;            /* Used for logging */
    const struct lsquic_rtt_stats
                   *cu_rtt_stats;
    enum ledbat_flags{
    	LEDBAT_VALID_RHZ = (1 << 0),
		LEDBAT_VALID_OWD = (1 << 1),
		LEDBAT_INCREASING = (1 << 2),
		LEDBAT_CAN_SS = (1 << 3),
    } lb_flags;
}

LSQUIC_EXTERN const struct cong_ctl_if lsquic_cong_ledbat_if;