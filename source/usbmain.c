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
#include <string.h>

#include "usbdrv.h"
#include "matrix.h"


uint8_t interfaceReady = 0;
extern int8_t usbmode;
extern unsigned char matrixFN;           // (col << 4 | row)

MODIFIERS modifierBitmap[] = {
    MOD_NONE ,
    MOD_CONTROL_LEFT ,
    MOD_SHIFT_LEFT ,
    MOD_ALT_LEFT ,
    MOD_GUI_LEFT ,
    MOD_CONTROL_RIGHT ,
    MOD_SHIFT_RIGHT ,
    MOD_ALT_RIGHT ,
    MOD_GUI_RIGHT
};

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

static uint8_t reportBuffer[8]; ///< buffer for HID reports
static uint8_t oldReportBuffer[8]; ///< buufer for HID reports save on Overflower
uint8_t reportIndex; // reportBuffer[0] contains modifiers

static uint8_t idleRate = 0;        ///< in 4ms units
static uint8_t protocolVer = 1; ///< 0 = boot protocol, 1 = report protocol
uint8_t expectReport = 0;       ///< flag to indicate if we expect an USB-report


/** USB report descriptor (length is defined in usbconfig.h). The report
 * descriptor has been created with usb.org's "HID Descriptor Tool" which can
 * be downloaded from http://www.usb.org/developers/hidpage/ (it's an .exe, but
 * it even runs under Wine).
 */
char PROGMEM const usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01,   // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,   // USAGE (Keyboard)
    0xa1, 0x01,   // COLLECTION (Application)
    0x05, 0x07,   //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,   //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,   //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,   //   LOGICAL_MINIMUM (0)
    0x25, 0x01,   //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,   //   REPORT_SIZE (1)
    0x95, 0x08,   //   REPORT_COUNT (8)
    0x81, 0x02,   //   INPUT (Data,Var,Abs)
    0x95, 0x01,   //   REPORT_COUNT (1)
    0x75, 0x08,   //   REPORT_SIZE (8)
    0x81, 0x03,   //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,   //   REPORT_COUNT (5)
    0x75, 0x01,   //   REPORT_SIZE (1)
    0x05, 0x08,   //   USAGE_PAGE (LEDs)
    0x19, 0x01,   //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,   //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,   //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,   //   REPORT_COUNT (1)
    0x75, 0x03,   //   REPORT_SIZE (3)
    0x91, 0x03,   //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,   //   REPORT_COUNT (6)
    0x75, 0x08,   //   REPORT_SIZE (8)
    0x15, 0x00,   //   LOGICAL_MINIMUM (0)
    0x25, 0x65,   //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,   //   USAGE_PAGE (Keyboard)
    0x19, 0x00,   //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,   //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,   //   INPUT (Data,Ary,Abs)
    0xc0          // END_COLLECTION
};

//#define usbDeviceConnect()      (USBDDR &= ~(1<<USBMINUS))
//#define usbDeviceDisconnect()   (USBDDR |= (1<<USBMINUS))


/**
 * This function is called whenever we receive a setup request via USB.
 * \param data[8] eight bytes of data we received
 * \return number of bytes to use, or 0xff if usbFunctionWrite() should be
 * called
 */
uint8_t usbFunctionSetup(uint8_t data[8]) {
    usbRequest_t *rq = (void *)data;

    
	interfaceReady = 1;

    usbMsgPtr = reportBuffer;
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        // class request type
        if (rq->bRequest == USBRQ_HID_GET_REPORT) {
            // wValue: ReportType (highbyte), ReportID (lowbyte)
            // we only have one report type, so don't look at wValue
            return sizeof(reportBuffer);
        } else if (rq->bRequest == USBRQ_HID_SET_REPORT) {
            if (rq->wLength.word == 1) {
                // We expect one byte reports
                expectReport = 1;
                return 0xff; // Call usbFunctionWrite with data
            }
        } else if (rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = &idleRate;
            return 1;
        } else if (rq->bRequest == USBRQ_HID_SET_IDLE) {
            idleRate = rq->wValue.bytes[1];
            DEBUG_PRINT(("idleRate = %2x\n", idleRate));
        } else if (rq->bRequest == USBRQ_HID_GET_PROTOCOL) {
            if (rq->wValue.bytes[1] < 1) {
                protocolVer = rq->wValue.bytes[1];
            }
        } else if(rq->bRequest == USBRQ_HID_SET_PROTOCOL) {
            usbMsgPtr = &protocolVer;
            return 1;
        }
    } else {
        // no vendor specific requests implemented
    }
    return 0;
}

