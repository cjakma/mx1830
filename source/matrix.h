#define SCAN_DIRTY          0x01 << 2
#define SCAN_PUSHED         0x01 << 1
#define SCAN_RELEASED       0x01 << 0
#define SCAN_CHANGED        (SCAN_PUSHED | SCAN_RELEASED)

#define DEBOUNCE_MAX        10           // 5ms at 12MHz XTAL


extern void keymap_init(void);
extern uint8_t processFNkeys(uint8_t keyidx);
extern uint8_t getLayer(uint8_t FNcolrow);
extern uint8_t scankey(void);
