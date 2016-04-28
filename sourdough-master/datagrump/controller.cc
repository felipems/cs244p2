#include <iostream>
#include "controller.hh"
#include "timestamp.hh"
#include <map>

#define MULT_DECREASE 2
#define ADD_INCREASE 2 
#define RTT_MAX_THRESHOLD 200
#define RTT_MIN_THRESHOLD 50

using namespace std;
unsigned int the_window_size;
map<uint64_t, uint64_t> datagram_sent;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
  debug_ = true;
  the_window_size = 13;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Add datagram to the map. */
  datagram_sent.insert(pair<uint64_t, uint64_t>(sequence_number, send_timestamp));

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
             /* when the ack was received (by sender) */
{
  /* Calculate round trip time. */
  uint64_t ack_time = timestamp_ack_received - send_timestamp_acked;

  /* If sequence number not in map, we already handled the case, 
     duplicate ack. */
  if (datagram_sent.count(sequence_number_acked) <= 0) {
    return;
  }

  uint64_t datagram_sent_timestamp = datagram_sent.at(sequence_number_acked);
  uint64_t rtt = ack_time + (recv_timestamp_acked - datagram_sent_timestamp);

  /* Adjust the window if thresholds are reached. */
  /*
  if (rtt > RTT_MAX_THRESHOLD)
    the_window_size -= ADD_INCREASE;
  else if (rtt < RTT_MIN_THRESHOLD)
    the_window_size += ADD_INCREASE;
    */

  datagram_sent.erase(sequence_number_acked);

  if ( debug_ ) {
    cerr << "RTT Time: " << rtt
   << "Ack time: " << ack_time
   << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
