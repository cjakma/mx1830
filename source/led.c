#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>     /* for _delay_ms() */


#include "led.h"
#include "global.h"
#include "timer.h"
#include "matrix.h"
#include "eepaddress.h"
#define PUSHED_LEVEL_MAX        20



LED_MODE led_mode = LED_FADING;
uint8_t LEDstate;     ///< current state of the LEDs


static uint16_t downLevelStay = 0;
static uint8_t downLevel = 0;

static uint8_t downLevelLife = 0;

extern int16_t scankeycntms;
extern uint8_t  kbdsleepmode;
int speed = 4;
int brigspeed = 10;
int pwmDir = 0;
static int pwmCounter = 4; //speed;


void led_3lockupdate(uint8_t LEDstate)
{
        if (LEDstate & LED_NUM) { // light up caps lock
            ledOn(LED_NUM_PIN);
        } else {
            ledOff(LED_NUM_PIN);
        }
        if (LEDstate & LED_CAPS) { // light up caps lock
            ledOn(LED_CAP_PIN);
            if (led_mode == LED_FULLCAPS)
                ledOn(LED_FULL_PIN);
        } else {
            ledOff(LED_CAP_PIN);
            if (led_mode == LED_FULLCAPS)
                ledOff(LED_FULL_PIN);
        }
        if (LEDstate & LED_SCROLL) { // light up caps lock
            ledOn(LED_SCR_PIN);
        } else {
            ledOff(LED_SCR_PIN);
        }
}


void led_blink(int matrixState)
{
    if(matrixState & SCAN_DIRTY)      // 1 or more key is pushed
    {
        switch(led_mode)
        {
            default :
                break;
            case LED_FADING_PUSH_ON:
            case LED_PUSH_ON:
                ledOn(LED_FULL_PIN);
                break;
            case LED_PUSH_OFF:
                timer1PWMBOn();
                timer1PWMBSet((uint16_t)(0));
                ledOff(LED_FULL_PIN);
                break;
        }
        scankeycntms = 0;
        if (kbdsleepmode == 1)
        {
            led_mode_init();
            led_3lockupdate(LEDstate);
            kbdsleepmode = 0;
        }
    }else{          // none of keys is pushed
        switch(led_mode)
             {
                 default :
                     break;
                 case LED_FADING_PUSH_ON:
                    if (scankeycntms > 1000)  // idle mode so fade
                        break;
                 case LED_PUSH_ON:
                    ledOff(LED_FULL_PIN);
                    break;
                 case LED_PUSH_OFF:
                    ledOn(LED_FULL_PIN);
                    break;
             }
    }
}

void led_fader(void)
{
    if((led_mode == LED_FADING) || ((led_mode == LED_FADING_PUSH_ON) && (scankeycntms > 1000)))
    {
        if(pwmDir==0)
        {
            timer1PWMBSet((uint16_t)(pwmCounter/brigspeed));        // brighter
            if(pwmCounter>=255*brigspeed)
                pwmDir = 1;
                
        }
        else if(pwmDir==2)
        {
            timer1PWMBSet((uint16_t)(255-pwmCounter/speed));    // darker
            if(pwmCounter>=255*speed)
                pwmDir = 3;

        }
        else if(pwmDir==1)
        {
            if(pwmCounter>=255*speed)
               {
                   pwmCounter = 0;
                   pwmDir = 2;
               }
        }else if(pwmDir==3)
        {
            if(pwmCounter>=255*brigspeed)
               {
                   pwmCounter = 0;
                   pwmDir = 0;
               }
        }
        


        timer1PWMBOn();

        // pwmDir 0~3 : idle
   
        pwmCounter++;

#if 0
        if (kbd_flags & FLA_RX_BAD)
        {
            cli();
            kbd_flags &= ~FLA_RX_BAD;
            sei();
        }
#endif
    }else if (led_mode == LED_PUSHED_LEVEL)
    {
		// 일정시간 유지

		if(downLevelStay > 0){
			downLevelStay--;
		}else{
			// 시간이 흐르면 레벨을 감소 시킨다.
			if(downLevelLife> 0){
				pwmCounter++;
				if(pwmCounter >= speed){
					pwmCounter = 0;			
					downLevelLife--;
					downLevel = PUSHED_LEVEL_MAX - (255-downLevelLife) / (255/PUSHED_LEVEL_MAX);
					/*if(downLevel_prev != downLevel){
						DEBUG_PRINT(("---------------------------------decrease downLevel : %d, life : %d\n", downLevel, downLevelLife));
						downLevel_prev = downLevel;
					}*/
				}
			}else{
				downLevel = 0;
				pwmCounter = 0;
			}
		}
		timer1PWMBSet((uint16_t)(downLevelLife));

	}else
    {
        timer1PWMBSet((uint16_t)(0));
        timer1PWMBOff();
        pwmCounter=0;
        pwmDir=0;
    }
}

void led_check(uint8_t forward)
{
    ledOff(LED_ALL_PIN);
    if(forward){
        ledOn(LED_NUM_PIN);
    }else{
        ledOn(LED_SCR_PIN);
    }
    _delay_ms(100);
    ledOff(LED_ALL_PIN);
    ledOn(LED_CAP_PIN);

    _delay_ms(100);
    ledOff(LED_ALL_PIN);
    if(forward){
        ledOn(LED_SCR_PIN);
    }else{
        ledOn(LED_NUM_PIN);
    }
    _delay_ms(100);
    ledOff(LED_ALL_PIN);
}

void led_mode_init(void)
{
    led_mode = eeprom_read_byte(EEPADDR_LED_STATUS);                
    led_mode_change(led_mode);
}

void led_mode_change (int mode)
{
    switch (mode)
    {
        case LED_FADING :
        case LED_FADING_PUSH_ON :
            break;
        case LED_PUSH_OFF :
        case LED_ALWASY :
            ledOn(LED_FULL_PIN);
            break;
        case LED_PUSH_ON :
        case LED_OFF :
        case LED_PUSHED_LEVEL :
        case LED_FULLCAPS :
            timer1PWMBOn();
            timer1PWMBSet((uint16_t)(0));
            ledOff(LED_FULL_PIN);
            break;
     }
    eeprom_write_byte(EEPADDR_LED_STATUS, led_mode);
}


void led_pushed_level_cal(void)
{
	// update pushed level
	if(downLevel < PUSHED_LEVEL_MAX)
	{
		downLevelStay = 511;
		downLevel++;
		downLevelLife = 255 * downLevel / PUSHED_LEVEL_MAX;
	}
}


