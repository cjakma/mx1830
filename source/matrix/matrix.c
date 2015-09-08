#define KEYBD_EXTERN
#include "global.h"
#include "timer.h"
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

#include "hwport.h"
#include "usbdrv.h"
#include "matrix.h"
#include "usbmain.h"
#include "hwaddress.h"
#include "macro.h"

#include "ps2main.h"
//#include "tinycmdapi.h"

uint32_t scankeycntms;
	
// 17*8 bit matrix
uint32_t MATRIX[VIRTUAL_MAX_ROW];
uint32_t curMATRIX[VIRTUAL_MAX_ROW];
int8_t debounceMATRIX[VIRTUAL_MAX_ROW][VIRTUAL_MAX_COL];
uint8_t svlayer;
uint8_t currentLayer[VIRTUAL_MAX_ROW][VIRTUAL_MAX_COL];
uint8_t matrixFN[MAX_LAYER];           // (col << 4 | row)
uint8_t kbdsleepmode = 0;              // 1 POWERDOWN, 2 LED SLEEP
uint16_t macrokeypushedcnt;
uint16_t ledkeypushedcnt;
uint16_t macroresetcnt;
uint16_t winkeylockcnt;
uint16_t keylockcnt;
uint8_t keylock = 0;

uint16_t cntLcaps = 0;
uint16_t cntLctrl = 0;
uint16_t cntLAlt = 0;
uint16_t cntLGui = 0;

int8_t isFNpushed = 0;
uint8_t fn_col;
uint8_t fn_row;

static uint8_t gDirty;



static uint8_t findFNkey(void)
{
   uint8_t col, row;
   uint8_t keyidx;
   uint8_t i;
   for(i = 0; i < MAX_LAYER; i++)
   {
      eeprom_read_block(currentLayer, EEP_KEYMAP_ADDR(i), sizeof(currentLayer));
      matrixFN[i] = 0xff;
      for(row=0;row<MATRIX_MAX_ROW;row++)
      {
         for(col=0;col<MATRIX_MAX_COL;col++)
         {
            keyidx = currentLayer[row][col];
            if(keyidx == K_FN)
            {
               matrixFN[i] = row << 5 | col;
            }
         }
      }

   }
   fn_col = 0xff;
   fn_row = 0xff;
   eeprom_read_block(currentLayer, EEP_KEYMAP_ADDR(kbdConf.keymapLayerIndex), sizeof(currentLayer));
   return 0;
}


void keymap_init(void) 
{
	int16_t i, j, keyidx;

	for(i=0; i<VIRTUAL_MAX_ROW; i++)
		MATRIX[i]=0;


	for(i=0;i<VIRTUAL_MAX_ROW;i++)
	{
        for(j=0;j<VIRTUAL_MAX_COL;j++)
        {
            debounceMATRIX[i][j] = -1;
        }
        curMATRIX[i] = 0;
	}

    findFNkey();
}


uint8_t processPushedFNkeys(uint8_t keyidx)
{
    uint8_t retVal = 0;
    
    if(keyidx >= K_LED0 && keyidx <= K_LED3)
    {
        retVal = 1;
    }else if(keyidx >= K_LFX && keyidx <= K_LARR)
    {
        retVal = 1;
    }else if(keyidx >= K_L0 && keyidx <= K_L6)
    {
        kbdConf.keymapLayerIndex = keyidx - K_L0;
        updateConf();
        keymap_init();
        retVal = 1;
    }else if(keyidx >= K_M01 && keyidx <= K_M52)
    {
        retVal = 1;
    }else if(keyidx == K_RESET)
    {
        DDRD |= 0x0C;
        PORTD &= 0xF3;  // Drive D+. D- to low
        
        Reset_AVR();
    }else if(keyidx == K_MRESET)
    {
        retVal = 1;
    }else if(keyidx == K_KEYLOCK)
    {
         keylock ^= 0x02;
         retVal = 1;
    }else if(keyidx == K_WINKEYLOCK)
    {
         keylock ^= 0x01;
         retVal = 1;
    }
    return retVal;
}

