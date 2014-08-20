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
#include "matrix.h"

uint32_t scankeycntms = 0;
uint8_t  kbdsleepmode = 0;
extern LED_MODE led_mode;

// for KEY_BEYOND_FN;
static uint8_t isBeyondFN = 0;	 //KEY_BEYOND_FN

static uint8_t currentKeymap=0;

	
// 17*8 bit matrix
uint8_t MATRIX[MAX_ROW];
uint8_t curMATRIX[MAX_ROW];
uint8_t svkeyidx[MAX_ROW][MAX_COL];

uint8_t matrixFN;           // (col << 4 | row)


extern int8_t usbmode;


extern uint8_t clearReportBuffer(void);
extern uint8_t buildHIDreports(uint8_t keyidx);
extern void putKey(uint8_t keyidx, uint8_t isPushed);

static uint8_t findFNkey(void)
{
    uint8_t matrixFN;
    uint8_t col, row;
    uint8_t keyidx;
    
	for(col=0;col<MAX_COL;col++)
	{
		for(row=0;row<MAX_ROW;row++)
        {
			keyidx = pgm_read_byte(&keymap_code[0][row][col]);
			//keyidx = pgm_read_byte(&keymap_code[currentKeymap][row][col]);

			if(keyidx == KEY_FN)
			{
                matrixFN = col << 4 | row;
                return matrixFN ;
			}
		}
    }
    return 0;
}


void keymap_init(void) 
{
	int i, keyidx;

	// set zero for every flags
	for(i=0;i<NUM_KEY;i++)
		KFLA[i]=0;
	
	// set flags
	for(i=0;(keyidx=pgm_read_byte(&keycode_set2_special[i]))!=KEY_NONE;i++)
		KFLA[keyidx] |= KFLA_SPECIAL;
	for(i=0;(keyidx=pgm_read_byte(&keycode_set2_makeonly[i]))!=KEY_NONE;i++)
		KFLA[keyidx] |= KFLA_MAKEONLY;
	for(i=0;(keyidx=pgm_read_byte(&keycode_set2_make_break[i]))!=KEY_NONE;i++)
		KFLA[keyidx] |= KFLA_MAKE_BREAK;
	for(i=0;(keyidx=pgm_read_byte(&keycode_set2_extend[i]))!=KEY_NONE;i++)
		KFLA[keyidx] |= KFLA_EXTEND;
	for(i=0;(keyidx=pgm_read_byte(&keycode_set2_proc_shift[i]))!=KEY_NONE;i++)
		KFLA[keyidx] |= KFLA_PROC_SHIFT;


	for(i=0;i<MAX_ROW;i++)
		MATRIX[i]=0;
    
    matrixFN = findFNkey();
}

uint8_t processFNkeys(uint8_t keyidx)
{
    uint8_t retVal = 0;
    if(keyidx == KEY_LED){
        if (led_mode >= LED_OFF)
        {
            led_mode = LED_FADING;
        }else{
            led_mode++;
        }

        led_mode_change(led_mode);
        retVal = 1;
    }
    return retVal;
}


uint8_t getLayer(uint8_t FNcolrow)
{
    uint8_t col;
    uint8_t row;
    uint8_t cur;
    
    col = (FNcolrow >> 4) & 0x0f;
    row = FNcolrow & 0x0f;
	
	DDRB  = BV(col);
	PORTB = ~BV(col);
	
	_delay_us(10);

	if(row<8)	{				// for 0..7, PORTA 0 -> 7
		cur = ~PINA & BV(row);
	}
	else if(row>=8 && row<16) {	// for 8..15, PORTC 7 -> 0
		cur = ~PINC & BV(15-row);
	}else{
        cur = ~PIND & BV(23-row);
	}

    if(cur)
        return 1+isBeyondFN;        // FN layer or beyondFN layer
    else
        return 0;                   // Normal layer
}


