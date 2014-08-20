make clean
pause;
make
pause;
avrdude -c stk500v2 -P com3 -p atmega32 -U flash:w:keymain.hex:i
pause;
