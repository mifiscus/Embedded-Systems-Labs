// Return-to-sender program that communicates with a PuTTY terminal over bluetooth,
// returning the characters the user has sent back to them in the terminal

#include "Lab_3_Header.h"
#include <stdio.h>

void UART2_Init();
void UART_send_char(char x);
void UART_print_string(char *str);
char UART_receive_char(void);

int main(void) {
    UART2_Init();
    while (1) {
      char UART_input[2];
      UART_input[0] = UART_receive_char();
      UART_print_string(UART_input);
      UART_print_string("\n");
      UART_print_string("\r");
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Baud Rate Divisors (Baud Rate = 9600):
//
// 16 MHz: IBRD = 104(.1667), FBRD = 11(.1688)
//
////////////////////////////////////////////////////////////////////////////////

void UART2_Init(void) {
  volatile unsigned short delay = 0;
  RCGCUART_EN |= 0x4;                    // enable UART module 2
  RCGCGPIO_EN |= RCGCGPIO_PORT_A;        // enable GPIO port a
  delay++;
  delay++;
  GPIO_PORT_A_AFSEL = 0xC0;               // enable alterate function on PA6, PA7
  GPIO_PORT_A_PCTL |= 0x11000000;        // configure PA6, PA7 for UART
  GPIO_PORT_A_DEN |= 0xC0;                // enable digital for PA6, PA7
  UART2_CTL &= ~0x1;                     // diable UART0
  UART2_IBRD = 104;                      // initialize integer BRD to 16 MHz
  UART2_FBRD = 11;                       // initialize fractional BRD to 16 MHz
  UART2_LCRH = (0x3 << 5);               // select 8-bit wordlength
  UART2_CC = 0x0;                        // use system clock
  UART2_CTL = 0x301;                     // enable UART0 with transmit/receive
}

void UART_send_char(char x) {
  while (UART2_FR & 0x20) {}        // wait until holding register isn't full
  UART2_DR = x;                       // send char to UART data
}

void UART_print_string(char *str) {
  while (*str) {                      // run until nothing left in string
    UART_send_char(*str);             // send char into UART data function
    str++;                            // advance to next char in string
  }
}

char UART_receive_char(void) {
  while (UART2_FR & 0x10) {}
  return (char) UART2_DR;
}
