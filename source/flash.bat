avrdude -c stk500v2 -P com3 -p atmega32 -U hfuse:w:0xD7:m -U lfuse:w:0x0E:m
avrdude -c stk500v2 -P com3 -p atmega32 -U flash:w:main.hex:i
pause;