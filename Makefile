# Top-level Makefile — builds bootloader, application, and packed image.

.PHONY: all bootloader app pack clean flash-bl flash-app

all: bootloader app pack

bootloader:
	$(MAKE) -C bootloader

app:
	$(MAKE) -C app

pack: app
	python3 tools/pack_image.py app/build/application.bin app/build/application_packed.bin

clean:
	$(MAKE) -C bootloader clean
	$(MAKE) -C app clean

# Flash via OpenOCD + ST-Link
flash-bl: bootloader
	openocd -f openocd.cfg \
	  -c "program bootloader/build/bootloader.bin 0x08000000 verify reset exit"

flash-app: pack
	openocd -f openocd.cfg \
	  -c "program app/build/application_packed.bin 0x08010000 verify reset exit"
