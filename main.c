/* Demo code showcasing the uartcli library
 * Program initializes a USCI_A capable MSP430 at 16MHz, runs SMCLK at 4MHz (MCLK/4),
 * then utilizes the uartcli library (9600bps--the only bitrate the TI LaunchPad USB
 * interface is capable of using) to listen for incoming commands, parse them against
 * a simple list of strings (cmdlist[]) deriving an index of which string corresponds to the
 * first contiguous word in the input buffer (or -1 if none do), then prints up to 9
 * arguments (whitespace-delimited) afterward.
 */

#include <msp430.h>
#include "uartcli.h"

const char *cmdlist[] = {"cat", "ls", "less", "more", NULL};

int main()
{
	char cmd[32], argbuf[32];
	int i;

        WDTCTL = WDTPW | WDTHOLD;
        DCOCTL = CALDCO_16MHZ;
        BCSCTL1 = CALBC1_16MHZ;
        BCSCTL2 = DIVS_2;  // SMCLK = MCLK/4
        BCSCTL3 = LFXT1S_2;  // ACLK = VLOCLK/1
        BCSCTL3 &= ~(XT2OF|LFXT1OF);

	uartcli_begin(cmd, 32);

	while (1) {
		if (uartcli_available()) {
			uartcli_token_begin();
			i = uartcli_token_cmd(cmdlist);
			uartcli_println_int(i);
			for (i=1; i<10; i++) {
				uartcli_print_str("Arg#"); uartcli_print_int(i); uartcli_print_str(": ");
				if (uartcli_token_arg(i, argbuf, 32) != NULL) {
					uartcli_println_str(argbuf);
				} else {
					uartcli_println_str("NULL");
				}
			}
			uartcli_clear();  // Tell UART code we're done processing the last cmd, and it can accept new input.
					  // Otherwise it will quite literally ignore every character coming in.
		}
		LPM0;
	}
}
