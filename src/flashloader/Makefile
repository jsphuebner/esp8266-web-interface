# Note that according to the original GPLed code, compiling is noted to be
# as simple as gcc -c, this fails with my tests where this will lead to a wrong 
# address read by the program.
# This makefile will save your time from dealing with compile errors
# Adjust CC if needed

CC = /opt/local/gcc-arm-none-eabi-8-2018-q4-major/bin/arm-none-eabi-gcc
CFLAGS = -mcpu=Cortex-M3 -Tlinker.ld -ffreestanding -nostdlib

all: stm32f0.o

stm32vl.o: stm32f0.s
	$(CC) stm32f0.s $(CFLAGS)  -o stm32f0.o

clean:
	rm *.o
