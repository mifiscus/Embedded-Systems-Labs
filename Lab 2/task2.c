// Michael Fiscus
// EE/CSE 474
// Spring 2020
// Lab 2 task 2_3

// This program uses timer interrupts to handle the timing demands of the 
// traffic light controller of lab 1
// Almost identical to my task 1_2, just with timer interrupts and flags instead

#include <stdint.h>
#include "lab_2_header.h"

#define FLAG_NONE 0x0
#define FLAG_CHANGE 0x1

void Traffic_SM(void);
void LED_Init(void);
void LED_R_On(void);
void LED_Y_On(void);
void LED_G_On(void);
void LED_Off(void);
void Extern_Switch_Init(void);
unsigned long Start_Stop_Input(void);
unsigned long Pedestrian_Input(void);
void Timer_Init(void);
void Button_Wait(void);
void Button_Wait_Restart(void);
void Light_Wait(void);
void Light_Wait_Restart(void);

// Flags for the ISR to change when the timing constraints have been met
unsigned char button_flag = 0;
unsigned char light_flag = 0;

// Button ISR resets the timer and changes the flag to tell the Button_wait
// function to increment the counter
void Timer0A_Handler(void) {
  GPTMICR_TIMER_0 |= 0x1;
  button_flag = FLAG_CHANGE;
}

// The same as the button ISR, but for the LED lights
void Timer1A_Handler(void) {
  GPTMICR_TIMER_1 |= 0x1;
  light_flag = FLAG_CHANGE;
}

// Variables to hold the count of seconds for the switches and LEDs
unsigned char button_second_count = 0;
unsigned char light_second_count = 0;
enum TC_States {TC_red, TC_yellow, TC_green, TC_rest} TC_State;

int main ( void )
{
  LED_Init();
  Extern_Switch_Init();
  Timer_Init();
  LED_Off();
  TC_State = TC_rest;
  
  while (1) {
    Traffic_SM();
    Light_Wait();
    Button_Wait();
  }
}

// Traffic light controller state machine
void Traffic_SM(void) {
  switch (TC_State) {   // Transitions
    case TC_red:
      if ((Start_Stop_Input() == 0x8) & (button_second_count >= 2)) {
        TC_State = TC_rest;
        Button_Wait_Restart();
      }
      if (light_second_count >= 5) {
        TC_State = TC_green;
        Light_Wait_Restart();
      }
      break;
    case TC_green:
      if ((Start_Stop_Input() == 0x8) & (button_second_count >= 2)) {
        TC_State = TC_rest;
        Button_Wait_Restart();
      }
      if ((Pedestrian_Input() == 0x4) & (button_second_count >= 2)) {
        TC_State = TC_yellow;
        Light_Wait_Restart();
      }
      else if (light_second_count >= 5) {
        TC_State = TC_red;
        Light_Wait_Restart();
      }
      break;
    case TC_yellow:
      if ((Start_Stop_Input() == 0x8) & (button_second_count >= 2)) {
        TC_State = TC_rest;
        Button_Wait_Restart();
      }
      if (light_second_count >= 5) {
        TC_State = TC_red;
        Light_Wait_Restart();
      }
      break;
    case TC_rest:
      if ((Start_Stop_Input() == 0x8) & (button_second_count >= 2)) {
        TC_State = TC_red;
        Button_Wait_Restart();
        Light_Wait_Restart();
      }
      break;
  }
  
  switch (TC_State) {   // Actions
    case TC_red:
      LED_R_On();
      break;
    case TC_green:
      LED_G_On();
      break;
    case TC_yellow:
      LED_Y_On();
      break;
    case TC_rest:
      LED_Off();
      break;
  }
}

void LED_Init(void)
{
 volatile unsigned short delay = 0;

 RCGCGPIO_EN |= RCGCGPIO_PORT_C; // activate clock for Port C
 delay++;
 delay++;
 GPIO_PORT_C_AMSEL &= ~0x70; // disable analog function of PC4
 GPIO_PORT_C_DIR |= 0x70; // set PC4 to output
 GPIO_PORT_C_AFSEL &= ~0x70; // regular port function
 GPIO_PORT_C_DEN |= 0x70; // enable digital output on PC4
}

