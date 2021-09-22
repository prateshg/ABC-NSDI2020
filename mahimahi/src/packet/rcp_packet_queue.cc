#include <chrono>

#include "rcp_packet_queue.hh"
#include "timestamp.hh"
#include <iostream>

using namespace std;

RCPPacketQueue::RCPPacketQueue( const string & args )
  : DroppingPacketQueue(args),
    qdelay_ref_ ( get_arg( args, "qdelay_ref" ) ),
    alpha_ ( get_arg( args,  "alpha" ) / 100.0),
    beta_ ( get_arg( args,  "beta" ) / 100.0),
    dq_queue ( {0} ),
    real_dq_queue ( {0} ),
    real_dq_queue_our_scheme ( {0} ), 
    eq_queue ( {0} ),
    rtt_avg ( {100} ),
    avg_rtt (100), 
    last_update (0),
    target_rate (0.5),
    mode ( get_arg( args,  "mode" ) )
{
  if ( qdelay_ref_ == 0 || alpha_==0 || beta_==0 || mode==0) {
    throw runtime_error( "RCP AQM queue must have qdelay_ref, alpha, beta, mode" );
  }
}

void RCPPacketQueue::enqueue( QueuedPacket && p )
{

  uint32_t now = timestamp();

  while((now-eq_queue[0])>100 && eq_queue.size()>1 && now>eq_queue[1]) {
    eq_queue.pop_front();
   }
  eq_queue.push_back(now);

  if ( ! good_with( size_bytes() + p.contents.size(),
        size_packets() + 1 ) ) {
    // Internal queue is full. Packet has to be dropped.
    return;
  } 

  accept( std::move( p ) );


  assert( good() );
}


QueuedPacket RCPPacketQueue::dequeue( void )
{
  uint32_t now = timestamp();
  while(now-real_dq_queue[0]>100 && real_dq_queue.size()>1 && now>real_dq_queue[1]) {
    real_dq_queue.pop_front();
  }
  real_dq_queue.push_back(now);
  
  while(now-real_dq_queue_our_scheme[0]>20 && real_dq_queue_our_scheme.size()>1 && now>real_dq_queue_our_scheme[1]) {
    real_dq_queue_our_scheme.pop_front();
  }
  real_dq_queue_our_scheme.push_back(now);

  if(size_packets()==0) {
    return QueuedPacket("arbit", 0);
  }

  QueuedPacket ret = std::move( DroppingPacketQueue::dequeue () );

  while(now-dq_queue[0]>avg_rtt && dq_queue.size()>1 && now>dq_queue[1]) {
    dq_queue.pop_front();
    rtt_avg.pop_front();
  }
  dq_queue.push_back(now);
  rtt_avg.push_back(100+(now-ret.arrival_time));
  
  if (now> (last_update+100)) {
    avg_rtt=0;
    for(unsigned int i=0;i<rtt_avg.size();i++) {
      avg_rtt+=rtt_avg[i];
    }
    avg_rtt/=dq_queue.size();
     if (mode==3) {
      //rcp
      double eta=0.95;
      double real_dq_rate_ = (1.0 * (real_dq_queue.size()-1))/(100.0);
      double eq_rate_ = (1.0 * (eq_queue.size()-1))/(100.0);
      target_rate = target_rate *(1.0 + (100.0/avg_rtt)*(((alpha_*(eta*real_dq_rate_ - eq_rate_)) - (beta_ * ((size_packets()) / (1.0*avg_rtt))))/(eta*real_dq_rate_)));
      target_rate=max(0.05, target_rate);
    }
    last_update=now;
  }
  if (mode==1) {
      double real_dq_rate_ = (1.0 * (real_dq_queue_our_scheme.size()-1))/(20.0);
      double current_qdelay = (size_packets() + 1) / ((real_dq_queue_our_scheme.size()-1)/(20.0));
      double delta = 100.0;
      target_rate = 0.98*real_dq_rate_ + beta_ * (real_dq_rate_ / delta) * min(0.0, (qdelay_ref_ - current_qdelay));
      target_rate=max(0.01, target_rate);
  }
  
  if(ret.contents.size()<100) {
	  return ret;
  }
  int target_pkts=target_rate*1000;
  //cout<<target_pkts<<endl;
  int current_pkts=0;

  for(int i=0;i<20;i++) {
    int bit=0;
    if ((ret.contents[ret.contents.size() - (i*3 + 1)] != ret.contents[ret.contents.size() - ((i+1)*3)])) {
      bit=0;
    } else {
      bit=1;
    }
    current_pkts += bit*pow(2,i);
  }

  if (target_pkts<current_pkts) {
    for(int i=0;i<20;i++) {
      int bit=target_pkts%2;
      target_pkts/=2;

      if(bit==1) {
	while(ret.contents[ret.contents.size() - (i*3 + 1)] != ret.contents[ret.contents.size() - ((i+1)*3)]) {
        ret.contents[ret.contents.size() - (i*3 + 1)] -= 1; 
	ret.contents[ret.contents.size() - ((i+1)*3)] += 1;
	}
      } else {
        ret.contents[ret.contents.size() - (i*3 + 1)] += 1; 
        ret.contents[ret.contents.size() - ((i+1)*3)] -= 1; 
      }
    }
  }

  return ret;
}
