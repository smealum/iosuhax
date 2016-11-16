ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

CC = arm-none-eabi-gcc
# LINK = arm-none-eabi-gcc
LINK = arm-none-eabi-ld
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy
CFLAGS += -Wall -mbig-endian -std=c99 -march=armv5 -Os -I$(DEVKITPRO)/libnds/include
LDFLAGS += --script=ccd00.ld -EB -L"$(DEVKITARM)/arm-none-eabi/lib"

CFILES = $(wildcard source/*.c)
BINFILES = $(wildcard data/*.*)
OFILES = $(CFILES:source/%.c=build/%.o)
DFILES = $(CFILES:source/%.c=build/%.d)
SFILES = $(wildcard source/*.s)
OFILES += $(SFILES:source/%.s=build/%.o)
OFILES += $(BINFILES:data/%=build/%.o)
PROJECTNAME = ${shell basename "$(CURDIR)"}
CWD = "$(CURDIR)""

#---------------------------------------------------------------------------------
# canned command sequence for binary data, taken from devkitARM
#---------------------------------------------------------------------------------
define bin2o
	bin2s $< | $(AS) -EB -o $(@)
endef

.PHONY:=all dirs

all: dirs $(PROJECTNAME).bin

dirs:
	@mkdir -p build

$(PROJECTNAME).bin: $(PROJECTNAME).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECTNAME).elf: $(OFILES)
	$(LINK) $(LDFLAGS) -o $(PROJECTNAME).elf $(sort $(filter-out build/crt0.o, $(OFILES)))

clean:
	@rm -f *.map
	@rm -f build/*.o build/*.d
	@rm -f $(PROJECTNAME).elf $(PROJECTNAME).bin
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

build/%.tga.o: data/%.tga
	@echo $(notdir $<)
	@$(bin2o)
