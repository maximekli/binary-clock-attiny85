DEVICE      = attiny85
CLOCK       = 1000000
PROGRAMMER  = -c arduino -P /dev/ttyUSB0 -b 19200
TARGET		= binary-clock-attiny
OBJECTS     = $(TARGET).o
FUSES       = -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 
AVRDUDE     = avrdude $(PROGRAMMER) -p $(DEVICE)
COMPILE     = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

all: $(TARGET).hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

flash: all
	$(AVRDUDE) -U flash:w:$(TARGET).hex:i

fuse:
	$(AVRDUDE) $(FUSES)

clean:
	rm -f $(TARGET).hex $(TARGET).elf $(OBJECTS)

$(TARGET).elf: $(OBJECTS)
	$(COMPILE) -o $(TARGET).elf $(OBJECTS)

$(TARGET).hex: $(TARGET).elf
	rm -f $(TARGET).hex
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-size --format=avr --mcu=$(DEVICE) $(TARGET).elf