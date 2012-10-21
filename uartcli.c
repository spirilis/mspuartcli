#include <msp430.h>
#include <stdlib.h>
#include <string.h>
#include "uartcli.h"

volatile char *recvbuf;
volatile int recvsize;
volatile unsigned int recvidx;
unsigned int uartcli_token_arg_begin, uartcli_token_cmd_begin;
volatile char uartcli_task;

void uartcli_begin(char *inbuf, int insize)
{
	// Init USCI
	// Assumes SMCLK = MCLK/2 (8MHz)
	IFG2 &= ~(UCA0TXIFG | UCA0RXIFG);
	UCA0CTL0 = 0x00;
	UCA0CTL1 = UCSSEL_2 | UCSWRST;
	UCA0BR0 = 52;
	UCA0BR1 = 0;
	UCA0MCTL = UCBRS_0 | UCBRF_1 | UCOS16;
        P1SEL |= BIT1|BIT2;  // USCIA
        P1SEL2 |= BIT1|BIT2; //
	UCA0CTL1 &= ~UCSWRST;
	IE2 |= UCA0TXIE | UCA0RXIE;

	uartcli_setbuf(inbuf, insize);
	recvidx = 0;

	_EINT();
}

void uartcli_setbuf(char *inbuf, int insize)
{
	recvbuf = inbuf;
	if (insize > 1)
		recvsize = insize;
	else
		recvsize = 2;  // This ought to be obvious to debug.  It's 1 char + \0 terminator.
}

void uart_end()
{
	UCA0CTL1 |= UCSWRST;
	IE2 &= ~(UCA0TXIE | UCA0RXIE);
}

void uartcli_tx_lpm0()
{
	uartcli_task |= UARTCLI_TASK_TX;
	while (uartcli_task & UARTCLI_TASK_TX)
		LPM0;
}

void uartcli_submit_newline()
{
	char *newline, i=0;

	newline = (char *)UARTCLI_NEWLINE_OUTPUT;
	while (newline[i] != '\0') {
		UCA0TXBUF = newline[i];
		uartcli_tx_lpm0();
		i++;
	}
}

void uartcli_print_str(char *str)
{
	int i=0, j;

	j = strlen(str);
	for (; j; j--) {
		UCA0TXBUF = str[i];
		uartcli_tx_lpm0();
		i++;
	}
}

void uartcli_println_str(char *str)
{
	int i=0, j;

	j = strlen(str);
	for (; j; j--) {
		UCA0TXBUF = str[i];
		uartcli_tx_lpm0();
		i++;
	}
	uartcli_submit_newline();
}

const char hexdigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void uartcli_print_int(int num)
{
	char force_zero = 0;

	if (num < 0) {
		UCA0TXBUF = '-';
		uartcli_tx_lpm0();
		num *= -1;
	}
	if (num >= 10000) {
		UCA0TXBUF = hexdigits[num / 10000];
		uartcli_tx_lpm0();
		num -= (num/10000)*10000;
		force_zero = 1;
	}
	if (num >= 1000 || force_zero) {
		UCA0TXBUF = hexdigits[num / 1000];
		uartcli_tx_lpm0();
		num -= (num/1000)*1000;
		force_zero = 1;
	}
	if (num >= 100 || force_zero) {
		UCA0TXBUF = hexdigits[num / 100];
		uartcli_tx_lpm0();
		num -= (num/100)*100;
		force_zero = 1;
	}
	if (num >= 10 || force_zero) {
		UCA0TXBUF = hexdigits[num / 10];
		uartcli_tx_lpm0();
		num -= (num/10)*10;
		force_zero = 1;
	}
	UCA0TXBUF = hexdigits[num];
	uartcli_tx_lpm0();
}

void uartcli_println_int(int num)
{
	uartcli_print_int(num);
	uartcli_submit_newline();
}

