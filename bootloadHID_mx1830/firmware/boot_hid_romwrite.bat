avrdude -c USBasp -P usbasp -p atmega32 -U hfuse:w:0xD0:m -U lfuse:w:0xCF:m
avrdude -c USBasp -P usbasp -p atmega32 -U flash:w:main.hex:i
pause;