// turn on red LED in PC4
void LED_R_On(void) {
  GPIO_PORT_C_DATA = 0x10; // set 4th bit to 1
}

// turn on yellow LED in PC5
void LED_Y_On(void) {
  GPIO_PORT_C_DATA = 0x20; 
}

// turn on green LED in PC6
void LED_G_On(void) {
  GPIO_PORT_C_DATA = 0x40;
}

// turn off all LEDs
void LED_Off(void) {
  GPIO_PORT_C_DATA = 0x0;
}

void Extern_Switch_Init(void)
{
 volatile unsigned short delay = 0;
 RCGCGPIO_EN |= RCGCGPIO_PORT_N; // Enable Port N Gating Clock
 delay++;
 delay++;
 GPIO_PORT_N_AMSEL &= ~0xC; // Disable PN2 and PN3 analog function
 GPIO_PORT_N_AFSEL &= ~0xC; // Select PN2 and PN3 regular port function
 GPIO_PORT_N_DIR &= ~0xC; // Set PN2 and PN3 to input direction
 GPIO_PORT_N_DEN |= 0xC; // Enable PN2 and PN3 digital function
}

// return output of left start/stop switch on PN3
unsigned long Start_Stop_Input(void) {
  return (GPIO_PORT_N_DATA & 0x8);
}

// return output of right pedestrian switch on PN2
unsigned long Pedestrian_Input(void) {
 return (GPIO_PORT_N_DATA & 0x4); 
}

void Timer_Init(void) {
  RCGCTIMER_EN |= 0x2;          // enable timer 0
  GPTMCTL_TIMER_0 &= ~0x1;      // disable timer 0A
  GPTMCTL_TIMER_1 &= ~0x1;      // disable timer 1A
  
  GPTMCFG_TIMER_0 &= 0x00000000;// select 32-bit timer 0 config
  GPTMCFG_TIMER_1 &= 0x00000000;// select 32-bit timer 1 config
  
  GPTMTAMR_TIMER_0 |= 0x2;      // set period timer 0 mode
  GPTMTAMR_TIMER_1 |= 0x2;      // set period timer 1 mode
  
  GPTMTAMR_TIMER_0 |= 0x10;     // set timer 0 to count down
  GPTMTAMR_TIMER_1 |= 0x10;     // set timer 1 to count down
  
  GPTMIMR_TIMER_0 |= 0x1;       // enable timer 0 interrupt mask
  GPTMIMR_TIMER_1 |= 0x1;       // enable timer 1 interrupt mask
  
  NVIC_EN0 |= (1<<19);          // enable timer 0 handler
  NVIC_EN0 |= (1<<21);          // enable timer 1 handler
}

// Replaced timer polling with interrupt flags to increment second counter
void Button_Wait(void) {
  if (Pedestrian_Input() | Start_Stop_Input()) {
    GPTMCTL_TIMER_0 |= 0x1;     // enable timer 0A
  }
  else {
    GPTMCTL_TIMER_0 &= ~0x1;    // disable timer 0A
    GPTMTAILR_TIMER_0 = 0xF42400;      // load 16 million (1 second)
    button_second_count = 0;
  }
  if (button_flag) {
    button_flag = 0;
    button_second_count++;
  }
}

void Button_Wait_Restart(void) {
  button_second_count = 0;
  GPTMCTL_TIMER_0 &= ~0x1;      // disable timer 0A
  GPTMTAILR_TIMER_0 = 0xF42400; // load 16 million (1 second)
  GPTMCTL_TIMER_0 |= 0x1;       // emable timer 0A
}

// Same as Button_Wait(), interrupt flag replaced timer polling
void Light_Wait(void) {
  if (light_flag) {
    light_flag = 0;
    light_second_count++;
  }
}

void Light_Wait_Restart(void) {
  light_second_count = 0;
  GPTMCTL_TIMER_1 &= ~0x1;    // disable timer 1A
  GPTMTAILR_TIMER_1 = 0xF42400;         // load 16 million (1 second)
  GPTMCTL_TIMER_1 |= 0x1;     // enable timer 1A
}
