#include <chrono>

#include "xcp_packet_queue.hh"
#include "timestamp.hh"
#include <iostream>
#include <math.h>
using namespace std;

XCPPacketQueue::XCPPacketQueue( const string & args )
  : DroppingPacketQueue(args),
    alpha_ ( get_arg( args,  "alpha" ) / 100.0),
    beta_ ( get_arg( args,  "beta" ) / 100.0),
    dq_queue ( {0} ),
    dq_queue2 ( {0} ),
    real_dq_queue ( {0} ), 
    eq_queue ( {0} ),
    rtt_avg ( {100} ),
    qsize_avg ( {0} ),
    avg_rtt (100),
    avg_qsize (0),
    last_update (0),
    eta (0.5)
{
  if (alpha_==0 || beta_==0) {
    throw runtime_error( "XCP AQM queue must have qdelay_ref, alpha, beta, mode" );
  }
}

void XCPPacketQueue::enqueue( QueuedPacket && p )
{
  uint32_t now = timestamp();
  while(now-eq_queue[0]>avg_rtt && eq_queue.size()>1 && now>eq_queue[1]) {
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


QueuedPacket XCPPacketQueue::dequeue( void )
{
  uint32_t now = timestamp();
  while(now-real_dq_queue[0]>avg_rtt && real_dq_queue.size()>1 && now>real_dq_queue[1]) {
    real_dq_queue.pop_front();
  }
  real_dq_queue.push_back(now);

  if(size_packets()==0) {
    return QueuedPacket("arbit", 0);
  }

  QueuedPacket ret = std::move( DroppingPacketQueue::dequeue () );
  while(now-dq_queue[0]>avg_rtt && dq_queue.size()>1 && now>dq_queue[1]) {
    dq_queue.pop_front();
    rtt_avg.pop_front();
  }
  while(now-dq_queue2[0]>100 && dq_queue2.size()>1 && now>dq_queue2[1]) {
    dq_queue2.pop_front();
    qsize_avg.pop_front();
  }
  dq_queue.push_back(now);
  dq_queue2.push_back(now);
  rtt_avg.push_back(100+(now-ret.arrival_time));
  qsize_avg.push_back(size_packets());
  
  //if (now> (last_update+avg_rtt)) {
      avg_rtt=0;
      for(unsigned int i=0;i<rtt_avg.size();i++) {
        avg_rtt+=rtt_avg[i];
      }
      avg_rtt/=dq_queue.size();
      avg_qsize=qsize_avg[0];
      for(unsigned int i=0;i<qsize_avg.size();i++) {
        if(qsize_avg[i]<avg_qsize) {
          avg_qsize=qsize_avg[i];
        }
      }
      double delta=avg_rtt;
      double real_dq_rate_ = (1.0 * (real_dq_queue.size()-1))/(avg_rtt);
      double eq_rate_ = (1.0 * (eq_queue.size()-1))/(avg_rtt);
      double phi=alpha_*delta*(real_dq_rate_ - eq_rate_) - beta_* avg_qsize;
      eta=phi/((double) dq_queue.size());
      last_update=now;
  //}
  if(ret.contents.size()<300) {
	  return ret;
  }
  int target_pkts=int(eta*1000);
  int incoming_target_pkts=0.0;
  for (int i=0;i<20;i++) {
    if (ret.contents[ret.contents.size() - ((1+i)*3 + 1)] == ret.contents[ret.contents.size() - ((1+i)*3 + 3)]) {
      incoming_target_pkts += pow(2,i);
    }
  }
  if (ret.contents[ret.contents.size()-(1)] != ret.contents[ret.contents.size()-(3)])
    incoming_target_pkts*=-1;
  target_pkts=min(target_pkts,incoming_target_pkts);
  if (target_pkts<0) {
    ret.contents[ret.contents.size() - (1)] += 1; 
    ret.contents[ret.contents.size() - ((1)*3)] -= 1; 
    target_pkts*=-1;
  }

  for(int j=0;j<20;j++) {
    int i=j+1;
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

  return ret;
}
