#include <iostream>
#include <cstdlib>
#include "controller.hh"
#include "timestamp.hh"
#include <map>

#define MULT_DECREASE 2
#define ADD_INCREASE_DEFAULT 1

#define TRIGGER_LOW 49
#define TRIGGER_HIGH 95

#define ALPHA 0.5 
#define BETA 0.4 




typedef struct packet{
  struct packet * next; 
  struct packet * prev;
  uint64_t sequence_number;
  uint64_t send_timestamp; 

}packet;

using namespace std;
static unsigned int the_window_size = 13;
static double prev_rtt = 200;
static double rate = 1;
static double alpha = 0.2;
static double beta = BETA;
static double min_rtt = 10000;  
static double rtt_diff = 0; 
static double additive_increase = 1; 
// static int add
// static int snowball_hi = 1;
// static int snowball_lo = 4;

static map <uint64_t, uint64_t> seq_cache;





/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
  // debug_ = true; 
  
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
// static void insert_packet(packet *new_packet){

//   if ( packets == NULL ){
//     packets = new_packet;  
//   }
//   else{
//     packets->prev = new_packet;
//     new_packet->next = packets; 
//     packets = new_packet;   
//   } 
// }
/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp, bool timeout_happened )
                                    /* in milliseconds */
{ 
  seq_cache[sequence_number] = send_timestamp;
  // if( timeout_happened){
  //   the_window_size = (the_window_size/MULT_DECREASE) +1; 
  //   // cout << "timeout!!" <<endl; 

  // }
  // else if( (rand()% 1000 + 1) == 7) {

  //    the_window_size += ADD_INCREASE_DEFAULT;
  // }
  /* Default: take no action */
  // packet* new_packet = ( packet * )malloc(sizeof(packet));
  // new_packet->sequence_number = sequence_number;
  // new_packet->send_timestamp = send_timestamp; 
  // new_packet->prev =  NULL; 
  // new_packet->next = NULL;
  // insert_packet(new_packet);

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << "timeout was: " << timeout_happened <<endl;

  }
}
// static packet* remove_from_ll(uint64_t sequence_number_acked, int &found, int &smaller_found){

//   packet* found_packet = NULL; 

//   for(packet* ptr = packets; ptr != NULL;  ptr = ptr->next){
//     if( ptr->sequence_number < sequence_number_acked ){
//         smaller_found++;
//     }
//     else if(ptr->sequence_number == sequence_number_acked){

//       found_packet = ptr; 
//       found++;

//       if(ptr->next == NULL && ptr->prev == NULL){
//         packets = NULL;
//         return found_packet; 
//       }

//       else if(ptr->prev == NULL){
//         packets = ptr->next;
//         ptr->next->prev = NULL;
//       }

//       else if(ptr->next == NULL){
//         ptr->prev->next = NULL;
//         return found_packet;
//       }

//       else
//       {
//         ptr->prev->next = ptr->next; 
//         ptr->next->prev = ptr->prev;
//       }
//       // printf("%s\n", "Checked");
//     }
//   }
//   return found_packet; 
// }
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

  uint64_t time_sent = seq_cache[sequence_number_acked];
  if (time_sent == 0) return;   

  seq_cache.erase(sequence_number_acked);

  double new_rtt =  timestamp_ack_received - time_sent; 

  // cout<< "new_rtt is: "<< new_rtt <<endl; 

  

  double new_rtt_diff = new_rtt - prev_rtt;
  // cout<< "new_rtt_diff is: "<< new_rtt_diff <<endl; 

  prev_rtt = new_rtt; 
  rtt_diff = ((1.0 - alpha) * rtt_diff) + (alpha * new_rtt_diff); 
  // cout<< "rtt_diff is: "<< rtt_diff <<endl;

  double normalized_gradient = rtt_diff / (min_rtt +1) ;

  // cout<< "normalized_gradient "<< normalized_gradient <<endl;

  if(new_rtt < TRIGGER_LOW){
    rate+=additive_increase;
    additive_increase *= 1.01; 
    // cout<< "Trigger low" <<endl; 
  } 
  else if(new_rtt > TRIGGER_HIGH){
    rate *= ( 1-beta *(1-(TRIGGER_HIGH/(1+new_rtt))));
    // cout<< "Trigger High" <<endl; 
    additive_increase = ADD_INCREASE_DEFAULT;
  }
  else if (normalized_gradient <= 0){
    double n = 0; 
    rate += (n * additive_increase);
    // cout<< "add increase mode" <<endl; 
    additive_increase = ADD_INCREASE_DEFAULT;
  }
  else {
    // cout<< "Other" <<endl; 
    rate *= 1- (beta * normalized_gradient);
    additive_increase = ADD_INCREASE_DEFAULT;
  }
  // cout<< "window size " << the_window_size<<endl; 
  the_window_size = rate  ;
  min_rtt = new_rtt < min_rtt? new_rtt : min_rtt;
  // // cout << "averageRTT: " << avg_rtt << "rtt "<< rtt << endl; 
  // // cout<< "WZP: " << the_window_size<< endl; 
  // if(avg_rtt - rtt  < TRIGGER_LOW ){
  //   the_window_size -= snowball_lo;

  //   if(the_window_size > 10000) the_window_size = 13; 

  //   snowball_lo *=4; 
  //   snowball_hi /= 2; 
  //    // cout << "going low"<< endl; 
  //    // cout<< "WZ: " << the_window_size<< endl; 

  // }
  // else if (avg_rtt - rtt > TRIGGER_HIGH  ){
  //   the_window_size += snowball_hi;
  //   snowball_hi *=1.2; 
  //   // snowball_hi /= 2; 
  //     snowball_lo /= 2; 
  //   // cout << "going high"<< endl;
  //   // cout<< "WZ: " << the_window_size << endl; 
  // } 
  // else{
  //     snowball_hi /= 2; 
  //     snowball_lo /= 2; 
  //     // snowball++; 
  //     snowball_hi++;
  //     snowball_lo++;
  // }
  // avg_rtt = (avg_rtt + rtt )/2; 
  // timestamp_ack_received
  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
  /* Default: take no action */
  // int found = 0; 
  // int smaller_found =0; 

  
  // packet* pkt = remove_from_ll(sequence_number_acked, found, smaller_found);

  // if(pkt && timestamp_ack_received - pkt->send_timestamp   > timeout_ms()){
  //   the_window_size /=2;
  // //    cout<< "MULT_DECREASE!" <<endl;
  // }
  // else{
  //   the_window_size += ADD_INCREASE_DEFAULT;
  // }


  // if(found == 0 && smaller_found)
    // window_size stays the same
  // if(found > 0 && smaller_found ){
  //    the_window_size /=2;
  //    cout<< "MULT_DECREASE!" <<endl;
  // }

  // else if (found > 0 && smaller_found == 0)
  // {
  //   the_window_size += ADD_INCREASE_DEFAULT; 
  // }
  
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 50;
}
