#include <chrono>

#include "cellular_packet_queue.hh"
#include "timestamp.hh"
#include <iostream>

using namespace std;

CELLULARPacketQueue::CELLULARPacketQueue( const string & args )
  : DroppingPacketQueue(args),
    qdelay_ref_ ( get_arg( args, "qdelay_ref" ) ),
    beta_ ( get_arg( args,  "beta" ) / 100.0),
    dq_queue ( {0} ),
    real_dq_queue ( {0} ),
    credits (5)
{
  if ( qdelay_ref_ == 0 || beta_==0) {
    throw runtime_error( "CELLULAR AQM queue must have qdelay_ref, beta" );
  }
}

void CELLULARPacketQueue::enqueue( QueuedPacket && p )
{


  if ( ! good_with( size_bytes() + p.contents.size(),
        size_packets() + 1 ) ) {
    // Internal queue is full. Packet has to be dropped.
    return;
  }
  accept( std::move( p ) );
  assert( good() );
}


QueuedPacket CELLULARPacketQueue::dequeue( void )
{
  uint32_t now = timestamp();

  if (empty()) {
    real_dq_queue.push_back(now);
    credits += 0.49;
    return QueuedPacket("arbit", 0);
  }

  QueuedPacket ret = std::move( DroppingPacketQueue::dequeue () );
  struct iphdr *iph = (struct iphdr *)&ret.contents[4];
  if (((iph->tos & IPTOS_ECN_MASK) == IPTOS_ECN_NOT_ECT)) {
    return ret;
  }

  while(now - real_dq_queue[0] > 20 && real_dq_queue.size() > 1 && now > real_dq_queue[1]) {
    real_dq_queue.pop_front();
  }
  real_dq_queue.push_back(now);
  while(now - dq_queue[0] > 20 && dq_queue.size() > 1 && now > dq_queue[1]) {
    dq_queue.pop_front();
  }
  dq_queue.push_back(now);

  double delta = 100.0;

  double real_dq_rate_, observed_dq_rate_, target_rate;
  real_dq_rate_ = (1.0 * (real_dq_queue.size()-1))/20.0;
  observed_dq_rate_ = (1.0 * (dq_queue.size()-1))/(20.0);

  double current_qdelay = (size_packets() + 1) / real_dq_rate_;
  target_rate = 0.98 * observed_dq_rate_ + beta_ * (real_dq_rate_ / delta) * min(0.0, (qdelay_ref_ - current_qdelay));
  double credit_prob_ = (target_rate /  observed_dq_rate_) * 0.5;

  credit_prob_ = max(0.0, credit_prob_);
  credit_prob_ = min(1.0, credit_prob_);
  credits += credit_prob_;

  if (credits > 1) {
    if((iph->tos & IPTOS_ECN_MASK) != IPTOS_ECN_CE) {
      credits -= 1;
    }
  } else {
    if(ret.contents.length() > 4) {
      iph->tos |= IPTOS_ECN_CE;
      iph->check = 0;
      iph->check = in_cksum ((unsigned short*) iph, iph->ihl<<2);
    }
  }

  if(credits > 5) {
    credits = 5;
    //credits *= exp(2*((ewma_time-now)/delta));
  }  
  return ret;
}
