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
#include "keymap.h"
#include "matrix.h"
#include "hwaddress.h"
#include "macro.h"

#define PUSHED_LEVEL_MAX        20


static uint8_t *const ledport[] = {LED_NUM_PORT, LED_CAP_PORT,LED_SCR_PORT, LED_BASE_PORT,  LED_WASD_PORT, 
                                   LED_PRT_PORT, LED_ESC_PORT,LED_Fx_PORT,LED_PAD_PORT, LED_ARROW18_PORT, LED_VESEL_PORT};
    
static uint8_t const ledpin[] = {LED_NUM_PIN, LED_CAP_PIN, LED_SCR_PIN, LED_BASE_PIN,   LED_WASD_PIN,
                                  LED_PRT_PIN, LED_ESC_PIN,LED_Fx_PIN,LED_PAD_PIN, LED_ARROW18_PIN, LED_VESEL_PIN};

#define LEDMODE_ARRAY_SIZE 5*11


static uint8_t speed[] = {1, 1, 1, 5, 5, 5, 5, 5, 5, 5, 5};
static uint8_t brigspeed[] = {1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3};
static uint8_t pwmDir[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint16_t pwmCounter[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static uint16_t pushedLevelStay[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t pushedLevel[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint16_t pushedLevelDuty[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t LEDstate;     ///< current state of the LEDs

extern uint16_t scankeycntms;



void led_off(LED_BLOCK block)
{
    uint8_t i;
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK:
        case LED_PIN_PRT:

        case LED_PIN_ESC:
        case LED_PIN_Fx:
        case LED_PIN_BASE:
        case LED_PIN_WASD:
        case LED_PIN_VESEL:
            break;                    
        case LED_PIN_PAD:
            break;
        case LED_PIN_ARROW18:
            break;
            
        case LED_PIN_ALL:
            return;
        default:
            return;
    }
    
    *(ledport[block]) &= ~(BV(ledpin[block]));
}

void led_on(LED_BLOCK block)
{
    
    uint8_t i;
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK:
        case LED_PIN_PRT:

        case LED_PIN_ESC:
        case LED_PIN_Fx:
        case LED_PIN_BASE:
        case LED_PIN_WASD:
        case LED_PIN_VESEL:
            break;                    
        case LED_PIN_PAD:
            break;
        case LED_PIN_ARROW18:
            break;
        case LED_PIN_ALL:
            return;
        default:
            return;
    }
    led_wave_off(block);
    *(ledport[block]) |= BV(ledpin[block]);
}


void led_wave_on(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_ESC:
            break;
        case LED_PIN_Fx:
            timer1PWMAOn();
            break;
        case LED_PIN_PAD:
            break;
        case LED_PIN_BASE:
            timer1PWMBOn();
            break;
        case LED_PIN_WASD:
            timer2PWMOn();
            break;
        case LED_PIN_ARROW18:
            break;
        case LED_PIN_VESEL:
            break;
            
        case LED_PIN_ALL:
            timer1PWMBOn();
            timer1PWMAOn();
            timer2PWMOn();
            break;
        default:
            break;
    }
}

void led_wave_off(LED_BLOCK block)
{
    switch(block)
    {
        case LED_PIN_ESC:
            break;
        case LED_PIN_Fx:
            timer1PWMAOff();
            break;
        case LED_PIN_PAD:
            break;
        case LED_PIN_BASE:
            timer1PWMBOff();
            break;
        case LED_PIN_WASD:
            timer2PWMOff();
            break;
        case LED_PIN_ARROW18:
            break;
        case LED_PIN_VESEL:
            break;

        case LED_PIN_ALL:
            timer1PWMAOff();
            timer1PWMBOff();
            timer2PWMOff();
            break;
        default:
            break;
    }
}




void led_wave_set(LED_BLOCK block, uint16_t duty)
{
    switch(block)
    {
        case LED_PIN_ESC:
            break;
        case LED_PIN_Fx:
            timer1PWMASet(duty);
            break;
        case LED_PIN_PAD:
            break;
        case LED_PIN_BASE:
            timer1PWMBSet(duty);
            break;
        case LED_PIN_WASD:
            timer2PWMSet(duty);
            break;
        case LED_PIN_ARROW18:
            break;
        case LED_PIN_VESEL:
            break;
        case LED_PIN_ALL:
            timer1PWMASet(duty);
            timer1PWMBSet(0xa0);
            timer2PWMSet(0xa0);
            break;
        default:
            break;
    }
}




void led_blink(int matrixState)
{
    LED_BLOCK ledblock;

    for (ledblock = LED_PIN_BASE; ledblock < LED_PIN_WASD; ledblock++)
    {
        
        if(matrixState & SCAN_DIRTY)      // 1 or more key is pushed
        {
            switch(kbdConf.led_preset[kbdConf.led_preset_index][ledblock])
            {

                case LED_EFFECT_FADING_PUSH_ON:
                case LED_EFFECT_PUSH_ON:
                    led_on(ledblock);
                    break;
                case LED_EFFECT_PUSH_OFF:
                    led_wave_off(ledblock);
                    led_wave_set(ledblock, 0);
                    led_off(ledblock);
                    break;
                default :
                    break;
            }             
        }else{          // none of keys is pushed
            switch(kbdConf.led_preset[kbdConf.led_preset_index][ledblock])
                 {
                     case LED_EFFECT_FADING_PUSH_ON:
                     case LED_EFFECT_PUSH_ON:
                        led_off(ledblock);
                        break;
                     case LED_EFFECT_PUSH_OFF:
                        led_on(ledblock);
                        break;
                     default :
                         break;
                 }
        }
    }
}

void led_fader(void)
{
    
    uint8_t ledblock;
    for (ledblock = LED_PIN_BASE; ledblock < LED_PIN_WASD; ledblock++)
    {
        if((kbdConf.led_preset[kbdConf.led_preset_index][ledblock] == LED_EFFECT_FADING) || ((kbdConf.led_preset[kbdConf.led_preset_index][ledblock] == LED_EFFECT_FADING_PUSH_ON) && (scankeycntms < (SCAN_COUNT_IN_MIN * kbdConf.sleepTimerMin) - 1000)))
        {
            if(pwmDir[ledblock]==0)
            {
                led_wave_set(ledblock, ((uint16_t)(pwmCounter[ledblock]/brigspeed[ledblock])));        // brighter
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                    pwmDir[ledblock] = 1;
                    
            }
            else if(pwmDir[ledblock]==2)
            {
                led_wave_set(ledblock, ((uint16_t)(255-pwmCounter[ledblock]/speed[ledblock])));    // darker
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                    pwmDir[ledblock] = 3;

            }
            else if(pwmDir[ledblock]==1)
            {
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                   {
                       pwmCounter[ledblock] = 0;
                       pwmDir[ledblock] = 2;
                   }
            }else if(pwmDir[ledblock]==3)
            {
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                   {
                       pwmCounter[ledblock] = 0;
                       pwmDir[ledblock] = 0;
                   }
            }


            led_wave_on(ledblock);

            // pwmDir 0~3 : idle
       
            pwmCounter[ledblock]++;

        }else if (kbdConf.led_preset[kbdConf.led_preset_index][ledblock] == LED_EFFECT_PUSHED_LEVEL)
        {
    		// �����ð� ����

    		if(pushedLevelStay[ledblock] > 0){
    			pushedLevelStay[ledblock]--;
    		}else{
    			// �ð��� �帣�� ������ ���� ��Ų��.
    			if(pushedLevelDuty[ledblock] > 0){
    				pwmCounter[ledblock]++;
    				if(pwmCounter[ledblock] >= speed[ledblock]){
    					pwmCounter[ledblock] = 0;			
    					pushedLevelDuty[ledblock]--;
    					pushedLevel[ledblock] = PUSHED_LEVEL_MAX - (255-pushedLevelDuty[ledblock]) / (255/PUSHED_LEVEL_MAX);
    					/*if(pushedLevel_prev != pushedLevel){
    						DEBUG_PRINT(("---------------------------------decrease pushedLevel : %d, life : %d\n", pushedLevel, pushedLevelDuty));
    						pushedLevel_prev = pushedLevel;
    					}*/
    				}
    			}else{
    				pushedLevel[ledblock] = 0;
    				pwmCounter[ledblock] = 0;
    			}
    		}
    		led_wave_set(ledblock, pushedLevelDuty[ledblock]);

    	}else
        {
            led_wave_set(ledblock, 0);
            led_wave_off(ledblock);
            pwmCounter[ledblock]=0;
            pwmDir[ledblock]=0;
        }
    }
}

void led_check(uint8_t forward)
{
    led_off(LED_PIN_ALL);
    if(forward){
        led_on(LED_PIN_NUMLOCK);
    }else{
        led_on(LED_PIN_SCROLLOCK);
    }
    _delay_ms(100);
    led_off(LED_PIN_ALL);
    led_on(LED_PIN_CAPSLOCK);

    _delay_ms(100);
    led_off(LED_PIN_ALL);
    if(forward){
        led_on(LED_PIN_SCROLLOCK);
    }else{
        led_on(LED_PIN_NUMLOCK);
    }
    _delay_ms(100);
    led_off(LED_PIN_ALL);
}


void led_3lockupdate(uint8_t LEDstate)
{
   uint8_t ledblock;
       if (LEDstate & LED_NUM) { // light up caps lock
           led_on(LED_PIN_NUMLOCK);
       } else {
           led_off(LED_PIN_NUMLOCK);
       }
       if (LEDstate & LED_CAPS) { // light up caps lock
           led_on(LED_PIN_CAPSLOCK);
            if (kbdConf.led_preset[kbdConf.led_preset_index][LED_PIN_BASE] == LED_EFFECT_BASECAPS)
                led_on(LED_PIN_BASE);
       } else {
           led_off(LED_PIN_CAPSLOCK);
            if (kbdConf.led_preset[kbdConf.led_preset_index][LED_PIN_BASE] == LED_EFFECT_BASECAPS)
               led_off(LED_PIN_BASE);
       }
       if (LEDstate & LED_SCROLL) { // light up caps lock
           led_on(LED_PIN_SCROLLOCK);
       } else {
           led_off(LED_PIN_SCROLLOCK);
       }

}

#define LED_INDICATOR_MAXTIME 90
#define LED_INDICATOR_MAXINDEX 16

uint8_t ledESCIndicator[6][LED_INDICATOR_MAXINDEX] = {
   {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}
};

uint8_t ledPRTIndicator[6][LED_INDICATOR_MAXINDEX] = {
   {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
   {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};


uint8_t ledESCIndicatorTimer;
uint8_t ledESCIndicatorIndex;

uint8_t ledPRTIndicatorTimer;
uint8_t ledPRTIndicatorIndex;


void led_ESCIndicater(uint8_t layer)
{
   return;
    if(ledESCIndicatorTimer++ >= LED_INDICATOR_MAXTIME)
    {
        if(ledESCIndicator[layer][ledESCIndicatorIndex] == 1)
        {
            led_on(LED_PIN_ESC);
        }else
        {
            led_off(LED_PIN_ESC);
        }
        if (++ledESCIndicatorIndex >= LED_INDICATOR_MAXINDEX)
        {
            ledESCIndicatorIndex = 0;
        }
        ledESCIndicatorTimer = 0;
    }
}

void led_PRTIndicater(uint8_t index)
{
    return;
    if(ledPRTIndicatorTimer++ >= LED_INDICATOR_MAXTIME)
    {
        if(ledPRTIndicator[index][ledPRTIndicatorIndex] == 1)
        {
            led_on(LED_PIN_PRT);
        }else
        {
            led_off(LED_PIN_PRT);
        }
        if (++ledPRTIndicatorIndex >= LED_INDICATOR_MAXINDEX)
        {
            ledPRTIndicatorIndex = 0;
        }
        ledPRTIndicatorTimer = 0;
    }
}



void led_mode_init(void)
{
    LED_BLOCK ledblock;
    int16_t i;
    uint8_t *buf;

    for (ledblock = LED_PIN_BASE; ledblock < LED_PIN_WASD; ledblock++)
    {
      pwmDir[ledblock ] = 0;
      pwmCounter[ledblock] = 0;
      led_mode_change(ledblock, kbdConf.led_preset[kbdConf.led_preset_index][ledblock]);
    }
    
    led_3lockupdate(LEDstate);
}

void led_mode_change (LED_BLOCK ledblock, int mode)
{
    switch (mode)
    {
        case LED_EFFECT_FADING :
        case LED_EFFECT_FADING_PUSH_ON :
            break;
        case LED_EFFECT_PUSH_OFF :
        case LED_EFFECT_ALWAYS :
            led_on(ledblock);
            break;
        case LED_EFFECT_PUSH_ON :
        case LED_EFFECT_OFF :
        case LED_EFFECT_PUSHED_LEVEL :
        case LED_EFFECT_BASECAPS :
            led_off(ledblock);
            led_wave_set(ledblock,0);
            led_wave_on(ledblock);
            break;
        default :
            kbdConf.led_preset[kbdConf.led_preset_index][ledblock] = LED_EFFECT_FADING;
            break;
     }
}

void led_mode_save(void)
{
   #if 0
    eeprom_write_byte(EEPADDR_LED_STATUS, kbdConf.led_preset_index);
#endif
}

void led_pushed_level_cal(void)
{
    LED_BLOCK ledblock;
	// update pushed level
	
    for (ledblock = LED_PIN_BASE; ledblock < LED_PIN_WASD; ledblock++)
    { 
        if(pushedLevel[ledblock] < PUSHED_LEVEL_MAX)
        {
            pushedLevelStay[ledblock] = 511;
            pushedLevel[ledblock]++;
            pushedLevelDuty[ledblock] = (255 * pushedLevel[ledblock]) / PUSHED_LEVEL_MAX;
        }
	}
}
#if 0
uint8_t PROGMEM ledstart[32] = "LED record mode - push any key@";
uint8_t PROGMEM ledend[32] = "LED record done@";
uint8_t PROGMEM sledmode[8][15] = {"fading@", "fading-pushon@", "pushed-weight@","pushon@", "pushoff@", "always@", "caps@", "off@"}; 
uint8_t PROGMEM sledblk[5][8] = {"Fx----", "Pad---", "Base--", "WASD--", "Arrow-"};
uint8_t PROGMEM line[] = "=====================@";
void recordLED(uint8_t ledkey)
{
    kbdConf.led_preset_index = ledkey - K_LED0;
    int8_t col, row;
    uint32_t prev, cur;
    uint8_t prevBit, curBit;
    uint8_t keyidx;
    uint8_t matrixState = 0;
    uint8_t retVal = 0;
    int8_t i;
    int16_t index;
    long page;
    uint8_t ledblk;
    long address;
    index = 0;
    page = 0;
    long keyaddr;
    uint8_t *tmpBuf;
    sendString(ledstart);
    wdt_reset();

    for(col = 0; col < MAX_COL; col++)
    {
        for(row = 0; row < MAX_ROW; row++)
        {
            debounceMATRIX[col][row] = -1;
        }
    }
    sendString(line);

    while(1)
    {

    wdt_reset();
    matrixState = scanmatrix();

    /* LED Blinker */
    led_blink(matrixState);

    /* LED Fader */
    led_fader();
    


    // debounce cleared => compare last matrix and current matrix
    for(col = 0; col < MAX_COL; col++)
    {
     prev = MATRIX[col];
     cur  = curMATRIX[col];
     MATRIX[col] = curMATRIX[col];
     for(i = 0; i < MAX_ROW; i++)
     {
        prevBit = (uint8_t)prev & 0x01;
        curBit = (uint8_t)cur & 0x01;
        prev >>= 1;
        cur >>= 1;

#ifdef KBDMOD_M5
        if (i < 8)
        {
            row = 10+i;
        }else if (i < 16)
        {
            row = -6+i;
        }else
        {
            row = -16+i;
        }
        
#else //KBDMOD_M7
         row = i;
#endif

        keyidx = pgm_read_byte(keymap[6]+(col*MAX_ROW)+row);

         if (keyidx == K_NONE)
            continue;

         if (!prevBit && curBit)   //pushed
         {
            debounceMATRIX[col][row] = 0;    //triger

         }else if (prevBit && !curBit)  //released
         {
            debounceMATRIX[col][row] = 0;    //triger
         }

         if(debounceMATRIX[col][row] >= 0)
         {                
            if(debounceMATRIX[col][row]++ >= DEBOUNCE_MAX)
            {
               if(curBit)
               {
                    if (keyidx == K_FN)
                    {
                        sendString(line);
                        wdt_reset();
                        for (ledblk = LED_PIN_Fx; ledblk <= LED_PIN_WASD; ledblk++)
                        {
                           if(ledblk == LED_PIN_PAD)
                            continue;
                           
                           sendString(sledblk[ledblk-5]);
                           sendString(sledmode[kbdConf.led_preset[kbdConf.led_preset_index][ledblk]]);
                        }
                        sendString(line);
                        wdt_reset();
                        sendString(ledend);


                        flash_writeinpage(kbdConf.led_preset, LEDMODE_ADDRESS);
                        wdt_reset();
                        led_mode_save();
                        wdt_reset();
                        

                        return;
                    }else
                    {
                        ledblk = keyidx - K_LFX + 5;
                        kbdConf.led_preset[kbdConf.led_preset_index][ledblk] = kbdConf.led_preset[kbdConf.led_preset_index][ledblk] + 1;
                        
                        if(kbdConf.led_preset[kbdConf.led_preset_index][ledblk] >= LED_EFFECT_NONE)
                        {
                            kbdConf.led_preset[kbdConf.led_preset_index][ledblk] = LED_EFFECT_FADING;
                        }
                        
                        wdt_reset();
                        sendString(sledblk[ledblk-5]);
                        
                        wdt_reset();
                        sendString(sledmode[kbdConf.led_preset[kbdConf.led_preset_index][ledblk]]);

                         for (ledblk = LED_PIN_Fx; ledblk <= LED_PIN_WASD; ledblk++)
                         {
                              pwmDir[ledblk ] = 0;
                              pwmCounter[ledblk] = 0;
                             led_mode_change(ledblk, kbdConf.led_preset[kbdConf.led_preset_index][ledblk]);
                         }                    
                     }
               }else
               {
                
               }

               debounceMATRIX[col][row] = -1;
            }
        }
    }
    }
    }
}
#endif

uint8_t led_sleep_preset[3][5] ={LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF
                                , LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF
                                , LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF, LED_EFFECT_OFF};
void led_sleep(void)
{
      LED_BLOCK ledblock;
    led_3lockupdate(0);
    
    for (ledblock = LED_PIN_BASE; ledblock < LED_PIN_WASD; ledblock++)
    {
       pwmDir[ledblock ] = 0;
       pwmCounter[ledblock] = 0;
       led_mode_change(ledblock, led_sleep_preset[0][ledblock]);
    }

}
void led_restore(void)
{
   LED_BLOCK ledblock;

   led_3lockupdate(gLEDstate);

   for (ledblock = LED_PIN_BASE; ledblock < LED_PIN_WASD; ledblock++)
   {
      pwmDir[ledblock ] = 0;
      pwmCounter[ledblock] = 0;
      led_mode_change(ledblock, kbdConf.led_preset[kbdConf.led_preset_index][ledblock]);
   }

}
