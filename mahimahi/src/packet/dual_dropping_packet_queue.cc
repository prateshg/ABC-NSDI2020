/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include "dual_dropping_packet_queue.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

DualDroppingPacketQueue::DualDroppingPacketQueue( const string & args )
    : packet_limit_( get_arg( args, "packets" ) ),
      byte_limit_( get_arg( args, "bytes" ) )
{
    if ( packet_limit_ == 0 and byte_limit_ == 0 ) {
        throw runtime_error( "Dropping queue must have a byte or packet limit." );
    }
}

QueuedPacket DualDroppingPacketQueue::dequeue_new( const unsigned int queue_no )
{
    if(queue_no==1) {
        assert( not internal_queue_1_.empty() );

        QueuedPacket ret = std::move( internal_queue_1_.front() );
        internal_queue_1_.pop();

        queue_size_in_bytes_1_ -= ret.contents.size();
        queue_size_in_packets_1_--;

        assert( good() );

        return ret;
    } else {
        assert( not internal_queue_2_.empty() );

        QueuedPacket ret = std::move( internal_queue_2_.front() );
        internal_queue_2_.pop();

        queue_size_in_bytes_2_ -= ret.contents.size();
        queue_size_in_packets_2_--;

        assert( good() );

        return ret;
    }
}

bool DualDroppingPacketQueue::empty( void ) const
{
    return internal_queue_1_.empty() && internal_queue_2_.empty();
}

bool DualDroppingPacketQueue::good_with( const unsigned int size_in_bytes,
                                     const unsigned int size_in_packets ) const
{
    bool ret = true;

    if ( byte_limit_ ) {
        ret &= ( size_in_bytes <= byte_limit_ );
    }

    if ( packet_limit_ ) {
        ret &= ( size_in_packets <= packet_limit_ );
    }

    return ret;
}

bool DualDroppingPacketQueue::good( void ) const
{
    return good_with( size_bytes_1(), size_packets_1() ) && good_with( size_bytes_2(), size_packets_2() );;
}

unsigned int DualDroppingPacketQueue::size_bytes_1( void ) const
{
    assert( queue_size_in_bytes_1_ >= 0 );
    return unsigned( queue_size_in_bytes_1_ );
}

unsigned int DualDroppingPacketQueue::size_packets_1( void ) const
{
    assert( queue_size_in_packets_1_ >= 0 );
    return unsigned( queue_size_in_packets_1_ );
}

unsigned int DualDroppingPacketQueue::size_bytes_2( void ) const
{
    assert( queue_size_in_bytes_2_ >= 0 );
    return unsigned( queue_size_in_bytes_2_ );
}

unsigned int DualDroppingPacketQueue::size_packets_2( void ) const
{
    assert( queue_size_in_packets_2_ >= 0 );
    return unsigned( queue_size_in_packets_2_ );
}

unsigned int DualDroppingPacketQueue::size_packets( void ) const
{
    return size_packets_1() + size_packets_2();
}

unsigned int DualDroppingPacketQueue::size_bytes( void ) const
{
    return size_bytes_1() + size_bytes_2();
}

/* put a packet on the back of the queue */
void DualDroppingPacketQueue::accept_1( QueuedPacket && p )
{
    queue_size_in_bytes_1_ += p.contents.size();
    queue_size_in_packets_1_++;
    internal_queue_1_.emplace( std::move( p ) );
}

void DualDroppingPacketQueue::accept_2( QueuedPacket && p )
{
    queue_size_in_bytes_2_ += p.contents.size();
    queue_size_in_packets_2_++;
    internal_queue_2_.emplace( std::move( p ) );
}

string DualDroppingPacketQueue::to_string( void ) const
{
    string ret = type() + " [";

    if ( byte_limit_ ) {
        ret += string( "bytes=" ) + ::to_string( byte_limit_ );
    }

    if ( packet_limit_ ) {
        if ( byte_limit_ ) {
            ret += ", ";
        }

        ret += string( "packets=" ) + ::to_string( packet_limit_ );
    }

    ret += "]";

    return ret;
}

unsigned int DualDroppingPacketQueue::get_arg( const string & args, const string & name )
{
    auto offset = args.find( name );
    if ( offset == string::npos ) {
        return 0; /* default value */
    } else {
        /* extract the value */

        /* advance by length of name */
        offset += name.size();

        /* make sure next char is "=" */
        if ( args.substr( offset, 1 ) != "=" ) {
            throw runtime_error( "could not parse queue arguments: " + args );
        }

        /* advance by length of "=" */
        offset++;

        /* find the first non-digit character */
        auto offset2 = args.substr( offset ).find_first_not_of( "0123456789" );

        auto digit_string = args.substr( offset ).substr( 0, offset2 );

        if ( digit_string.empty() ) {
            throw runtime_error( "could not parse queue arguments: " + args );
        }

        return myatoi( digit_string );
    }
}
