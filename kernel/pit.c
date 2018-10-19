#include "pit.h"

/**
 * Initialize PIT
 */
void initialize_PIT(WORD cnt, BOOL periodic)
{
  // Initialize the value in the PIT control register (port 0x43) to stop counting 
  // and set it to binary counter in mode 0
  out_port_byte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);
    
  // If the timer repeats at a fixed cycle, set to mode 2
  if (periodic == TRUE) {
    // PIT control register (port 0x43) set to binary counter in mode 2
    out_port_byte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
  }    
    
  // Set counter initial value in LSB -> MSB order in Counter 0 (Port 0x40)
  out_port_byte(PIT_PORT_COUNTER0, cnt);
  out_port_byte(PIT_PORT_COUNTER0, cnt >> 8);    
}

/**
 * Return current value of the counter0
 */
WORD read_counter0(void)
{
  BYTE high_byte, low_byte;
  WORD temp = 0;
    
  // Sends a latch command to the PIT control register (port 0x43) 
  // to read the current value at counter 0
  out_port_byte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);
    
  // Counter 0 (port 0x40) reads the counter value in LSB -> MSB order
  low_byte = in_port_byte(PIT_PORT_COUNTER0);
  high_byte = in_port_byte(PIT_PORT_COUNTER0);

  // Returns the sum of the read values in 16 bits 
  temp = high_byte;
  temp = (temp << 8) | low_byte;

  return temp;
}

/**
 * Wait for more than a certain time by setting count 0 directly
 */
void wait_using_directPIT(WORD cnt)
{
  WORD last_counter0;
  WORD curr_counter0;
    
  // Set the PIT controller to count from 0 to 0xFFFF repeatedly
  initialize_PIT(0, TRUE);
    
  // Wait until the counter excceed the cnt from now on
  last_counter0 = read_counter0();
  while (1) {
    // return current valute of counter0
    curr_counter0 = read_counter0();
    if (((last_counter0 - curr_counter0) & 0xFFFF) >= cnt) {
      break;
    }
  }
}
