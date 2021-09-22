#include <chrono>

#include "weighted_packet_queue.hh"
#include "timestamp.hh"
#include <stdlib.h>
#include <time.h>
#include <algorithm>

using namespace std;

WEIGHTEDPacketQueue::WEIGHTEDPacketQueue( const string & args )
  : DroppingPacketQueue(args),
    qdelay_ref_ ( get_arg( args, "qdelay_ref" ) ),
    beta_ ( get_arg( args,  "beta" ) / 100.0),
    mode ( get_arg( args, "mode") ),
    dq_queue ( {0} ),
    real_dq_queue ( {0} ), 
    eq_queue ( {0} ),
    credits (5),
    weight (0.5),
    weight_credits (5),
    category1 (std::map<uint16_t, double> ()),
    category2 (std::map<uint16_t, double> ()),
    zombie1 ({1.0}),
    zombie2 ({1.0}),
    actual_deque (0.5),
    //queue1 (CELLULARPacketQueue(args)),
    queue1 (DropTailPacketQueue(args)),
    queue2 (CELLULARPacketQueue(args))
    //queue2 (DropTailPacketQueue(args))
{
  if ( qdelay_ref_ == 0 || beta_==0) {
    throw runtime_error( "WEIGHTED AQM queue must have qdelay_ref, beta" );
  }
  srand(time(NULL));
  //cout<<"vcvx"<<endl;
}

void WEIGHTEDPacketQueue::enqueue( QueuedPacket && p )
{
unsigned int M=1000;
double prob=0.25;
double alpha=prob/M;
uint16_t src,dst;
  _parse_ports((const unsigned char *) p.contents.substr(24,4).c_str(), &src, &dst);

  if(src<5200) {
    if(category1.find(src)==category1.end()) {
      category1[src]=alpha;
    } else {
      category1[src]+=alpha;
    }
    zombie1[0]=(1-alpha)*zombie1[0];
    int random=rand()%M+1;
    if(zombie1.size()<=M) {
      zombie1.push_back(src);
      zombie1[0] += alpha;
    } else if ((uint16_t) zombie1[random]==src){
       zombie1[0] += alpha;
    } else if ((rand()%M)< (M*prob)) {
        zombie1[random]=src;
    }
    queue1.enqueue((QueuedPacket &&) p);
    //accept_1( std::move( p ) );
  } else {
    if(category2.find(src)==category2.end()) {
      category2[src]=alpha;
    } else {
      category2[src]+=alpha;
    }

    zombie2[0]=(1-alpha)*zombie2[0];
    int random=rand()%M+1;
    if(zombie2.size()<=M) {
      zombie2.push_back(src);
      zombie2[0] += alpha;
    } else if ((uint16_t) zombie2[random]==src){
      zombie2[0] += alpha;
    } else if ((rand()%M)< (M*prob)) {
        zombie2[random]=src;
    }
    queue2.enqueue((QueuedPacket &&) p);
    //accept_2( std::move( p ) );
  }


  assert( good() );
}


