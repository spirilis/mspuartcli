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
