/* Copyright Jamie Honan, 2001.  Distributed under the GPL.
   This program comes with ABSOLUTELY NO WARRANTY.
   See the file COPYING for license details.
   */
#define KEYBD_EXTERN
#include "global.h"
#include "timer.h"
//#include "keysta.h"
#include "keymap.h"
#include "print.h"
#include "led.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>     /* for _delay_ms() */

#include "usbdrv.h"
#include "eepaddress.h"

#define USB_LEVEL_SHIFT_PORT    PORTD
#define USB_LEVEL_SHIFT_DDR     DDRD

#define USB_LEVEL_SHIFT_PIN     6
#define PS2_PULLUP_PIN           7
#define INTERFACE_CHECK_TIME    5     // 50ms


int8_t usbmode = 1;

extern uint8_t usbmain(void);
extern uint8_t ps2main(void);




#ifdef DEBUG
void enable_printf(void)
{
	stdout = &mystdout;
    DDRD |= 0x01;

	UBRRH = (UBRR>>8);
	UBRRL = UBRR;
    
	UCSRA = 0x00;									   // asynchronous normal mode
	UCSRB = 0x08;									   // Tx enable, 8 data
	UCSRC = 0x86;									   // no parity, 1 stop, 8 data
}
#else
void enable_printf(void)
{
}
#endif





/* control zener diode for level shift signal line
    1 : TR on - 3v level
    0 : TR off - 5v level
*/
void DPpullEn(uint8_t enable)
{
    if (enable)
    {
        sbi(USB_LEVEL_SHIFT_PORT, USB_LEVEL_SHIFT_PIN);     // pullup
        cbi(USB_LEVEL_SHIFT_DDR, USB_LEVEL_SHIFT_PIN);      // input
        sbi(USB_LEVEL_SHIFT_DDR, PS2_PULLUP_PIN);      // input
        cbi(USB_LEVEL_SHIFT_PORT, PS2_PULLUP_PIN);     // drive 0
    }
    else
    {
        sbi(USB_LEVEL_SHIFT_DDR, USB_LEVEL_SHIFT_PIN);      // input
        cbi(USB_LEVEL_SHIFT_PORT, USB_LEVEL_SHIFT_PIN);     // drive 0
        sbi(USB_LEVEL_SHIFT_PORT, PS2_PULLUP_PIN);
        cbi(USB_LEVEL_SHIFT_DDR, PS2_PULLUP_PIN);
    }
}



int portInit(void)
{
    // initialize matrix ports - cols, rows
	// PB0-PB7 : col0 .. col7
	// PA0-PA7 : row0 .. row7
	// PC7-PC3 : row8 .. row12

    //PC 0 : reserved
    //PC 1 : reserved
    //PC 2 : reserved

    // PD0 : NUM
    // PD1 : CAPS
    // PD2 : D+ / Clock
    // PD3 : D- / Data
    // PD4 : FULL LED
    // PD5 : SCL
	// PD6 : TR pin for ZenerDiode
    // PD7 : DM Pullup
    
	// signal direction : col -> row


	PORTA	= 0xFF;	// all rows pull-up.
	PORTB	= 0xFF;	// pull-up
	PORTC	= 0xFF;
	PORTD	= 0x80; // PD6 drive Low -5V mode, ALL_LED off
	
	DDRA	= 0x00;	// all inputs for rows
	DDRB 	= 0xFF;	// all outputs for cols
	DDRC	= 0x00;
	DDRD    = 0x7F; // PD6(zener), PD3(D+), PD2(D-), ALL_LED output

    return 0;
}




int8_t checkInterface(void)
{
    uint8_t vPinA, vPinC, vPinD;
    uint8_t cur_usbmode = 0;

#ifdef PS2AVR
    DDRB  = BV(0);        //  col2
    PORTB = ~BV(0);       //
#else
	DDRB  = BV(2);        //  col2
	PORTB = ~BV(2);       //
#endif
    _delay_us(10);

    vPinA = ~PINA;
    vPinC = ~PINC;
    vPinD = ~PIND;
#ifdef PS2AVR
    if (vPinA & 0x20)   // col0-row5 => U
#else
    if (vPinA & 0x80)   // col2-row7 => U
#endif
    {
        cur_usbmode = 1;
        eeprom_write_byte(EEPADDR_USBPS2_MODE, cur_usbmode);
#ifdef PS2AVR
    }else if (vPinC & 0x80) // col0-row8 => P
#else
    }else if (vPinC & 0x20) // col2-row10 => P
#endif
    {
        cur_usbmode = 0;
        eeprom_write_byte(EEPADDR_USBPS2_MODE, cur_usbmode);
    }else
    {
        cur_usbmode = eeprom_read_byte(EEPADDR_USBPS2_MODE);                   
    }
    return cur_usbmode;
}

    



int main(void)
{

    enable_printf();
    
    portInit();
    usbmode = checkInterface();
    DPpullEn(usbmode);

    timerInit();
	timer1PWMInit(8);
	keymap_init();
    led_mode_init();
    

    if(usbmode)
    {
        led_check(1);
        usbmain();
    }else
    {
        
        timer2IntEnable();
        led_check(0);
        ps2main();
    }
    return 0;
}