QueuedPacket WEIGHTEDPacketQueue::dequeue( void )
{
  if(empty()) {
    return queue2.dequeue();
    //return QueuedPacket("arbit", 0);
  }
  unsigned int M=1000;
  double prob=0.25;
  double alpha=prob/M;

  for(map<uint16_t, double>::iterator it=category1.begin();it!=category1.end();++it) {
    it->second*=(1-alpha);
  }  
  for(map<uint16_t, double>::iterator it=category2.begin();it!=category2.end();++it) {
    it->second*=(1-alpha);
  }
  QueuedPacket ret=QueuedPacket("arbit", 0);

  auto x1 = max_element(category1.begin(), category1.end(),
    [](const pair<uint16_t,double>& p1, const pair<uint16_t,double>& p2) {
        return p1.second < p2.second; });
  auto x2 = max_element(category2.begin(), category2.end(),
    [](const pair<uint16_t,double>& p1, const pair<uint16_t,double>& p2) {
        return p1.second < p2.second; });

  vector<double> v3, v4;
  double actual_deque_overall = 0.0;
  for(map<uint16_t, double>::iterator it=category1.begin();it!=category1.end();++it) {
    actual_deque_overall+=it->second;
    if(it->first<5197)
	v3.push_back(it->second);
    else
	v3.push_back(it->second*1.1);
  }
  for(map<uint16_t, double>::iterator it=category2.begin();it!=category2.end();++it) {
    actual_deque_overall+=it->second;
    if(it->first<5197)
	v3.push_back(it->second);
    else
	v3.push_back(it->second*1.1);
  }
  sort(v3.begin(),v3.end());
  v4.push_back(0.0);
  for(unsigned int i=0;i<v3.size();i++) {
    v4.push_back(v4[i]+v3[i]);
  }
  
  
  double alpha_water=0;
  for(unsigned int i=0;i<v3.size();i++){
    if((v4[i]+v3[i]*(v3.size()-i))>actual_deque_overall) {
        alpha_water=(actual_deque_overall-v4[i])/(v3.size()-i);
        break;
    }
    alpha_water=v3[i];
  }
  double rand1,rand2;
  rand1=rand2=0.0;
  for(map<uint16_t, double>::iterator it=category1.begin();it!=category1.end();++it) {
    if(it->first<5197) 
	rand1+=it->second;
    else
      rand1+=min(it->second*1.1,alpha_water);
  }
  for(map<uint16_t, double>::iterator it=category2.begin();it!=category2.end();++it) {
    if(it->first<5197) 
	rand2+=it->second;
    else
    rand2+=min(it->second*1.1,alpha_water);
  }
  
  double N1=max(1.0,1/zombie1[0]);
  double N2=max(1.0,1/zombie2[0]);
  double throughput1=weight/(N1);
  double throughput2=(1-weight)/(N2);

  int weighted_mode=1;
  if (weighted_mode==0) {
    weight = weight + 0.001*((x2->second-x1->second)*1.0)/(x1->second+x2->second);
  } else if (weighted_mode==1) {
    weight = rand1/(rand1+rand2);
    if (rand()%1000>990) {
      cout<<alpha_water<<" "<<rand1<<" "<<rand2<<endl;
      cout<<v3[v3.size()-6]<<" "<<v3[v3.size()-1]<<" "<<v3[v3.size()-2]<<" "<<v3[v3.size()-3]<<" "<<v3[v3.size()-4]<<" "<<v3[v3.size()-5]<<" "<<v4[v4.size()-7]<<" "<<actual_deque_overall<<endl;
  }
  } else {
    weight = weight + 0.001*((throughput2-throughput1)*1.0)/(throughput2+throughput1);
    if (rand()%1000==454) {
      cout<<N1<<" "<<N2<<endl;
    }
  }
  weight=max(0.8*actual_deque,weight);
  weight=min(1.2*actual_deque,weight);
  weight=max(0.05,weight);
  weight=min(0.95,weight);
  weight_credits+=weight;
  actual_deque*=(1-alpha);
  //cout<<weight<<endl;
  int random=rand()%1000;
  if(queue2.empty() || (random<weight*1000 && !queue1.empty())) {
    weight_credits-=1;
    ret=queue1.dequeue();
    //ret = std::move( DualDroppingPacketQueue::dequeue_new (1) );
    actual_deque += alpha;
    if(queue2.empty() && !(random<weight*1000)) {
      queue2.dequeue();
    }
  } else {
    ret=queue2.dequeue();
    //ret = std::move( DualDroppingPacketQueue::dequeue_new (2) );
  }
  weight_credits=max(-10.0, weight_credits);
  weight_credits=min(10.0, weight_credits);
  return ret;
}

bool WEIGHTEDPacketQueue::empty( void ) const
{
    return queue1.empty() && queue2.empty();
}