uint8_t processReleasedFNkeys(uint8_t keyidx)
{
    uint8_t retVal = 0;
    uint8_t ledblock;
        
    if(keyidx >= K_LED0 && keyidx <= K_LED3)
    {

        led_mode_change(LED_PIN_BASE, kbdConf.led_preset[kbdConf.led_preset_index][LED_PIN_BASE]++);
        updateConf();
        retVal = 1;
    }else if(keyidx >= K_LFX && keyidx <= K_LARR)
    {
        retVal = 1;
    }else if(keyidx >= K_M01 && keyidx <= K_M52)
    {
        if(kbdConf.ps2usb_mode)
             playMacroUSB(keyidx);
         else
             playMacroPS2(keyidx);
         retVal = 1;
    }else if(keyidx == K_RESET)
    {
        Reset_AVR();
    }else if(keyidx == K_MRESET)
    {
        retVal = 1;
    }
    return retVal;
}

uint32_t scanRow(uint8_t row)
{
    uint8_t vPinA, vPinB, vPinC;
    uint32_t rowValue, vPintmp;

   rowValue = 0;
    // Col -> set only one port as input and all others as output low
   DDRB  = BV(row);        //  only target col bit is output and others are input
   PORTB = ~BV(row);       //  only target col bit is LOW and other are pull-up

   _delay_us(10);

   vPinA = ~PINA;
   vPinC = ~PINC;
   vPintmp = (uint16_t)((vPinC >> 1 & 0x08) |  (vPinC >> 3 & 0x04) |  (vPinC >> 5 & 0x02) | (vPinC >> 7 & 0x01) | (vPinC << 1 & 0x10));
   rowValue = (vPintmp << 8) | (uint16_t)vPinA;

    return rowValue;
}

uint8_t getLayer(uint8_t FNcolrow)
{
   uint32_t tmp;

   if(FNcolrow != 0xFF)
   {
      fn_row = (FNcolrow >> 5) & 0x07;
      fn_col = FNcolrow & 0x1f;

      tmp = scanRow(fn_row);

      if((tmp >> fn_col) & 0x00000001)
      {
         isFNpushed = (uint8_t)kbdConf.matrix_debounce*2;
         return KEY_LAYER_FN;                // FN layer
      }
      else
      {
         if(isFNpushed)
         {
            return KEY_LAYER_FN;             // FN layer
         }else
         {
            return kbdConf.keymapLayerIndex; // Normal layer
         }
      }
   }else
   {
      return kbdConf.keymapLayerIndex;       // Normal layer
   }
}


uint8_t scanmatrix(void)
{
   uint8_t row;
   uint8_t matrixState = 0;
   uint32_t tmpMatrix, tmpRaw;    
   uint8_t i;
	// scan matrix 
	for(row=0; row<VIRTUAL_MAX_ROW; row++)
	{
      curMATRIX[row] = scanRow(row);
       
      if(curMATRIX[row])
      {
         matrixState |= SCAN_DIRTY;
      }
 	}
   
   tmpMatrix = scanRow(6);
   tmpRaw = tmpMatrix << 13;
   curMATRIX[0] |= (tmpRaw & 0x000FE000);
   
   tmpRaw = tmpMatrix << 6; 
   curMATRIX[1] |= (tmpRaw & 0x0007E000);


   tmpMatrix = scanRow(7);  
   
   tmpRaw = tmpMatrix << 13;
   curMATRIX[2] |= (tmpRaw & 0x000FE000);
   
   tmpRaw = tmpMatrix << 6; 
   curMATRIX[3] |= (tmpRaw & 0x0007E000);
    
    return matrixState;
}


