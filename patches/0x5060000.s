.arm.big

.open "sections/0x5060000.bin","patched_sections/0x5060000.bin",0x05060000

; force loading of a different system.xml
.org 0x050600DC
.area 0x20
.ascii "/vol/system/config/syshax.xml",0
.endarea

.org 0x050600FC
.area 0x24
.ascii "/vol/system_slc/config/syshax.xml",0
.endarea

.Close
