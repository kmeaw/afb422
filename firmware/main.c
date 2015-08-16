/* Name: main.c
 * Project: hid-custom-rq example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-07
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/*
This example should run on most AVRs with only little changes. No special
hardware resources except INT0 are used. You may have to change usbconfig.h for
different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
at least be connected to INT0 as well.
We assume that an LED is connected to port B bit 0. If you connect it to a
different port or bit, change the macros below:
*/
#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
#define LED_BIT             0

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "oddebug.h"        /* This is also an example for using debug macros */
#include "requests.h"       /* The custom request numbers we use */

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor[22] = {   /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};
/* The descriptor above is a dummy only, it silences the drivers. The report
 * it describes consists of one byte of undefined data.
 * We don't transfer our data through HID reports, we use custom requests
 * instead.
 */

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;
static char code[3];
uchar i, j, k, x;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR){
        DBG1(0x50, &rq->bRequest, 1);   /* debug output: print our request */
        if(rq->bRequest == CUSTOM_RQ_SET_BYTE0){
	  code[0] = rq->wValue.bytes[0];
	}else if(rq->bRequest == CUSTOM_RQ_SET_BYTE1){
	  code[1] = rq->wValue.bytes[0];
	}else if(rq->bRequest == CUSTOM_RQ_SET_BYTE2){
	  code[2] = rq->wValue.bytes[0];
        }else if(rq->bRequest == CUSTOM_RQ_EXEC){
	  for(i=0; i<8; i++)
	  {
	    /* 0 */
#define DELAY0 186
#define DELAY1 580
#define DELAY_SYNC 6048
	    PORTB |= 4;
	    _delay_us(DELAY0);
	    PORTB &= ~4;
	    _delay_us(DELAY1);
	    for(j=0; j<3; j++)
	    {
	      x = code[j];
	      for(k=0; k<8; k++)
	      {
		PORTB |= 4; // TX
		if (x & 0x80)
		  _delay_us(DELAY1);
		else
		  _delay_us(DELAY0);
		PORTB &= ~4; // TX
		if (x & 0x80)
		  _delay_us(DELAY0);
		else
		  _delay_us(DELAY1);
		x <<= 1;
	      }
	    }
	    PORTB |= 4;
	    _delay_ms(DELAY0);
	    PORTB &= ~4;
	    _delay_us(DELAY_SYNC);
	  }
	  // PORTB &= ~32; // disable power
	_delay_ms(100);
	PORTB |= 4;
        }
    }else{
        /* calss requests USBRQ_HID_GET_REPORT and USBRQ_HID_SET_REPORT are
         * not implemented since we never call them. The operating system
         * won't call them either because our descriptor defines no meaning.
         */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

int __attribute__((noreturn)) main(void)
{
        uchar i, j;

        /* no pullups on USB and ISP pins */
        PORTD = 0;
        PORTB = 0;
        /* all outputs except PD2 = INT0 */
        DDRD = ~(1 << 2);

        /* output SE0 for USB reset */
        DDRB = ~0;
        j = 0;
        /* USB Reset by device only required on Watchdog Reset */
        while (--j) {
                i = 0;
                /* delay >10ms for USB reset */
                while (--i)
                        ;
        }
        /* all USB and ISP pins inputs */
        DDRB = ((1<<2)|(1<<5)|(1<<4));

        /* all inputs except PC0, PC1 */
        DDRC = 0x03;
        PORTC = 0xfe;

        /* main event loop */
        usbInit();
        sei();
	PORTB |= 32; // enable power
        for (;;) {
                usbPoll();
        }
}

/* ------------------------------------------------------------------------- */
