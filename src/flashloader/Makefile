# Note that according to the original GPLed code, compiling is noted to be
# as simple as gcc -c, this fails with my tests where this will lead to a wrong 
# address read by the program.
# This makefile will save your time from dealing with compile errors
# Adjust CC if needed

#CC = /usr/local/share/gcc-arm-none-eabi-9-2019-q4-major/bin/arm-none-eabi-gcc
#OBJCOPY = /usr/local/share/gcc-arm-none-eabi-9-2019-q4-major/bin/arm-none-eabi-objcopy
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=Cortex-M3 -Tlinker.ld -ffreestanding -nostdlib

all: stm32x.o stm32f0.o

%.bin: %.o
	$(OBJCOPY) -O binary $< $@
	rm $<

stm32x.o: stm32x.s
	$(CC) stm32x.s $(CFLAGS)  -o stm32x.o
	make stm32x.bin

stm32f0.o: stm32f0.s
	$(CC) stm32f0.s $(CFLAGS)  -o stm32f0.o
	make stm32f0.bin

clean:
	rm *.bin
