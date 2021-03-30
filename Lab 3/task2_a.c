// Same visual temperature program as task 1, but added functionality of communicating
// with a PuTTY terminal over UART serial communication

#include "Lab_3_Header.h"
#include "Lab_3_Driver.h"
#include <stdio.h>

#define SW 0x3
#define IBRD_60 32
#define IBRD_12 6
#define IBRD_120 65
#define FBRD_60 35
#define FBRD_12 33
#define FBRD_120 7

void Timer_0_Init(void);
void Timer_0_Count_Change(unsigned long TAILR_count);
void Timer_0_Poll(void);
void Timer_1_Init(void);
void Timer_1_Count_Change(unsigned long TAILR_count);
void LED_Ladder(void);
void LED_Init(void);
void SW_Init();
unsigned char SW_Input();
void ADC0_Init();
void UART0_Init();
void UART0_Clock_Change(int ibrd, int fbrd);
void UART_send_char(char x);
void UART_print_string(char *str);

volatile double temperature = 0;
volatile double adc_conversion = 0;
volatile unsigned char on_off = 1;

void ADC0_Handler() {
  while ((ADC0_RIS & 0x08) == 0) {}     // wait for conversion to complete
    adc_conversion = ADC0_SSFIFO3;
    temperature = (147.5 - (247.5 * adc_conversion) / 4096.0) - 1;
    ADC0_ISC |= 0x8;                    // reset ADC interrupt flag
    char temperature_string[8];
    sprintf(temperature_string, "%lf", temperature);
    UART_print_string(temperature_string);
    UART_print_string("\n");
    UART_print_string("\r");
}

int main(void) {
    enum frequency freq = PRESET2;
    PLL_Init(freq);             // PLL must be configured before everthing else
    Timer_0_Init();
    Timer_1_Init();
    LED_Init();
    SW_Init();
    ADC0_Init();
    UART0_Init();
    // Initialize timer0 and UART0 to 60 MHz and timer1 to 30 MHz (2 Hz blink)
    Timer_0_Count_Change(0x3938700);
    Timer_1_Count_Change(0x1C00000);
    UART0_Clock_Change(IBRD_60, FBRD_60);
    while (1) {
      LED_Ladder();
      Timer_0_Poll();
      switch (SW_Input()) {     // user switches change PLL frequency
        case 0x0:
          freq = PRESET2;                       // 60 MHz
          PLL_Init(freq);
          break;
        case 0x1:
          freq = PRESET3;                       // 12 MHz
          PLL_Init(freq);
          Timer_0_Count_Change(0xB71B00);        // 12 million
          Timer_1_Count_Change(0x5B8D80);        // 6 million
          UART0_Clock_Change(IBRD_12, FBRD_12);
          break;
        case 0x2:
          freq = PRESET1;                       // 120 MHz
          PLL_Init(freq);
          Timer_0_Count_Change(0x7270E00);      // 120 million
          Timer_1_Count_Change(0x3938700);      // 60 million
          UART0_Clock_Change(IBRD_120, FBRD_120);
          break;
      }
    }
    return 0;
}

void Timer_0_Init(void) {
  RCGCTIMER_EN |= 0x1;          // enable timer 0
  GPTMCTL_TIMER_0 &= ~0x1;      // disable timer 0A
  GPTMCFG_TIMER_0 &= 0x00000000;// select 32-bit timer 0 config
  GPTMTAMR_TIMER_0 |= 0x2;      // set period timer 0 mode
  GPTMADCEV_TIMER_0 |= 0x1;     // enable Timer A time-out event ADC trigger
}

void Timer_0_Count_Change(unsigned long TAILR_count) {
  GPTMCTL_TIMER_0 &= ~0x1;      // disable timer 0A
  GPTMTAILR_TIMER_0 = TAILR_count; // load variable
  GPTMCTL_TIMER_0 |= 0x21;       // enable timer 0A and set ADC trigger
}

void Timer_0_Poll(void) {
  if ((GPTMRIS_TIMER_0 & 0x1) == 1) {   // activate when 1 second has passed
    ADC0_PSSI |= 0x8;           // begin sampling SS3
    GPTMICR_TIMER_0 |= 0x1;     // reset timer 0
  }
}

void Timer_1_Init(void) {
  RCGCTIMER_EN |= 0x2;          // enable timer 1
  GPTMCTL_TIMER_1 &= ~0x1;      // disable timer 1A
  GPTMCFG_TIMER_1 &= 0x00000000;// select 32-bit timer 1 config
  GPTMTAMR_TIMER_1 |= 0x2;      // set period timer 1 mode
}

void Timer_1_Count_Change(unsigned long TAILR_count) {
  GPTMCTL_TIMER_1 &= ~0x1;      // disable timer 0A
  GPTMTAILR_TIMER_1 = TAILR_count; // load variable
  GPTMCTL_TIMER_1 |= 0x1;       // emable timer 0A
}

void LED_Ladder(void) {
  if ((GPTMRIS_TIMER_1 & 0x1) == 1) {
    if (on_off) {
      on_off = 0;            // blinking flag
      if (temperature < 20) {
        GPIO_PORT_N_DATA = 0x2;   // PN1
      }
      else if ((temperature >= 20) && (temperature < 24)) {
        GPIO_PORT_N_DATA = 0x3;   // PN1, PN0
      }
      else if ((temperature >= 24) && (temperature < 28)) {
        GPIO_PORT_N_DATA = 0x3;  // PN1, PN0
        GPIO_PORT_F_DATA = 0x10; // PF4
      }
      else if (temperature >= 28) {
        GPIO_PORT_N_DATA = 0x3;  // PN1, PN0
        GPIO_PORT_F_DATA = 0x11; // PF4, PF0
      }
    }
    else {
      GPIO_PORT_F_DATA = 0x0;
      GPIO_PORT_N_DATA = 0x0;
      on_off = 1;           // blinking flag
    }
    GPTMICR_TIMER_1 |= 0x1;
  }
}

