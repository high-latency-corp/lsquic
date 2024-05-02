
#include "lsquic_hybla.h"







static void
lsquic_hybla_init(void *cong_ctl, const struct lsquic_conn_public *conn_pub,
                                            enum quic_ft_bit UNUSED_retx_frames)



{
	struct lsquic_hybla * const hybla = cong_ctl;
	hybla->rho = 0;
	hybla->rho2 = 0;
	hybla->rho_3ls = 0;
	hybla->rho2_7ls = 0;
	hybla->snd_cwnd_cents = 0;
	hybla->hybla_en = true;
	hybla->hy_rtt_stats = &conn_pub->rtt_stats;
	hybla->minrtt_us = &conn_pub->min_rtt;

}

static inline uint32_t hybla_fraction(uint32_t odds)
{
	static const uint32_t fractions[] = {
		128, 139, 152, 165, 181, 197, 215, 234,
	};

	return (odds < ARRAY_SIZE(fractions)) ? fractions[odds] : 128;
}


static void
lsquic_hybla_cleanup (void *cong_ctl)
{
}

static inline void hybla_recalc_param (struct lsquic_hybla* hybla)
{
	lsquic_time_t srtt = lsquic_rtt_stats_get_srtt(hybla->hy_rtt_stats);

	ca->rho_3ls = max_t(u32,
			    srtt / (rtt0 * USEC_PER_MSEC),
			    8U);
	ca->rho = ca->rho_3ls >> 3;
	ca->rho2_7ls = (ca->rho_3ls * ca->rho_3ls) << 1;
	ca->rho2 = ca->rho2_7ls >> 7;
}



static uint64_t
lsquic_cubic_get_cwnd (void *cong_ctl)
{
    struct lsquic_hybla *const hybla = cong_ctl;
    return hybla->hy_cwnd;
}

static int in_slow_start(void *cong_ctl){
	struct lsquic_hybla *const hybla = cong_ctl;
	return hybla->hy_cwnd < hybla->hy_ssthresh; 
}


static void hybla_cong_avoid(void*cong_ctl, u32 ack, u32 acked)
{
	struct lsquic_hybla *const hybla = cong_ctl;
	u32 increment, odd, rho_fractions;
	int is_slowstart = 0;
	lsquic_time_t srtt = lsquic_rtt_stats_get_srtt(hybla->hy_rtt_stats);
	
	/*  Recalculate rho only if this srtt is the lowest */
	if (srtt < hybla->minrtt_us) {
		hybla_recalc_param(hybla);
		hybla->minrtt_us = hybla->srtt_us;
	}


	if (ca->rho == 0)
		hybla_recalc_param(hybla);

	rho_fractions = ca->rho_3ls - (ca->rho << 3);

	if (in_slow_start(cong_ctl)) {
		/*
		 * slow start
		 *      INC = 2^RHO - 1
		 * This is done by splitting the rho parameter
		 * into 2 parts: an integer part and a fraction part.
		 * Inrement<<7 is estimated by doing:
		 *	       [2^(int+fract)]<<7
		 * that is equal to:
		 *	       (2^int)	*  [(2^fract) <<7]
		 * 2^int is straightly computed as 1<<int,
		 * while we will use hybla_slowstart_fraction_increment() to
		 * calculate 2^fract in a <<7 value.
		 */
		is_slowstart = 1;
		increment = ((1 << min(hybla->rho, 16U)) *
			hybla_fraction(rho_fractions)) - 128;
	} else {
		/*
		 * congestion avoidance
		 * INC = RHO^2 / W
		 * as long as increment is estimated as (rho<<7)/window
		 * it already is <<7 and we can easily count its fractions.
		 */
		increment = hybla->rho2_7ls / hybla->hy_cwnd(tp);
		if (increment < 128)
			hybla->hy_cwnd++;
	}

	odd = increment % 128;
	hybla->hy_cwnd +=  (increment >> 7);
	hybla->snd_cwnd_cents += odd;

	/* check when fractions goes >=128 and increase cwnd by 1. */
	while (ca->snd_cwnd_cents >= 128) {
		hybla->hy_cwnd ++;
		hybla->snd_cwnd_cents -= 128;
		hybla->snd_cwnd_cnt = 0;
	}
	/* check when cwnd has not been incremented for a while */
	if (increment == 0 && odd == 0 && hybla->snd_cwnd_cnt >= tcp_snd_cwnd(tp)) {
			hybla->hy_cwnd ++;
		tp->snd_cwnd_cnt = 0;
	}
	/* clamp down slowstart cwnd to ssthresh value. */
	if (is_slowstart)
		hybla->snd_cwnd_cnt = min(hybla->snd_cwnd_cnt , tp->snd_ssthresh);

	hybla->snd_cwnd_cnt = min(hybla->snd_cwnd_cnt, tp->snd_cwnd_clamp);
}



const struct cong_ctl_if lsquic_cong_hybla_if =
{
    .cci_ack           = lsquic_hybla_ack,
    .cci_cleanup       = lsquic_hybla_cleanup,
    .cci_get_cwnd      = lsquic_hybla_get_cwnd,
    .cci_init          = lsquic_hybla_init,
    .cci_pacing_rate   = lsquic_hybla_pacing_rate,
    .cci_loss          = lsquic_hybla_loss,
    .cci_reinit        = lsquic_hybla_init,
    .cci_timeout       = lsquic_hybla_timeout,
    .cci_was_quiet     = lsquic_hybla_was_quiet,
};
