ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

CC = arm-none-eabi-gcc
LINK = arm-none-eabi-gcc
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
CFLAGS += -Wall -mbig-endian -std=gnu99 -mcpu=arm926ej-s -msoft-float -mfloat-abi=soft -Os
LDFLAGS += -nostartfiles -nodefaultlibs -mbig-endian -Wl,-T,link.ld

LIBDIRS += -L$(CURDIR)/libs
LIBS += -lgcc

CFILES = $(wildcard source/*.c)
BINFILES = $(wildcard data/*.bin)
OFILES = $(BINFILES:data/%.bin=build/%.bin.o)
OFILES += $(CFILES:source/%.c=build/%.o)
DFILES = $(CFILES:source/%.c=build/%.d)
SFILES = $(wildcard source/*.s)
OFILES += $(SFILES:source/%.s=build/%.o)
PROJECTNAME = ${shell basename "$(CURDIR)"}
CWD = "$(CURDIR)""

#---------------------------------------------------------------------------------
# canned command sequence for binary data, taken from devkitARM
#---------------------------------------------------------------------------------
define bin2o
	bin2s $< | $(AS) -o $(@)
	echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > source/`(echo $(<F) | tr . _)`.h
	echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> source/`(echo $(<F) | tr . _)`.h
	echo "extern const u32" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> source/`(echo $(<F) | tr . _)`.h
endef

.PHONY:=all dirs

all: dirs $(PROJECTNAME).bin $(PROJECTNAME).syms $(PROJECTNAME).text.bin

dirs:
	@mkdir -p build

$(PROJECTNAME).bin: $(PROJECTNAME).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECTNAME).elf: $(OFILES)
	$(LINK) $(LDFLAGS) -o $(PROJECTNAME).elf $(sort $(filter-out build/crt0.o, $(OFILES))) $(LIBDIRS) $(LIBS)

$(PROJECTNAME).syms:
	$(OBJDUMP) -EB -t -marm $(PROJECTNAME).elf | grep 'g     F .text' | grep -v '.hidden' | awk '{print $$6 " equ 0x" $$1}' > $@
	$(OBJDUMP) -EB -t -marm $(PROJECTNAME).elf | grep -e 'g       .text' -e '_bss_' | awk '{print $$5 " equ 0x" $$1}' >> $@

$(PROJECTNAME).text.bin:
	$(OBJCOPY) -j .text -j .rodata -O binary $(PROJECTNAME).elf $@
	
#$(PROJECTNAME).data.bin:
#	$(OBJCOPY) -j .data -O binary $(PROJECTNAME).elf $@

clean:
	@rm -f build/*.o build/*.d
	@rm -f $(PROJECTNAME).elf $(PROJECTNAME).bin $(PROJECTNAME).syms $(PROJECTNAME).text.bin $(PROJECTNAME).data.bin
	@echo "all cleaned up !"

-include $(DFILES)

build/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > build/$*.d

build/%.o: source/%.s
	$(CC) $(CFLAGS) -xassembler-with-cpp -c $< -o $@
	@$(CC) -MM $< > build/$*.d

build/%.bin.o: data/%.bin
	@echo $(notdir $<)
	@$(bin2o)
