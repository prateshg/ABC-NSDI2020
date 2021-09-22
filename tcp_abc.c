
#include <linux/module.h>
#include <linux/mm.h>
#include <net/tcp.h>
#include <linux/net.h>
#include <linux/inet_diag.h>

struct abc {
	u32 prior_snd_una;
	u32 prior_rcv_nxt;
	u32 ce_state;
	u32 delayed_ack_reserved;
	u32 loss_cwnd;
	u32 ai_cwnd;
};

static struct tcp_congestion_ops abc_reno;

static void abc_init(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	tp->snd_ssthresh = TCP_INFINITE_SSTHRESH;
	if ((tp->ecn_flags & TCP_ECN_OK) ||
	    (sk->sk_state == TCP_LISTEN ||
	     sk->sk_state == TCP_CLOSE)) {
		struct abc *ca = inet_csk_ca(sk);

		ca->prior_snd_una = tp->snd_una;
		ca->prior_rcv_nxt = tp->rcv_nxt;

		ca->delayed_ack_reserved = 0;
		ca->loss_cwnd = tp->snd_cwnd;
		ca->ce_state = 0;
		ca->ai_cwnd = 0;
		cmpxchg(&sk->sk_pacing_status, SK_PACING_NONE, SK_PACING_NEEDED);
		return;
	}

	/* No ECN support? Fall back to Reno. Also need to clear
	 * ECT from sk since it is set during 3WHS for DCTCP.
	 */
	inet_csk(sk)->icsk_ca_ops = &abc_reno;
	INET_ECN_dontxmit(sk);
}

static u32 abc_ssthresh(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	//const struct abc *ca = inet_csk_ca(sk);
	return tp->snd_ssthresh;// ca->loss_cwnd;//max(tp->snd_cwnd - ((tp->snd_cwnd * ca->dctcpalpha) >> 11U), 2U);
}

/* Minimal DCTP CE state machine:
 *
 * S:	0 <- last pkt was non-CE
 *	1 <- last pkt was CE
 */

static void abc_ce_state_0_to_1(struct sock *sk)
{
	struct abc *ca = inet_csk_ca(sk);
	struct tcp_sock *tp = tcp_sk(sk);

	/* State has changed from CE=0 to CE=1 and delayed
	 * ACK has not sent yet.
	 */
	if (!ca->ce_state && ca->delayed_ack_reserved) {
		u32 tmp_rcv_nxt;

		/* Save current rcv_nxt. */
		tmp_rcv_nxt = tp->rcv_nxt;

		/* Generate previous ack with CE=0. */
		tp->ecn_flags &= ~TCP_ECN_DEMAND_CWR;
		tp->rcv_nxt = ca->prior_rcv_nxt;

		tcp_send_ack(sk);

		/* Recover current rcv_nxt. */
		tp->rcv_nxt = tmp_rcv_nxt;
	}

	ca->prior_rcv_nxt = tp->rcv_nxt;
	ca->ce_state = 1;

	tp->ecn_flags |= TCP_ECN_DEMAND_CWR;
}

static void abc_ce_state_1_to_0(struct sock *sk)
{
	struct abc *ca = inet_csk_ca(sk);
	struct tcp_sock *tp = tcp_sk(sk);

	/* State has changed from CE=1 to CE=0 and delayed
	 * ACK has not sent yet.
	 */
	if (ca->ce_state && ca->delayed_ack_reserved) {
		u32 tmp_rcv_nxt;

		/* Save current rcv_nxt. */
		tmp_rcv_nxt = tp->rcv_nxt;

		/* Generate previous ack with CE=1. */
		tp->ecn_flags |= TCP_ECN_DEMAND_CWR;
		tp->rcv_nxt = ca->prior_rcv_nxt;

		tcp_send_ack(sk);

		/* Recover current rcv_nxt. */
		tp->rcv_nxt = tmp_rcv_nxt;
	}

	ca->prior_rcv_nxt = tp->rcv_nxt;
	ca->ce_state = 0;

	tp->ecn_flags &= ~TCP_ECN_DEMAND_CWR;
}