uint8_t cntKey(uint8_t keyidx, uint8_t clearmask)
{
    switch (keyidx)
    {
        case K_CAPS:
            if (clearmask == 0)
            {
                kbdConf.swapCtrlCaps |= 0x80;
                cntLcaps = 0;
            }
            if (cntLcaps++ >= SWAP_TIMER)
                cntLcaps = SWAP_TIMER;
            break;
        case K_LCTRL:
            if (clearmask == 0)
            {
                kbdConf.swapCtrlCaps |= 0x80;
                cntLctrl = 0;
            }
            if (cntLctrl++ >= SWAP_TIMER)
                cntLctrl = SWAP_TIMER;
            break;
        case K_LALT:
            if (clearmask == 0)
            {
                kbdConf.swapAltGui |= 0x80;
                cntLAlt = 0;
            }
            if (cntLAlt++ >= SWAP_TIMER)
                cntLAlt = SWAP_TIMER;
            break;
        case K_LGUI:
            if (clearmask == 0)
            {
                kbdConf.swapAltGui |= 0x80;
                cntLGui = 0;
            }
            if (cntLGui++ >= SWAP_TIMER)
                cntLGui = SWAP_TIMER;
            break;
    }
    if((cntLcaps == SWAP_TIMER) && (cntLctrl == SWAP_TIMER) && (kbdConf.swapCtrlCaps & 0x80))
    {
        kbdConf.swapCtrlCaps ^= 1;
        kbdConf.swapCtrlCaps &= 0x7F;
        updateConf();
    }
    if((cntLGui == SWAP_TIMER) && (cntLAlt == SWAP_TIMER) && (kbdConf.swapAltGui & 0x80))
    {
        kbdConf.swapAltGui ^= 1;
        kbdConf.swapAltGui &= 0x7F;
        updateConf();
    }
    if(keyidx >= K_M01 && keyidx <= K_M48)
    {
         if(clearmask == 0x0000)
        {
            macrokeypushedcnt = 0;
        }
        if (macrokeypushedcnt++ == SWAP_TIMER)
        {
            recordMacro(keyidx);
            macrokeypushedcnt = 0;
        }
        
    }
#if 0    
    else if (keyidx >= K_LED0 && keyidx <= K_LED3)
    {
        if(clearmask == 0x0000)
        {
            ledkeypushedcnt = 0;
        }
        if (ledkeypushedcnt++ == SWAP_TIMER)
        {
            recordLED(keyidx);
            ledkeypushedcnt = 0;
        }
        
    }
#endif    
    else if (keyidx == K_MRESET)
    {
         if(clearmask == 0x0000)
        {
            macroresetcnt = 0;
        }
        if(macroresetcnt++ == SWAP_TIMER)
        {
            resetMacro();
        }
    }
    return 0;
}


uint8_t swap_key(uint8_t keyidx)
{
    if(keylock & 0x02)
    {
      keyidx = K_NONE;
      return keyidx;
    }
    if(kbdConf.swapCtrlCaps & 0x01)
    {
        if(keyidx == K_CAPS)
        {
            keyidx = K_LCTRL;
        }
        else if(keyidx == K_LCTRL)
        {
            keyidx = K_CAPS;
        }
    }
    if(kbdConf.swapAltGui & 0x01)
    {
        if(keyidx == K_LALT)
        {
            keyidx = K_LGUI;
        }
        else if(keyidx == K_LGUI)
        {
            keyidx = K_LALT;
        }
    }
    if (keylock & 0x01)
    {
      if ((keyidx == K_LGUI) || (keyidx == K_RGUI))
         keyidx = K_NONE;
    }
    return keyidx;
}



#if 0//def SUPPORT_TINY_CMD

