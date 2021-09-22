/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef CELLULAR_PACKET_QUEUE_HH
#define CELLULAR_PACKET_QUEUE_HH

#include <random>
#include <thread>
#include "dropping_packet_queue.hh"
#include <deque>
#include <vector>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <math.h>

class CELLULARPacketQueue : public DroppingPacketQueue
{
private:
    //This constant is copied from link_queue.hh.
    //It maybe better to get this in a more reliable way in the future.
    const static unsigned int PACKET_SIZE = 1504; /* default max TUN payload size */

    //Configurable parameters
    uint32_t qdelay_ref_;

    //Internal parameters
    double beta_;

    //Status variables
    std::deque<uint32_t> dq_queue;
    std::deque<uint32_t> real_dq_queue;
    double credits;

    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "cellular" };
        return type_;
    }

    /* set ip checksum of a given ip header*/
    /* Compute checksum for count bytes starting at addr, using one's complement of one's complement sum*/
    static unsigned short in_cksum(unsigned short *addr, unsigned int count) {
        register unsigned long sum = 0;
        while (count > 1) {
            sum += * addr++;
            count -= 2;
        }
        //if any bytes left, pad the bytes and add
        if(count > 0) {
            sum += ((*addr)&htons(0xFF00));
        }
        //Fold sum to 16 bits: add carrier to result
        while (sum>>16) {
            sum = (sum & 0xffff) + (sum >> 16);
        }
        //one's complement
        sum = ~sum;
        return ((unsigned short)sum);
    }

public:
    CELLULARPacketQueue( const std::string & args );

    void enqueue( QueuedPacket && p ) override;

    QueuedPacket dequeue( void ) override;
};

#endif