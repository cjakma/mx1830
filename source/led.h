
#define LED_PORT		PORTD
#define LED_DDR			DDRD


#define LED_NUM_PIN			(1 << 0)
#define LED_CAP_PIN			(1 << 1)
#define LED_SCR_PIN			(1 << 5)
#define LED_FULL_PIN		(1 << 4)
#define LED_ALL_PIN         (LED_NUM_PIN | LED_CAP_PIN | LED_SCR_PIN | LED_FULL_PIN)


#define LED_NUM     0x01  ///< num LED on a boot-protocol keyboard
#define LED_CAPS    0x02  ///< caps LED on a boot-protocol keyboard
#define LED_SCROLL  0x04  ///< scroll LED on a boot-protocol keyboard
#define LED_COMPOSE 0x08  ///< compose LED on a boot-protocol keyboard
#define LED_KANA    0x10  ///< kana LED on a boot-protocol keyboard



#define ledOn(pin)          LED_PORT |= (pin);
#define ledOff(pin)         LED_PORT &= ~(pin);
#define ledEnable(pin)      LED_DDR  |= (pin);
#define ledDisable(pin)     LED_DDR  &= ~(pin);

typedef enum
{
    LED_FADING          = 0,
    LED_FADING_PUSH_ON  = 1,
    LED_PUSHED_LEVEL    = 2,
    LED_PUSH_ON         = 3,
    LED_PUSH_OFF        = 4,
    LED_ALWASY          = 5,
    LED_FULLCAPS        = 6,
    LED_OFF
}LED_MODE;

extern LED_MODE led_mode;
extern uint8_t LEDstate;     ///< current state of the LEDs


void led_blink(int matrixState);
void led_fader(void);
void led_check(uint8_t forward);
void led_mode_init(void);
void led_mode_change (int mode);
void led_pushed_level_cal(void);