void uartcli_print_uint(unsigned int num)
{
	char force_zero = 0;

	if (num >= 10000) {
		UCA0TXBUF = hexdigits[num / 10000];
		uartcli_tx_lpm0();
		num -= (num/10000)*10000;
		force_zero = 1;
	}
	if (num >= 1000 || force_zero) {
		UCA0TXBUF = hexdigits[num / 1000];
		uartcli_tx_lpm0();
		num -= (num/1000)*1000;
		force_zero = 1;
	}
	if (num >= 100 || force_zero) {
		UCA0TXBUF = hexdigits[num / 100];
		uartcli_tx_lpm0();
		num -= (num/100)*100;
		force_zero = 1;
	}
	if (num >= 10 || force_zero) {
		UCA0TXBUF = hexdigits[num / 10];
		uartcli_tx_lpm0();
		num -= (num/10)*10;
		force_zero = 1;
	}
	UCA0TXBUF = hexdigits[num];
	uartcli_tx_lpm0();
}

void uartcli_println_uint(unsigned int num)
{
	uartcli_print_uint(num);
	uartcli_submit_newline();
}

void uartcli_printhex_byte(char c)
{
	UCA0TXBUF = hexdigits[(c >> 4) & 0x0F];
	uartcli_tx_lpm0();
	UCA0TXBUF = hexdigits[c & 0x0F];
	uartcli_tx_lpm0();
}

void uartcli_printhex_word(int i)
{
	UCA0TXBUF = hexdigits[(i >> 12) & 0x0F];
	uartcli_tx_lpm0();
	UCA0TXBUF = hexdigits[(i >> 8) & 0x0F];
	uartcli_tx_lpm0();
	UCA0TXBUF = hexdigits[(i >> 4) & 0x0F];
	uartcli_tx_lpm0();
	UCA0TXBUF = hexdigits[i & 0x0F];
	uartcli_tx_lpm0();
}

inline char uartcli_available()
{
	return (uartcli_task & UARTCLI_TASK_AVAILABLE);
}

inline void uartcli_clear()
{
	uartcli_task &= ~UARTCLI_TASK_AVAILABLE;
}

/* UART Command Tokenizer */
void uartcli_token_begin()
{
	unsigned int i=0, l = strlen((const char*)recvbuf);

	while (i < l && recvbuf[i] == UARTCLI_TOKEN_DELIM)  // Ignore any leading whitespace
		i++;
	if (i != l)
		uartcli_token_cmd_begin = i;
	else
		uartcli_token_cmd_begin = 0;
	while (i < l && recvbuf[i] != UARTCLI_TOKEN_DELIM)  // Get past the initial command
		i++;
	while (i < l && recvbuf[i] == UARTCLI_TOKEN_DELIM)  // Get past the whitespace between command & arg#1
		i++;
	if (i != l)
		uartcli_token_arg_begin = i;
	else
		uartcli_token_arg_begin = 0;  // 0 is the "no args listed" signal.
}

int uartcli_token_cmd(const char *cmdstrings[])
{
	unsigned int i, k, l = strlen((const char*)recvbuf);
	int j=0;

	while (cmdstrings[j] != NULL)
		j++;
	if (j)       // Are there any commands listed at all?
		j--; // Start at the last one
	else
		return -1;  // If no commands are listed we have nothing more to do!

	for (; j+1; j--) {  // Count down from last item in cmdstrings[] to first item (0)
		// Compare cmdstrings[j] to the command in recvbuf
		i = uartcli_token_cmd_begin;
		k = 0;  // count of # matched chars
		while (i < l && recvbuf[i] != UARTCLI_TOKEN_DELIM && cmdstrings[j][i-uartcli_token_cmd_begin] != '\0') {
			if (cmdstrings[j][i-uartcli_token_cmd_begin] == recvbuf[i])
				k++;
			else
				break;  // didn't match?  make sure we skip to the next command
			i++;
		}
		if (k == (i - uartcli_token_cmd_begin) && cmdstrings[j][k] == '\0') {  // Make sure the cmdstring is this same size too
			// Matched the command; return the index
			return j;
		}
	}

	return -1;  // -1 = always the No Match condition
}

