/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef WEIGHTED_PACKET_QUEUE_HH
#define WEIGHTED_PACKET_QUEUE_HH

#include <random>
#include <thread>
//#include "dropping_packet_queue.hh"
#include "dropping_packet_queue.hh"
#include "cellular_packet_queue.hh"
#include <deque>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <vector>
#include "drop_tail_packet_queue.hh"

class WEIGHTEDPacketQueue : public DroppingPacketQueue
{
private:
    //This constant is copied from link_queue.hh.
    //It maybe better to get this in a more reliable way in the future.
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    //Configurable parameters
    uint32_t qdelay_ref_;

    //Internal parameters
    double beta_;
    uint32_t mode;

    //Status variables
    std::deque<uint32_t> dq_queue;
    std::deque<uint32_t> real_dq_queue;
   	std::deque<uint32_t> eq_queue;
    double credits,weight,weight_credits;

    std::map<uint16_t,double> category1;
    std::map<uint16_t,double> category2;

    std::vector<double> zombie1;
    std::vector<double> zombie2;

    double actual_deque;

    DropTailPacketQueue queue1;
    //CELLULARPacketQueue queue1;
    //DropTailPacketQueue queue2;
    CELLULARPacketQueue queue2;
    //Perfect knwoledge

    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "weighted" };
        return type_;
    }

    inline void _parse_ports( const unsigned char *s, uint16_t *src, uint16_t *dst ) {
        *src = (s[0] << 8) | s[1];
        *dst = (s[2] << 8) | s[3];
    }

public:
    WEIGHTEDPacketQueue( const std::string & args );

    void enqueue( QueuedPacket && p ) override;

    QueuedPacket dequeue( void ) override;

    bool empty( void ) const override;
};

#endif