void LED_Init(void) {
  volatile unsigned short delay = 0;
  RCGCGPIO_EN |= RCGCGPIO_PORT_F;     // enable port f
  RCGCGPIO_EN |= RCGCGPIO_PORT_N;     // enable port n
  delay++; // Delay 2 more cycles before access Timer registers
  delay++; // Refer to Page. 756 of Datasheet for info
  GPIO_PORT_F_DIR = 0x11;      // set PF0 and PF4 to output
  GPIO_PORT_F_DEN = 0x11;      // set PF0 and PF4 to digital port
  GPIO_PORT_N_DIR = 0x3;      // set PN0 and PN1 to output
  GPIO_PORT_N_DEN = 0x3;      // set PN0 and PN1 to digital port
  GPIO_PORT_F_DATA = 0x0;       // ensure LEDs are off
  GPIO_PORT_N_DATA = 0x0;
}

void SW_Init(void) {
  volatile unsigned short delay = 0;
  RCGCGPIO_EN |= RCGCGPIO_PORT_J;   // enable port j
  delay++; // Delay 2 more cycles before access Timer registers
  delay++; // Refer to Page. 756 of Datasheet for info
  GPIO_PORT_J_LOCK = 0x4C4F434B;    // unlock GPIOCR register for port j
  GPIO_PORT_J_CR = SW;              // commit used bits write to other registers
  GPIO_PORT_J_DIR = ~SW;            // set switch PJ0 and PJ1 as input
  GPIO_PORT_J_DEN = SW;             // set PJ0 and PJ1 to digital port
  GPIO_PORT_J_PUR = SW;             // enable pull up resistors on PJ0 and PJ1
}

unsigned char SW_Input (void) {
  return (GPIO_PORT_J_DATA & SW);
}

void ADC0_Init(void) {
  volatile unsigned short delay = 0;
  RCGCADC_EN |= 0x1;            // enable ADC module 0
  delay++;
  delay++;
  RCGCGPIO_EN |= RCGCGPIO_PORT_E;       // enable GPIO port E for ADC input
  delay++;
  delay++;
  GPIO_PORT_E_DIR &= ~0x8;
  GPIO_PORT_E_AFSEL |= 0x8;             // set PE3 to ADC input
  GPIO_PORT_E_DEN &= ~0x8;              // disable PE3 digital functions
  GPIO_PORT_E_AMSEL |= 0x8;             // enable PE3 analog function

  ALT_CLKCFG = 0x0;              // set PIOSC to alternate clock configuration
  ADC0_ACTSS &= ~0x8;            // disable ADC 0 sample sequencer 3
  ADC0_EMUX = 0x5000;            // use timer software trigger for ss3
  ADC0_SSMUX3 = 0x0;             // select 1st sample input PE3
  ADC0_SSCTL3 |= 0xE;            // select 1st sample temp sensor, enable
                                 // interrupt, and single conversion per sample
  ADC0_IM |= 0x08;               // set interrupt mask
  NVIC_EN0 |= (1 << 17);         // enable ADC0 interrupt
  ADC0_ACTSS |= 0x8;             // enable ADC0 sample sequencer 3
}

////////////////////////////////////////////////////////////////////////////////
//
// Baud Rate Divisors (Baud Rate = 115200):
//
// 60 MHz: IBRD = 32(.5521), FBRD = 35(.8344)
// 12 MHz: IBRD = 6(.5104), FBRD = 33(.1656)
// 120 MHZ: IBRD = 65(.1042), FBRD = 7(.1688)
//
////////////////////////////////////////////////////////////////////////////////
void UART0_Init(void) {
  volatile unsigned short delay = 0;
  RCGCUART_EN |= 0x1;                    // enable UART module 0
  RCGCGPIO_EN |= RCGCGPIO_PORT_A;        // enable GPIO port a
  delay++;
  delay++;
  GPIO_PORT_A_AFSEL = 0x3;               // enable alterate function on PA0, PA1
  GPIO_PORT_A_PCTL |= 0x11;              // configure PA0, PA1 for UART
  GPIO_PORT_A_DEN |= 0x3;                // enable digital for PA0, PA1
  UART0_CTL &= ~0x1;                     // diable UART0
  UART0_CC = 0x0;                        // use system clock
}

void UART0_Clock_Change(int ibrd, int fbrd) {
  UART0_CTL &= ~0x1;                     // diable UART0
  UART0_IBRD = ibrd;                  // initialize integer BRD to 60 MHz
  UART0_FBRD = fbrd;                  // initialize fractional BRD to 60 MHz
  UART0_LCRH = (0x3 << 5);               // select 8-bit wordlength
  UART0_CTL = 0x301;                     // enable UART0 with transmit/receive
}

void UART_send_char(char x) {
  while (UART0_FR & (0x20)) {}        // wait until holding register isn't full
  UART0_DR = x;                       // send char to UART data
}

void UART_print_string(char *str) {
  while (*str) {                      // run until nothing left in string
    UART_send_char(*str);             // send char into UART data function
    str++;                            // advance to next char in string
  }
}