static void abc_in_ack(struct sock *sk, u32 flags)
{
	struct tcp_sock *tp = tcp_sk(sk);
	
	//tp->ecn_flags |= TCP_ECN_QUEUE_CWR;
	struct abc *ca = inet_csk_ca(sk);
	u32 acked_bytes = tp->snd_una - ca->prior_snd_una;
	
	/* If ack did not advance snd_una, count dupack as MSS size.
	 * If ack did update window, do not count it at all.
	 */
	if (acked_bytes == 0 && !(flags & CA_ACK_WIN_UPDATE))
		acked_bytes = inet_csk(sk)->icsk_ack.rcv_mss;
	if (acked_bytes) {
		ca->ai_cwnd+=1;
		if(ca->ai_cwnd>ca->loss_cwnd) {
			ca->ai_cwnd-=ca->loss_cwnd;
			ca->loss_cwnd++;
		}
		ca->prior_snd_una = tp->snd_una;
		if (flags & CA_ACK_ECE) {
			if (ca->loss_cwnd>(5+acked_bytes/1448))
				ca->loss_cwnd-=acked_bytes/1448;
		} else {
				ca->loss_cwnd+=acked_bytes/1448;
		}
		tp->snd_cwnd = ca->loss_cwnd;
		printk(KERN_INFO "Ack Event %u\n", tp->snd_cwnd);
	}


}

static void abc_state(struct sock *sk, u8 new_state)
{
        switch (new_state) {
        case TCP_CA_Recovery:
            //printk(KERN_INFO "entered TCP_CA_Recovery (dupack drop) %u32\n", tp->snd_cwnd);
            break;
        case TCP_CA_Loss:
            //printk(KERN_INFO "entered TCP_CA_Loss (timeout drop)\n");
            break;
        case TCP_CA_CWR:
            //printk(KERN_INFO "entered TCP_CA_CWR (ecn drop)\n");
            break;
        default:
            //printk(KERN_INFO "entered TCP normal state\n");
            return;
    }
}

static void abc_update_ack_reserved(struct sock *sk, enum tcp_ca_event ev)
{
	struct abc *ca = inet_csk_ca(sk);

	switch (ev) {
	case CA_EVENT_DELAYED_ACK:
		if (!ca->delayed_ack_reserved)
			ca->delayed_ack_reserved = 1;
		break;
	case CA_EVENT_NON_DELAYED_ACK:
		if (ca->delayed_ack_reserved)
			ca->delayed_ack_reserved = 0;
		break;
	default:
		/* Don't care for the rest. */
		break;
	}
}

static void abc_cwnd_event(struct sock *sk, enum tcp_ca_event ev)
{	
	switch (ev) {
	case CA_EVENT_ECN_IS_CE:
		abc_ce_state_0_to_1(sk);
		break;
	case CA_EVENT_ECN_NO_CE:
		abc_ce_state_1_to_0(sk);
		break;
	case CA_EVENT_DELAYED_ACK:
	case CA_EVENT_NON_DELAYED_ACK:
		abc_update_ack_reserved(sk, ev);
		break;
	default:
		/* Don't care for the rest. */
		break;
	}
}


static u32 abc_cwnd_undo(struct sock *sk)
{
	const struct abc *ca = inet_csk_ca(sk);
	return ca->loss_cwnd;
}

void abc_cong_avoid(struct sock *sk, u32 ack, u32 acked)
{
	return;
}

void abc_cong_control(struct sock *sk, const struct rate_sample *rs) {
		return;
}

void abc_pkts_acked(struct sock *sk, const struct ack_sample *sample) {
	const struct abc *ca = inet_csk_ca(sk);
	if (sample->rtt_us>0) {
		sk->sk_pacing_rate = (ca->loss_cwnd*100)*(29000000/sample->rtt_us);
	}
}

struct tcp_congestion_ops abc = {
	.init		= abc_init,
	.in_ack_event   = abc_in_ack,
	.cwnd_event	= abc_cwnd_event,
	.ssthresh	= abc_ssthresh,
	.cong_avoid	= abc_cong_avoid,
	.cong_control = abc_cong_control,
	.pkts_acked = abc_pkts_acked,
	.undo_cwnd	= abc_cwnd_undo,
	.set_state	= abc_state,
	.flags		= TCP_CONG_NEEDS_ECN,
	.owner		= THIS_MODULE,
	.name		= "abc",
};

static struct tcp_congestion_ops abc_reno __read_mostly = {
	.ssthresh	= tcp_reno_ssthresh,
	.cong_avoid	= tcp_reno_cong_avoid,
	.undo_cwnd	= tcp_reno_undo_cwnd,
	//.get_info	= dctcp_get_info,
	.owner		= THIS_MODULE,
	.name		= "abc-reno",
};

static int __init abc_register(void)
{
	BUILD_BUG_ON(sizeof(struct abc) > ICSK_CA_PRIV_SIZE);
	return tcp_register_congestion_control(&abc);
}

static void __exit abc_unregister(void)
{
	tcp_unregister_congestion_control(&abc);
}

module_init(abc_register);
module_exit(abc_unregister);

MODULE_AUTHOR("Prateesh Goyal <prateesh@csail.mit.edu>");

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Accelerate Brake Control (ABC)");
