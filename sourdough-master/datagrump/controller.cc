#include <iostream>
#include <cstdlib>
#include "controller.hh"
#include "timestamp.hh"
#include <map>

#define MULT_DECREASE 4
#define ADD_INCREASE_DEFAULT 1

#define TRIGGER_LOW 49
#define TRIGGER_HIGH 90
#define TRIGGER_SUPER_HIGH 300
#define BETA 0.2
#define MULT_DEC_SCALE 1.2
#define ADD_INCR_SCALE 1.01
#define BETA_SCALE 1

using namespace std;
static unsigned int the_window_size = 13;
static double prev_rtt = 200;
static double rate = 13;
static double alpha = 0.2;
static double beta = BETA;
static double min_rtt = 10000;  
static double rtt_diff = 0; 
static double additive_increase = 1; 
static double mult_decrease = MULT_DECREASE; 
// static int add
// static int snowball_hi = 1;
// static int snowball_lo = 4;

static map <uint64_t, uint64_t> seq_cache;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
  cout << "TRIGGER_LOW " << TRIGGER_LOW
    << "\nTRIGGER_HIGH " << TRIGGER_HIGH
    << "\nTRIGGER_SUPER_HIGH " << TRIGGER_SUPER_HIGH 
    << "\nBETA " << BETA
    << "\nMULT_DEC_SCALE " << MULT_DEC_SCALE 
    << "\nADD_INCR_SCALE " << ADD_INCR_SCALE 
    << "\nBETA_SCALE " << BETA_SCALE << endl;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }
  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp, bool timeout_happened )
                                    /* in milliseconds */
{ 
  seq_cache[sequence_number] = send_timestamp;
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << "timeout was: " << timeout_happened <<endl;
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
  double new_rtt;
  uint64_t time_sent = seq_cache[sequence_number_acked];
  if (time_sent == 0) 
      new_rtt = prev_rtt;
  
  new_rtt =  timestamp_ack_received - time_sent;  
  seq_cache.erase(sequence_number_acked);

  // cout<< "new_rtt is: "<< new_rtt <<endl; 

  double new_rtt_diff = new_rtt - prev_rtt;
  // cout<< "new_rtt_diff is: "<< new_rtt_diff <<endl; 

  prev_rtt = new_rtt; 
  rtt_diff = ((1.0 - alpha) * rtt_diff) + (alpha * new_rtt_diff); 
  // cout<< "rtt_diff is: "<< rtt_diff <<endl;

  double normalized_gradient = rtt_diff / (min_rtt +1) ;

  // cout<< "normalized_gradient "<< normalized_gradient <<endl;

  if (new_rtt < TRIGGER_LOW) {
    rate += additive_increase;
    additive_increase *= ADD_INCR_SCALE; 
    mult_decrease = MULT_DECREASE; 
    beta = BETA;
    // cout<< "Trigger low" <<endl; 
  } 
  else if (new_rtt > TRIGGER_SUPER_HIGH){
    rate /= mult_decrease;
    //1.2 is best so far
    mult_decrease *= MULT_DEC_SCALE;
    if (beta < 0.9) beta *= BETA_SCALE;
  }
  else if(new_rtt > TRIGGER_HIGH){
    rate *= ( 1-beta *(1-(TRIGGER_HIGH/(1+new_rtt))));
    // cout<< "Trigger High" <<endl; 
    additive_increase = ADD_INCREASE_DEFAULT;
    mult_decrease = MULT_DECREASE; 
  }
  else if (normalized_gradient <= 0){
    double n = 0; 
    rate += (n * additive_increase);
    // cout<< "add increase mode" <<endl; 
    additive_increase = ADD_INCREASE_DEFAULT;
    mult_decrease = MULT_DECREASE; 
    beta = BETA;
  }
  else {
    // cout<< "Other" <<endl; 
    rate *= 1- (beta * normalized_gradient);
    additive_increase = ADD_INCREASE_DEFAULT;
    mult_decrease = MULT_DECREASE; 
    beta = BETA;
  }
  // cout<< "window size " << the_window_size<<endl; 
  the_window_size = rate  ;
  min_rtt = new_rtt < min_rtt? new_rtt : min_rtt;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
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
  return 32;
}
