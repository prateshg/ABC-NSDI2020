#include <chrono>

#include "cellular_packet_queue.hh"
#include "timestamp.hh"
#include <iostream>

using namespace std;

CELLULARPacketQueue::CELLULARPacketQueue( const string & args )
  : DroppingPacketQueue(args),
    qdelay_ref ( get_arg( args, "qdelay_ref" ) ),
    delta ( get_arg( args,  "delta" ) ),
    dq_queue ( {0} ),
    link_capacity_queue ( {0} ),
    tokens (5),
    token_limit (5)
{
  if ( qdelay_ref == 0 || delta == 0) {
    throw runtime_error( "CELLULAR AQM queue must have qdelay_ref, delta" );
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
    link_capacity_queue.push_back(now);
    tokens += 0.49;
    return QueuedPacket("arbit", 0);
  }

  QueuedPacket ret = std::move( DroppingPacketQueue::dequeue () );
  struct iphdr *iph = (struct iphdr *)&ret.contents[4];
  if (((iph->tos & IPTOS_ECN_MASK) == IPTOS_ECN_NOT_ECT)) {
    return ret;
  }

  while(now - link_capacity_queue[0] > 20 && link_capacity_queue.size() > 1 && now > link_capacity_queue[1]) {
    link_capacity_queue.pop_front();
  }
  link_capacity_queue.push_back(now);
  while(now - dq_queue[0] > 20 && dq_queue.size() > 1 && now > dq_queue[1]) {
    dq_queue.pop_front();
  }
  dq_queue.push_back(now);


  double link_capacity_, observed_dq_rate_, target_rate_;
  link_capacity_ = (1.0 * (link_capacity_queue.size()-1))/20.0;
  observed_dq_rate_ = (1.0 * (dq_queue.size()-1))/(20.0);

  double current_qdelay_ = (size_packets() + 1) / link_capacity_;
  target_rate_ = 0.98 * observed_dq_rate_ + (link_capacity_ / delta) * min(0.0, (qdelay_ref - current_qdelay_));
  double credit_prob_ = (target_rate_ /  observed_dq_rate_) * 0.5;

  credit_prob_ = max(0.0, credit_prob_);
  credit_prob_ = min(1.0, credit_prob_);
  tokens += credit_prob_;

  if (tokens > 1) {
    if((iph->tos & IPTOS_ECN_MASK) != IPTOS_ECN_CE) {
      tokens -= 1;
    }
  } else {
    if(ret.contents.length() > 4) {
      iph->tos |= IPTOS_ECN_CE;
      iph->check = 0;
      iph->check = in_cksum ((unsigned short*) iph, iph->ihl<<2);
    }
  }

  if(tokens > token_limit) {
    tokens = token_limit;
  }  
  return ret;
}
