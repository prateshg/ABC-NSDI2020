/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef VCP_PACKET_QUEUE_HH
#define VCP_PACKET_QUEUE_HH

#include <random>
#include <thread>
#include "dropping_packet_queue.hh"
#include <deque>
#include <vector>


class VCPPacketQueue : public DroppingPacketQueue
{
private:
    //This constant is copied from link_queue.hh.
    //It maybe better to get this in a more reliable way in the future.
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    //Status variables
    std::deque<uint32_t> dq_queue;
    std::deque<uint32_t> real_dq_queue;
   	std::deque<uint32_t> eq_queue;
    double credits;
    //Perfect knwoledge
    uint32_t last_update, mode;
    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "vcp" };
        return type_;
    }

public:
    VCPPacketQueue( const std::string & args );

    void enqueue( QueuedPacket && p ) override;

    QueuedPacket dequeue( void ) override;
};

#endif