void testTinyCmd(uint8_t keyidx)
{
/*
    RGB_EFFECT_FADE_BUF,
    RGB_EFFECT_FADE_LOOP,
    RGB_EFFECT_HEARTBEAT_BUF,
    RGB_EFFECT_HEARTBEAT_LOOP,
    RGB_EFFECT_MAX

*/
    switch (keyidx)
    {
        case K_F1:
            tinycmd_rgb_effect_on(FALSE, TRUE);
            tinycmd_rgb_all(0, 0, 0, 0, TRUE);
            break;
        case K_F2:
            tinycmd_rgb_buffer(MAX_RGB_CHAIN, 0, (uint8_t *)kbdConf.rgb_preset, TRUE);
            tinycmd_rgb_effect_on(TRUE, TRUE);
            break;
        case K_F3:
            tinycmd_rgb_set_preset(RGB_EFFECT_BOOTHID, &kbdConf.rgb_effect_param[RGB_EFFECT_BOOTHID], TRUE); // RGB_EFFECT_BOOTHID
            tinycmd_rgb_set_effect(0, TRUE); // RGB_EFFECT_BOOTHID
            break;
        case K_F4:
            tinycmd_rgb_set_preset(RGB_EFFECT_FADE_BUF, &kbdConf.rgb_effect_param[RGB_EFFECT_FADE_BUF], TRUE); // RGB_EFFECT_FADE_BUF
            tinycmd_rgb_set_effect(0, TRUE); // RGB_EFFECT_FADE_BUF
            break;
        case K_F5:
            tinycmd_rgb_set_preset(RGB_EFFECT_FADE_LOOP, &kbdConf.rgb_effect_param[RGB_EFFECT_FADE_LOOP], TRUE); // RGB_EFFECT_FADE_LOOP
            tinycmd_rgb_set_effect(0, TRUE); // RGB_EFFECT_FADE_LOOP
            break;
        case K_F6:
            tinycmd_rgb_set_preset(RGB_EFFECT_HEARTBEAT_BUF, &kbdConf.rgb_effect_param[RGB_EFFECT_HEARTBEAT_BUF], TRUE); // RGB_EFFECT_HEARTBEAT_BUF
            tinycmd_rgb_set_effect(0, TRUE); // RGB_EFFECT_HEARTBEAT_BUF
            break;
        case K_F7:
            tinycmd_rgb_set_preset(RGB_EFFECT_HEARTBEAT_LOOP, &kbdConf.rgb_effect_param[RGB_EFFECT_HEARTBEAT_LOOP], TRUE); // RGB_EFFECT_HEARTBEAT_LOOP
            tinycmd_rgb_set_effect(0, TRUE); // RGB_EFFECT_HEARTBEAT_LOOP
            break;
        case K_F8:
            
            tinycmd_rgb_all(1, 255, 255, 255, TRUE);
            tinycmd_rgb_set_preset(RGB_EFFECT_BASIC, &kbdConf.rgb_effect_param[RGB_EFFECT_BASIC], TRUE); // RGB_EFFECT_BOOTHID
            tinycmd_config(kbdConf.rgb_chain + 1, 100, TRUE);
            break;
        case K_F9:
            
            tinycmd_config(15, 40, TRUE);
            break;
        case K_F10:
            
            tinycmd_config(15, 150, TRUE);
            //tinycmd_led_set_effect(0, TRUE);
            tinycmd_rgb_effect_speed(100, TRUE); // fast
            break;
        case K_F11:
            
            tinycmd_config(15, 250, TRUE);
            //tinycmd_led_set_effect(1, TRUE);
            tinycmd_rgb_effect_speed(400, TRUE); // normal

            break;
        case K_F12:
            
            tinycmd_config(15, 500, TRUE);
            //tinycmd_led_set_effect(2, TRUE);
            tinycmd_rgb_effect_speed(600, TRUE); //slow
            break;
    }
}
#endif // SUPPORT_TINY_CMD