/**
 * The write function is called when LEDs should be set. Normally, we get only
 * one byte that contains info about the LED states.
 * \param data pointer to received data
 * \param len number ob bytes received
 * \return 0x01
 */
uint8_t usbFunctionWrite(uchar *data, uchar len) {
    if (expectReport && (len == 1)) {
        LEDstate = data[0]; // Get the state of all 5 LEDs
        led_3lockupdate(LEDstate);
    }
    expectReport = 0;
    return 0x01;
}

/**
 * Send a single report to the computer. This function is not used during
 * normal typing, it is only used to send non-pressed keys to simulate input.
 * \param mode modifier-byte
 * \param key key-code
 */
void usbSendReport(uint8_t mode, uint8_t key) {
    // buffer for HID reports. we use a private one, so nobody gets disturbed
    uint8_t repBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    repBuffer[0] = mode;
    repBuffer[2] = key;
    while (!usbInterruptIsReady()); // wait
    usbSetInterrupt(repBuffer, sizeof(repBuffer)); // send
}

/**
 * This structure can be used as a container for a single 'key'. It consists of
 * the key-code and the modifier-code.
 */
typedef struct {
    uint8_t mode;
    uint8_t key;
} Key;

/**
 * Convert an ASCII-character to the corresponding key-code and modifier-code
 * combination.
 * \parm character ASCII-character to convert
 * \return structure containing the combination
 */
Key charToKey(char character) {
    Key key;
    // initialize with reserved values
    key.mode = MOD_NONE;
    key.key = KEY_NONE;
    if ((character >= 'a') && (character <= 'z')) {
        // a..z
        key.key = (character - 'a') + 0x04;
    } else if ((character >= 'A') && (character <= 'Z')) {
        // A..Z
        key.mode = MOD_SHIFT_LEFT;
        key.key = (character - 'A') + 0x04;
    } else if ((character >= '1') && (character <= '9')) {
        // 1..9
        key.key = (character - '1') + 0x1E;
    }
    // we can't map the other characters directly, so we switch...
    switch (character) {
        case '0':
            key.key = KEY_0; break;
        case '!':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_1; break;
        /*
        case '@':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_2; break;
        case '#':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_3; break;
        */
        case '$':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_4; break;
        case '%':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_5; break;
        case '^':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_6; break;
        case '&':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_7; break;
        case '*':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_8; break;
        case '(':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_9; break;
        case ')':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_0; break;
        case ' ':
            key.key = KEY_SPACE; break;
        case '-':
            key.key = KEY_MINUS; break;
        case '_':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_MINUS; break;
        case '=':
            key.key = KEY_EQUAL; break;
        case '+':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_EQUAL; break;
        case '[':
            key.key = KEY_LBR; break;
        case '{':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_LBR; break;
        case ']':
            key.key = KEY_RBR; break;
        case '}':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_RBR; break;
        case '\\':
            key.key = KEY_BKSLASH; break;
        case '|':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_BKSLASH; break;
        case '#':
            key.key = KEY_HASH; break;
        case '@':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_HASH; break;
        case ';':
            key.key = KEY_COLON; break;
        case ':':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_COLON; break;
#if 0
        case '\'':
            key.key = KEY_apostroph; break;
        case '"':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_apostroph; break;
#endif            
        case '`':
            key.key = KEY_HASH; break;
        case '~':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_HASH; break;
        case ',':
            key.key = KEY_COMMA; break;
        case '<':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_COMMA; break;
        case '.':
            key.key = KEY_DOT; break;
        case '>':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_DOT; break;
        case '/':
            key.key = KEY_SLASH; break;
        case '?':
            key.mode = MOD_SHIFT_LEFT;
            key.key = KEY_SLASH; break;
    }
    if (key.key == KEY_NONE) {
        // still reserved? WTF? return question mark...
        key.mode = MOD_SHIFT_LEFT;
        key.key = KEY_SLASH;
    }
    return key;
} 


/**
 * Send a key to the computer, followed by the release of all keys. This can be
 * used repetitively to send a string.
 * \param keytosend key structure to send
 */
void sendKey(Key keytosend) {
    usbSendReport(keytosend.mode, keytosend.key);
    usbSendReport(0, 0);
}

/**
 * Send a string to the computer. This function converts each character of an
 * ASCII-string to a key-structure and uses sendKey() to send it.
 * \param string string to send
 */
void sendString(char* string) {
    uint8_t i;

    for (i = 0; i < strlen(string); i++) {
        Key key = charToKey(string[i]);
        sendKey(key);
    }
}




