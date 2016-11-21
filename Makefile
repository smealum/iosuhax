INPUT := bin/fw.img.full.bin
INPUT_SECTIONS := $(addprefix sections/, $(addsuffix .bin, $(SECTIONS)))
PATCHED_SECTIONS := $(addprefix patched_sections/, $(addsuffix .bin, $(SECTIONS)))

.PHONY: all clean wupserver/wupserver.bin

all: redNAND

redNAND:
	@$(MAKE) SECTIONS="0x10700000 0x10800000 0x8120000 0x5000000 0x5100000 0x8140000 0x4000000 0xE0000000" BSS_SECTIONS="0x10835000 0x5074000 0x8150000" fw.img

cfw:
	@$(MAKE) SECTIONS="0x8120000 0x5000000 0x5100000 0x8140000 0x4000000 0xE0000000" BSS_SECTIONS="0x5074000 0x8150000" fw.img
	
cfw-coldboot:
	@$(MAKE) SECTIONS="0x8120000 0x5000000 0x5100000 0x8140000 0x4000000 0x5060000 0xE0000000" BSS_SECTIONS="0x5074000 0x8150000" fw.img

$(INPUT):
	@cd bin && python getfwimg.py

sections/%.bin: $(INPUT) 
	@mkdir -p sections
	@cd sections && python gensections.py
	@python scripts/anpack.py -in $(INPUT) -e $*,$@

extract: $(INPUT_SECTIONS)

wupserver/wupserver.bin:
	@cd wupserver && make
	
ios_fs/ios_fs.bin:
	@make -C ios_fs

patched_sections/%.bin: sections/%.bin patches/%.s wupserver/wupserver.bin ios_fs/ios_fs.bin
	@mkdir -p patched_sections
	@echo patches/$*.s
	@armips patches/$*.s

patch: $(PATCHED_SECTIONS)

fw.img: $(INPUT) $(PATCHED_SECTIONS)
	@python scripts/anpack.py -in $(INPUT) -out fw.img $(foreach s,$(SECTIONS),-r $(s),patched_sections/$(s).bin) $(foreach s,$(BSS_SECTIONS),-b $(s),patched_sections/$(s).bin)
 
clean:
	@rm -f fw.img
	@rm -f sections/*.bin
	@rm -fr patched_sections
	@make -C wupserver clean
	@make -C ios_fs clean
