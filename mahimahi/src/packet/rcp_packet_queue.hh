/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef RCP_PACKET_QUEUE_HH
#define RCP_PACKET_QUEUE_HH

#include <random>
#include <thread>
#include "dropping_packet_queue.hh"
#include <deque>
#include <vector>
#include <math.h>
#include <string>

class RCPPacketQueue : public DroppingPacketQueue
{
private:
    //This constant is copied from link_queue.hh.
    //It maybe better to get this in a more reliable way in the future.
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    //Configurable parameters
    uint32_t qdelay_ref_;

    //Internal parameters
    double alpha_, beta_;

    //Status variables
    std::deque<uint32_t> dq_queue;
    std::deque<uint32_t> real_dq_queue;
    std::deque<uint32_t> real_dq_queue_our_scheme;
    std::deque<uint32_t> eq_queue, rtt_avg;
    uint32_t avg_rtt, last_update;
    double target_rate;
    uint32_t mode; 
    //Perfect knwoledge

    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "rcp" };
        return type_;
    }

public:
    RCPPacketQueue( const std::string & args );

    void enqueue( QueuedPacket && p ) override;

    QueuedPacket dequeue( void ) override;
};

#endif