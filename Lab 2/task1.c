// Michael Fiscus
// EE/CSE 474
// Spring 2020
// Lab 2 task 1_1

// This program uses timers to cycle through the on-board LEDs in a waterfall
// fashion

#include <stdint.h>
#include "lab_2_header.h"

#define LED_F 0x11
#define LED_N 0x3

void GPIO_init(void);
void Timer_init(void);

int main ( void )
{
  GPIO_init();
  Timer_init();
  
  enum S_states {D1, D2, D3, D4} S_state;
  // Cycle through individual on-board LEDs
  while (1) {
    
    switch (S_state) {          // Transitions
      case D1:
        if ((GPTMRIS_TIMER_0 & 0x1) == 1) {
          S_state = D2;
          GPTMICR_TIMER_0 |= 0x1;
        }
        else {
          S_state = D1;
        }
        break;
      case D2:
        if ((GPTMRIS_TIMER_0 & 0x1) == 1) {
          GPTMICR_TIMER_0 |= 0x1;
          S_state = D3;
        }
        else {
          S_state = D2;
        }
        break;
      case D3:
        if ((GPTMRIS_TIMER_0 & 0x1) == 1) {
          GPTMICR_TIMER_0 |= 0x1;
          S_state = D4;
        }
        else {
          S_state = D3;
        }
        break;
      case D4:
        if ((GPTMRIS_TIMER_0 & 0x1) == 1) {
          GPTMICR_TIMER_0 |= 0x1;
          S_state = D1;
        }
        else {
          S_state = D4;
        }
        break;
    }
    
    switch (S_state) {          // Actions
      case D1:
        GPIO_PORT_F_DATA = 0x0;
        GPIO_PORT_N_DATA = 0x2;
        break;
      case D2:
        GPIO_PORT_N_DATA = 0x1;
        break;
      case D3:
        GPIO_PORT_F_DATA = 0x10;
        GPIO_PORT_N_DATA = 0x0;
        break;
      case D4:
        GPIO_PORT_F_DATA = 0x1;
        break;
    }
  }
  return 0;
}

void GPIO_init(void) {
  volatile unsigned short delay = 0;
  RCGCGPIO_EN |= RCGCGPIO_PORT_F;     // enable port f
  RCGCGPIO_EN |= RCGCGPIO_PORT_N;     // enable port n
  delay++; // Delay 2 more cycles before access Timer registers
  delay++; // Refer to Page. 756 of Datasheet for info
  
  GPIO_PORT_F_DIR = LED_F;      // set PF0 and PF1 to output
  GPIO_PORT_F_DEN = LED_F;      // set PF0 and PF1 to digital port
  
  GPIO_PORT_N_DIR = LED_N;      // set PN0 and PN1 to output
  GPIO_PORT_N_DEN = LED_N;      // set PN0 and PN1 to digital port
  
  GPIO_PORT_F_DATA = 0x0;       // turn off LEDs first
  GPIO_PORT_N_DATA = 0x0;
}

void Timer_init(void) {
  RCGCTIMER_EN |= 0x1;          // enable timer 0
  GPTMCTL_TIMER_0 &= ~0x1;      // disable timer 0A
  GPTMCFG_TIMER_0 &= 0x00000000;// select 32-bit timer config
  GPTMTAMR_TIMER_0 |= 0x2;      // set period timer mode
  GPTMTAMR_TIMER_0 |= 0x10;     // set timer to count down
  GPTMTAILR_TIMER_0 = 0xF42400; // load 16 million (0xF42400)
  GPTMCTL_TIMER_0 |= 0x1;       // enable timer 0A;
}