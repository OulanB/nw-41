API=../../api
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
CFLAGS=-DNDEBUG -I$(API) -O2 -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -fno-common -fdata-sections -ffunction-sections -fno-exceptions -Wall
LDFLAGS=-Wl,-L$(API) -Wl,--gc-sections -Wl,--entry=entrypoint --specs=nosys.specs -nostartfiles -Wl,-Ur -lapi

CFLAGS += -I./

OBJS = main.o nut.o lcd.o keys.o scanner.o phi.o

app.elf: $(OBJS)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)
	arm-none-eabi-strip $@

FORCE:

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) app.elf