static uint8_t scanmatrix(void)
{
	uint8_t col, row;
	uint8_t prev, cur;
    uint8_t vPinA;
    uint8_t vPinC;
    uint8_t vPinD;

	uint8_t matrixState = 0;
#if 0
    if (scankeycntms++ >= 60000)
    {
        scankeycntms--;
        kbdsleepmode = 1;
        led_mode = LED_OFF;
        timer1PWMBSet((uint16_t)(0));
        timer1PWMBOn();
        ledOff(LED_ALL_PIN);
    }
#endif

	// scan matrix 
	for(col=0; col<MAX_COL; col++)
	{
		// Col -> set only one port as input and all others as output low
		DDRB  = BV(col);        //  only target col bit is output and others are input
		PORTB = ~BV(col);       //  only target col bit is LOW and other are pull-up

        _delay_us(10);

        vPinA = ~PINA;
        vPinC = ~PINC;
        vPinD = ~PIND;
        
		// scan each rows
		for(row=0; row<MAX_ROW; row++)
		{
			if(row<8)	{				// for 0..7, PORTA 0 -> 7
				cur = vPinA & BV(row);
			}
			else if(row>=8 && row < 16) {	// for 8..15, PORTC 7 -> 0
				cur = vPinC & BV(15-row);
			}else{
			    cur = vPinD & BV(23-row);
			}
            
            if (cur)
                matrixState |= SCAN_DIRTY;
            
			prev = curMATRIX[row]&BV(col);

			if (prev && !cur)       // key released
			{
                curMATRIX[row] &=~ BV(col);
                matrixState |= SCAN_RELEASED;		
			}else if (!prev && cur)       // key pushed
            {
                curMATRIX[row] |= BV(col);
                matrixState |= SCAN_PUSHED;		
            }		

        }
	}
    return matrixState;
}


uint8_t toggle = 0;

// return : key modified
uint8_t scankey(void)
{
	uint8_t col, row;
	uint8_t prev, cur;
	uint8_t keyidx;
	uint8_t matrixState = 0;
	uint8_t retVal = 0;
    uint8_t debounce = 0;

#if 0       // scan rate check 8times-USB (5times(PS/2)) in 5ms @ 12MHz XTAL
    if(toggle)
    {
        ledOn(LED_SCR_PIN);
    }else
    {
        ledOff(LED_SCR_PIN);
    }
    toggle ^= 1;
#endif

    matrixState = scanmatrix();


	//static int downLevel_prev = 0;

    /* LED Blinker */
    led_blink(matrixState);

    /* LED Fader */
    led_fader();


    /* debounce check */
	if(matrixState & SCAN_CHANGED)
	{
		debounce = DEBOUNCE_MAX;
        return retVal;
	}
	if(debounce > 1)
	{
        debounce--;
		return retVal;
	}

  
    clearReportBuffer();
	uint8_t keymap = getLayer(matrixFN);

	// keymap changes detect
	if(currentKeymap != keymap) {
		//DEBUG_PRINT(("KEYMAP CHANGED FROM %d - %d, lastMAKE_SIZE = %d, lastMAKE_keyidx = %d\n", currentKeymap, keymap, lastMAKE_SIZE, lastMAKE_keyidx));
	}
	// debounce cleared => compare last matrix and current matrix
	for(col=0;col<MAX_COL;col++)
	{
		for(row=0;row<MAX_ROW;row++)
		{
			prev = MATRIX[row]&BV(col);
			cur  = curMATRIX[row]&BV(col);
			keyidx = pgm_read_byte(&keymap_code[keymap][row][col]);
            if (keyidx == KEY_NONE || keyidx == KEY_FN)
                continue;


            /* LED_PUSHED_LEVEL calculate */
            if ((led_mode == LED_PUSHED_LEVEL) && (!prev && cur))
            {
                led_pushed_level_cal();
            }
            
            if (!prev && cur)   //pushed
            {
                if (processFNkeys(keyidx))
                    continue;
            }

            if(usbmode)
            {
                if(cur)
                {
                    retVal = buildHIDreports(keyidx);
                }
            }else
            {
                if (!prev && cur)   //pushed
                {
                    putKey(keyidx, 1);
                    svkeyidx[row][col] = keyidx;

                }else if (prev && !cur)  //released
                {
                    if (keyidx != KEY_LED)  // ignore KEY_LED relaseasing
                        putKey(svkeyidx[row][col], 0);
    			}
            }
 		}
	}
	
	for(row=0; row<MAX_ROW; row++)
		MATRIX[row] = curMATRIX[row];
	currentKeymap = keymap;
 
    retVal |= 1;
	return retVal;
}


