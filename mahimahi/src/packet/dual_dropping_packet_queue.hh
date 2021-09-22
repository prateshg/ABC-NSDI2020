/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef DUAL_DROPPING_PACKET_QUEUE_HH
#define DUAL_DROPPING_PACKET_QUEUE_HH

#include <queue>
#include <cassert>

#include "abstract_packet_queue.hh"
#include "exception.hh"

class DualDroppingPacketQueue : public AbstractPacketQueue
{
private:
    int queue_size_in_bytes_1_ = 0, queue_size_in_packets_1_ = 0;
    int queue_size_in_bytes_2_ = 0, queue_size_in_packets_2_ = 0;

    std::queue<QueuedPacket> internal_queue_1_ {};
    std::queue<QueuedPacket> internal_queue_2_ {};

    virtual const std::string & type( void ) const = 0;

protected:
    const unsigned int packet_limit_;
    const unsigned int byte_limit_;

    unsigned int size_bytes_1( void ) const;
    unsigned int size_packets_1( void ) const;
    unsigned int size_bytes_2( void ) const;
    unsigned int size_packets_2( void ) const;
    unsigned int size_bytes( void ) const;
    unsigned int size_packets( void ) const;

    /* put a packet on the back of the queue */
    void accept_1( QueuedPacket && p );
    void accept_2( QueuedPacket && p );

    /* are the limits currently met? */
    bool good( void ) const;
    bool good_with( const unsigned int size_in_bytes,
                    const unsigned int size_in_packets ) const;

    QueuedPacket dequeue_new( const unsigned int queue_no );

public:
    DualDroppingPacketQueue( const std::string & args );

    virtual void enqueue( QueuedPacket && p ) = 0;

    virtual QueuedPacket dequeue( void ) = 0;

    bool empty( void ) const override;

    std::string to_string( void ) const override;

    static unsigned int get_arg( const std::string & args, const std::string & name );
};

#endif /* DROPPING_PACKET_QUEUE_HH */ 
