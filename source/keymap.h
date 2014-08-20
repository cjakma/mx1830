#ifndef KEYMAP_H
#define KEYMAP_H

#include <avr/pgmspace.h>

#define MX3018
//#define PS2AVR

#define MAX_COL     8

#ifdef MX3018
#define MAX_ROW     13
#endif

#ifdef PS2AVR
#define MAX_ROW     17
#endif


#define MAX_LAYER   3

// Total 132 keys + one none
#define NUM_KEY         172

#define KEY_FN          255
#define KEY_LED         254
#define KEY_BEYOND_FN   253


/// Codes for modifier-keys.
typedef enum modifiers {
    MOD_NONE          = 0,
    MOD_CONTROL_LEFT  = (1 << 0),
    MOD_SHIFT_LEFT    = (1 << 1),
    MOD_ALT_LEFT      = (1 << 2),
    MOD_GUI_LEFT      = (1 << 3),
    MOD_CONTROL_RIGHT = (1 << 4),
    MOD_SHIFT_RIGHT   = (1 << 5),
    MOD_ALT_RIGHT     = (1 << 6),
    MOD_GUI_RIGHT     = (1 << 7),
} MODIFIERS;


enum {
	KEY_NONE=0,
    ErrorRollOver,
    POSTFail,
    ErrorUndefined,

    KEY_A,                // 0x04
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,

    KEY_M,                // 0x10
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_1,                //       1 and !
    KEY_2,                //       2 and @

    KEY_3,                // 0x20  3 and #
    KEY_4,                //       4 and $
    KEY_5,                //       5 and %
    KEY_6,                //       6 and ^
    KEY_7,                //       7 and &
    KEY_8,                //       8 and *
    KEY_9,                //       9 and (
    KEY_0,                // 0x27  0 and )
    KEY_ENTER,           // 0x28  enter
    KEY_ESC,           // 0x29
    KEY_BKSP,           // 0x2A  backspace
    KEY_TAB,              // 0x2B
    KEY_SPACE,         // 0x2C
    KEY_MINUS,            // 0x2D  - and _
    KEY_EQUAL,           // 0x2E  = and +
    KEY_LBR,         // 0x2F  [ and {

    KEY_RBR,         // 0x30  ] and }
    KEY_BKSLASH,        // 0x31  \ and |
    KEY_Europe1,             // 0x32  non-US # and ~
    KEY_COLON,        // 0x33  ; and :
    KEY_QUOTE,        // 0x34  ' and "
    KEY_HASH,            // 0x35  grave accent and tilde
    KEY_COMMA,            // 0x36  , and <
    KEY_DOT,              // 0x37  . and >
    KEY_SLASH,            // 0x38  / and ?
    KEY_CAPS,         // 0x39
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,

    KEY_F7,               // 0x40
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_PRNSCR,
    KEY_SCRLCK,
    KEY_PAUSE,            //Break
    KEY_INSERT,
    KEY_HOME,
    KEY_PGUP,
    KEY_DEL,
    KEY_END,
    KEY_PGDN,
    KEY_RIGHT,
    
    KEY_LEFT,        // 0x50
    KEY_DOWN,
    KEY_UP,
    KEY_NUMLOCK,          //       Clear
    KEY_KP_SLASH,
    KEY_KP_AST,
    KEY_KP_MINUS,
    KEY_KP_PLUS,
    KEY_KP_ENTER,
    KEY_KP_1,              //       End
    KEY_KP_2,              //       Down Arrow
    KEY_KP_3,              //       Page Down
    KEY_KP_4,              //       Left Arrow
    KEY_KP_5,
    KEY_KP_6,              //       Right Arrow
    KEY_KP_7,              //       Home
    
    KEY_KP_8,              // 0x60  Up Arrow
    KEY_KP_9,              //       Page Up
    KEY_KP_0,              //       Insert
    KEY_KP_DOT,             //       Delete
    KEY_Europe2,             //       non-US \ and |
    KEY_APPS,		        // 102

    KEY_POWER_HID,
    KEY_KP_EQUAL,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,

    KEY_F21,        // 0x70
    KEY_F22,
    KEY_F23,
    KEY_F24,
    KEY_EXECUTE,
    KEY_HELP,
    KEY_MENU,
    KEY_SEL,
    KEY_STOP_HID,
    KEY_AGAIN,
    KEY_UNDO,
    KEY_CUT,
    KEY_COPY,
    KEY_PASTE,
    KEY_FIND,
    KEY_MUTE_HID,
    
    KEY_VOLUP,
    KEY_VOLDN,
    KEY_KL_CAP,
    KEY_KL_NUM,
    KEY_KL_SCL,

	 /* These are NOT standard USB HID - handled specially in decoding, 
     so they will be mapped to the modifier byte in the USB report */
	KEY_Modifiers,
	KEY_LCTRL,    // 0x01
	KEY_LSHIFT,   // 0x02
	KEY_LALT,     // 0x04
	KEY_LGUI,     // 0x08
	KEY_RCTRL,    // 0x10
	KEY_RSHIFT,   // 0x20
	KEY_RALT,     // 0x40
	KEY_RGUI,     // 0x80
	KEY_Modifiers_end,


	KEY_HANJA,	KEY_HANGLE,	KEY_POWER,	KEY_SLEEP,	KEY_WAKE,	KEY_EMAIL,
 	KEY_WWW_SEARCH, KEY_WWW_HOME,	KEY_WWW_BACK,	KEY_WWW_FORWARD,	KEY_WWW_STOP,	KEY_WWW_REFRESH,	KEY_WWW_FAVORITE,	KEY_NEXT_TRK,	KEY_PREV_TRK,	KEY_STOP,
 	KEY_PLAY,	KEY_MUTE,	KEY_VOL_UP,	KEY_VOL_DOWN,	KEY_MEDIA,	KEY_CALC,	KEY_MYCOM,	KEY_SCREENSAVE,	KEY_REC,	KEY_REWIND,
 	KEY_MINIMIZE,	KEY_EJECT
};


#define KFLA_EXTEND 		0x01
#define KFLA_SPECIAL		0x02
#define KFLA_MAKEONLY		0x04
#define KFLA_MAKE_BREAK		0x08
#define KFLA_PROC_SHIFT		0x10


extern const uint8_t PROGMEM keycode_set2[];
extern uint8_t KFLA[];
extern const uint8_t PROGMEM keycode_set2_special[];
extern const uint8_t PROGMEM keycode_set2_makeonly[];
extern const uint8_t PROGMEM keycode_set2_make_break[];
extern const uint8_t PROGMEM keycode_set2_extend[];
extern const uint8_t PROGMEM keycode_set2_proc_shift[];
extern const uint8_t PROGMEM keymap_code[MAX_LAYER][MAX_ROW][MAX_COL];


#endif
