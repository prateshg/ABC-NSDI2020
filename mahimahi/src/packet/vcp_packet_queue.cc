#include <chrono>

#include "vcp_packet_queue.hh"
#include "timestamp.hh"
#include <iostream>

using namespace std;

VCPPacketQueue::VCPPacketQueue( const string & args )
  : DroppingPacketQueue(args),
    dq_queue ( {0} ),
    real_dq_queue ( {0} ), 
    eq_queue ( {0} ),
    credits (5),
    last_update ( 0 ),
    mode ( 0 )
{
}

void VCPPacketQueue::enqueue( QueuedPacket && p )
{


  if ( ! good_with( size_bytes() + p.contents.size(),
        size_packets() + 1 ) ) {
    // Internal queue is full. Packet has to be dropped.
    return;
  } 

  accept( std::move( p ) );
  uint32_t now = timestamp();

  while(now-eq_queue[0]>200 && eq_queue.size()>1 && now>eq_queue[1]) {
    eq_queue.pop_front();
  }
  eq_queue.push_back(now);


  assert( good() );
}


QueuedPacket VCPPacketQueue::dequeue( void )
{
  uint32_t now = timestamp();
  while(now-real_dq_queue[0]>200 && real_dq_queue.size()>1 && now>real_dq_queue[1]) {
    real_dq_queue.pop_front();
  }
  real_dq_queue.push_back(now);
  
  if(size_packets()==0) {
    return QueuedPacket("arbit", 0);
  }

  QueuedPacket ret = std::move( DroppingPacketQueue::dequeue () );

  double pl;
  pl = (eq_queue.size() + 0.5*size_packets())/(0.98*real_dq_queue.size());
  if(now-last_update>200) {
	  last_update=now;
	  if(pl<0.8) {
		  mode=0;
	  } else if(pl>1.0) {
		  mode=2;
		  credits -= 0.125*(0.5*dq_queue.size()+size_packets());
	  } else {
		  mode=1;
	  }
  }
  uint32_t mode_drop_sensitive=mode;
  if(mode==2 && (now-last_update)>100) {
    mode_drop_sensitive=1;
  }
  uint32_t incoming_mode;
  if (ret.contents[ret.contents.size()-1] == (4 + ret.contents[ret.contents.size()-3])) {
    incoming_mode=2;
  } else if (ret.contents[ret.contents.size()-1] == (2 + ret.contents[ret.contents.size()-3])) {
    incoming_mode=1;
  } else {
    incoming_mode=0;
  }

  mode_drop_sensitive = max(incoming_mode, mode_drop_sensitive);
    ret.contents[ret.contents.size() - 1] += (mode_drop_sensitive-incoming_mode);
    ret.contents[ret.contents.size() - 3] -= (mode_drop_sensitive-incoming_mode);
  return ret;
}