char* uartcli_token_cmdstr(char *buf, int buflen)
{
	unsigned int i=0, j=0, l = strlen((const char*)recvbuf);

	if (buflen < 1 || buf == NULL)
		return NULL;

	i = uartcli_token_cmd_begin;
	while (i < l && recvbuf[i] != UARTCLI_TOKEN_DELIM && j < buflen-1) {
		buf[j] = recvbuf[i];
		i++;
		j++;
	}
	buf[j] = '\0';

	return buf;
}

char* uartcli_token_arg(unsigned char argnum, char *buf, int buflen)
{
	unsigned int i=0, j=0, l = strlen((const char*)recvbuf);

	if (!argnum || !uartcli_token_arg_begin)
		return NULL;  // Arguments are addressed starting with 1.

	argnum--;  // But internally we start with 0...
	i = uartcli_token_arg_begin;
	while (j < argnum) {
		// Not at our intended argument; keep searching
		while (i < l && recvbuf[i] != UARTCLI_TOKEN_DELIM)  // Get past this argument...
			i++;
		while (i < l && recvbuf[i] == UARTCLI_TOKEN_DELIM)  // and the whitespace after it...
			i++;
		if (i != l)
			j++;
		else
			return NULL;  // Didn't find our argument, and hit the end.  Bail out.
	}

	// Finally there!  Copy as much as we can (up to 'buflen' bytes) into 'buf'.
	j = 0;
	while (i < l && recvbuf[i] != UARTCLI_TOKEN_DELIM && j < buflen-1) {
		buf[j] = recvbuf[i];
		i++;
		j++;
	}
	buf[j] = '\0';

	return buf;
}

// USCI TX continue with next char
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
	IFG2 &= ~UCB0TXIFG;  // Strawman; should never see this IFG anyway
	if (IFG2 & UCA0TXIFG) {
		IFG2 &= ~UCA0TXIFG;
		uartcli_task &= ~UARTCLI_TASK_TX;
		__bic_SR_register_on_exit(LPM4_bits);
	}
}

// USCI RX stuff a buffer
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
	// Handle nRF24L01+ SPI traffic
	if (IFG2 & UCB0RXIFG) {
		IFG2 &= ~UCB0RXIFG;
	        IE2 &= ~UCB0RXIE;
		__bic_SR_register_on_exit(LPM4_bits);
	}

	// Incoming UART serial data
	if (IFG2 & UCA0RXIFG) {
		char c = UCA0RXBUF;
		if ( !(uartcli_task & UARTCLI_TASK_AVAILABLE) ) {  // Don't process data if user hasn't interpreted last buffer
			if (c != UARTCLI_NEWLINE_DELIM) {
				if (c != '\n') {  // Ignore linefeed characters
					recvbuf[recvidx++] = c;
					if (recvidx >= recvsize-1) {  // Overflow; notify user & ignore further data
						recvbuf[recvidx] = '\0';
						recvidx = 0;
						uartcli_task &= ~UARTCLI_TASK_RECEIVING;
						uartcli_task |= UARTCLI_TASK_AVAILABLE;
						__bic_SR_register_on_exit(LPM4_bits);
					} else {
						uartcli_task |= UARTCLI_TASK_RECEIVING;
					}
				}
			} else {
				if (uartcli_task & UARTCLI_TASK_RECEIVING) {  // All done; notify user of new data!
					recvbuf[recvidx] = '\0';  // Close up the buffer
					recvidx = 0;              // Reset for next time
					uartcli_task &= ~UARTCLI_TASK_RECEIVING;
					uartcli_task |= UARTCLI_TASK_AVAILABLE;  // Notify user
					__bic_SR_register_on_exit(LPM4_bits);
				}
				/* If we're receiving a newline but haven't received any non-newline chars since
				 * the last clear, ignore.  No substance to show the user after all.
				 */
			}
		}
	}
}