// return : key modified
uint8_t scankey(void)
{
	int8_t col, row;
	uint32_t prev, cur;
    uint8_t prevBit, curBit;
	uint8_t keyidx;
	uint8_t matrixState = 0;
	uint8_t retVal = 0;
   uint8_t t_layer;

    matrixState = scanmatrix();
    if(matrixState != gDirty)
    {
        gDirty = matrixState;
//        tinycmd_dirty(matrixState);
    }

   if (matrixState == 0 && isFNpushed > 0)
   {
      isFNpushed--;
   }

   //static int pushedLevel_prev = 0;

    /* LED Blinker */
    led_blink(matrixState);

    /* LED Fader */
    led_fader();

    clearReportBuffer();
   
	t_layer = getLayer(matrixFN[kbdConf.keymapLayerIndex]);

	// debounce cleared => compare last matrix and current matrix
	for(row = 0; row < VIRTUAL_MAX_ROW; row++)
	{

        prev = MATRIX[row];
        cur  = curMATRIX[row];
        MATRIX[row] = curMATRIX[row];
		for(col = 0; col < VIRTUAL_MAX_COL; col++)
		{
            prevBit = (uint8_t)(prev & 0x01);
            curBit = (uint8_t)(cur & 0x01);
            prev >>= 1;
            cur >>= 1;

            if (reportMatrix == HID_REPORT_MATRIX)
            {
                if(prevBit && !curBit)
                    sendMatrix(row, col);
                continue;
            }


            if(t_layer != KEY_LAYER_FN)
                keyidx = currentLayer[row][col];
            else
                keyidx = eeprom_read_byte(EEP_KEYMAP_ADDR(t_layer)+(row*VIRTUAL_MAX_COL)+col);
            

            if ((keyidx == K_NONE) || ((fn_col == col) && (fn_row == row)))
                continue;

            if(curBit && !(keylock & 0x02))
               cntKey(keyidx, 0xFF);
            
            if (!prevBit && curBit)   //pushed
            {
                led_pushed_level_cal();          /* LED_EFFECT_PUSHED_LEVEL calculate */        
                if (processPushedFNkeys(keyidx))
                    continue;
            }else if (prevBit && !curBit)  //released
            {
                cntKey(keyidx, 0x0000);
                if (processReleasedFNkeys(keyidx))
                    continue;
            }

            keyidx = swap_key(keyidx);

            if ((K_L0 <= keyidx && keyidx <= K_L6) || (K_LED0 <= keyidx && keyidx <= K_FN) || (K_M01 <= keyidx) || (keyidx == K_NONE))
               continue;
            
            if(kbdConf.ps2usb_mode)
            {
                if(curBit)
                {
                    if(debounceMATRIX[row][col]++ >= (uint8_t)kbdConf.matrix_debounce)
                    {
                        retVal = buildHIDreports(keyidx);
                        debounceMATRIX[row][col] = (uint8_t)kbdConf.matrix_debounce*2;
                    }
                }else
                {
                    if(debounceMATRIX[row][col]-- >= (uint8_t)kbdConf.matrix_debounce)
                    {
                        retVal = buildHIDreports(keyidx);
                    }else
                    {
                        debounceMATRIX[row][col] = 0;
                    }
                }
            }else
            {
                if (!prevBit && curBit)   //pushed
                {
                    debounceMATRIX[row][col] = 0;    //triger

                }else if (prevBit && !curBit)  //released
                {
                    debounceMATRIX[row][col] = 0;    //triger
    			   }
                
                if(debounceMATRIX[row][col] >= 0)
                {                
                   if(debounceMATRIX[row][col]++ >= (uint8_t)kbdConf.matrix_debounce)
                   {
                        if(curBit)
                        {
                            putKey(keyidx, 1);
                            svlayer = t_layer;
                        }else
                        {
                            if (keyidx <= K_M01)  // ignore FN keys
                               keyidx = eeprom_read_byte(EEP_KEYMAP_ADDR(svlayer)+(row*VIRTUAL_MAX_COL)+col);
                               keyidx = swap_key(keyidx);
                               putKey(keyidx, 0);
                        }
                                               
                        debounceMATRIX[row][col] = -1;
                   }
  
                }
 		    }
		}
	}
 
    retVal |= 0x05;
	return retVal;
}