uint8_t clearReportBuffer(void)
{   
    reportIndex = 2; // reportBuffer[0] contains modifiers
    memset(reportBuffer, 0, sizeof(reportBuffer)); // clear report
    return 0;
}

uint8_t saveReportBuffer(void)
{
    memcpy(oldReportBuffer, reportBuffer, sizeof(reportBuffer));
    return 0;
}

uint8_t restoreReportBuffer(void)
{
    memcpy(reportBuffer, oldReportBuffer, sizeof(reportBuffer));
    return 0;
}


uint8_t bufcmp(uint8_t *s1, uint8_t *s2, uint8_t size)
{
    uint8_t result = 0;
    uint8_t i;
    
    for (i = 0; i < size; i++)
    {
        if(*s1 != *s2)
        {
            result = 1;
            break;
        }
        s1++;
        s2++;
        
    }
    return result;
}


uint8_t cmpReportBuffer()
{
    uint8_t result = 0;
    uint8_t i;
    uint8_t *s1, *s2;
    uint8_t size;

    s1 = reportBuffer;
    s2 = oldReportBuffer;
    size = sizeof(reportBuffer);
    result = bufcmp(s1, s2, size);
    
    return result;
}


uint8_t usbRollOver = 0;


uint8_t buildHIDreports(uint8_t keyidx)
{
    uint8_t retval = 0;
   
    if((keyidx > KEY_Modifiers) && (keyidx < KEY_Modifiers_end))
    {
        reportBuffer[0] |= modifierBitmap[keyidx-KEY_Modifiers];
    }else
    {
        if (reportIndex >= sizeof(reportBuffer))
        {   
            // too many keycodes
            memset(reportBuffer+2, ErrorRollOver, sizeof(reportBuffer)-2);
            retval |= 0x02;                                                             // continue decoding to get modifiers
        }else
        {
            reportBuffer[reportIndex] = keyidx; // set next available entry
            reportIndex++;
        }
        
    }    
    retval |= 0x01; // must have been a change at some point, since debounce is done
    return retval;
}

#ifdef DEBUG
uint8_t toggle1 = 0;

void dumpreportBuffer(void)
{
    uint8_t i;

    DEBUG_PRINT(("RBuf "));
    for (i = 0; i < sizeof(reportBuffer); i++)
    {
        DEBUG_PRINT(("%02x", reportBuffer[i]));
    }
    DEBUG_PRINT(("\n"));
}
#endif
uint8_t usbmain(void) {
    uint8_t updateNeeded = 0;
    uint8_t idleCounter = 0;
    int interfaceCount = 0;
    
	interfaceReady = 0;

    DEBUG_PRINT(("USB\n"));


    cli();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    _delay_ms(10);
    usbDeviceConnect();
    sei();

    wdt_enable(WDTO_1S);


    while (1) {
        // main event loop

        if(interfaceReady == 0 && interfaceCount++ > 4000){
			// move to ps/2
		    usbmode = 0;
			DEBUG_PRINT(("move to ps/2 \n"));
			break;
		}
                
        wdt_reset();
        usbPoll();

        updateNeeded = scankey();   // changes?
        if (updateNeeded == 0)      //debounce
            continue;
        

        if (idleRate == 0)                  // report only when the change occured
        {
            if (cmpReportBuffer() == 0)     // exactly same status?
            {
                updateNeeded = 0;
#ifdef DEBUG   
                if (toggle1 == 0)
                {
                    DEBUG_PRINT(("samebuffer\n"));
                    dumpreportBuffer();
                    toggle1 = 1;
                }
               
            }else{
                DEBUG_PRINT(("NOTsamebuffer\n"));
                dumpreportBuffer();
                toggle1 = 0;
#endif 
            }
        }

        if (TIFR & (1 << TOV0)) {
            TIFR = (1 << TOV0); // reset flag
            if (idleRate != 0) { // do we need periodic reports?
                if(idleCounter > 4){ // yes, but not yet
                    idleCounter -= 5; // 22ms in units of 4ms
               } else { // yes, it is time now
                    updateNeeded = 1;
                    idleCounter = idleRate;
                }
            }
        }
        // if an update is needed, send the report
       
        if(updateNeeded  && usbInterruptIsReady())
        {
            updateNeeded = 0;
            usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
            saveReportBuffer();
        }
    }

    wdt_disable();
	USB_INTR_ENABLE &= ~(1 << USB_INTR_ENABLE_BIT);
    return 0;
}
