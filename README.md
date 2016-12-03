# iosuhax
We continue to work on this repo the dropped iosuhax project by smea and continue to improve it and make it more user friendly.

# Features this repo offers additionally to the original iosuhax from smea
- improved NAND dump process with reliable dumps
- creating NAND dumps at the beginning of SD card, rest of the SD card is available for other usage
- detection of not existing dump on SD card with user interaction (POWER button) to auto format and prepare SD card for redNAND
- automatic FAT32 partition creation for rest of the available space on SD card
- automatic NAND type detection and dump process adapting to it (8GB or 32GB MLC)
- automatic detection of existing NAND dump and launch of redNAND directly with same fw.img
- tool to extract nand dump from SD to a file or inject from a file to SD card
- install signature checks patched (FIX94)
- boot movie patches (Maschell)
- build of cfw for sysNAND and redNAND
- custom /dev/iosuhax node for access from PPC
- custom splash screen when launching fw.img

# Getting the dump out of SD card or inject a dump file into the SD card
I wrote a windows tool to dump only one single NAND dump image from the SD card to your PC for backup reasons. You can also inject a
dump image back into an SD card with it. If you inject into a fresh SD card be ware to create a FAT32 partition above the necessary
space for the NAND dump (not at the beginning of the SD card). You might need to run the application with administrator rights to be 
able to access the SD card. This depends on your Windows configuration.
Linux and MAC users can use dd to dump or inject the data. (TODO: provide the commands...)

# WARNING (READ THIS)
Be aware that this is still in <b>development</b> and that normal users should not touch this if they don't know exactly what they do.

<b>THIS CAN BRICK YOUR WII U IF NOT USED CORRECTLY!!! THE DEVELOPERS ARE NOT RESPONSIBLE FOR ANY BRICKS YOU DO TO YOUR CONSOLE BY USING THIS SOFTWARE!!!</b>

<b>DON'T UPDATE SYSNAND or REDNAND!!! IT MIGHT BRICK YOUR CONSOLE!</b>

# Credits
- smea
- dimok
- kanye_west
- FIX94
- Maschell

-------------------
# Original README by smea

this repo contains some of the tools I wrote when working on the wii u. iosuhax is essentially a set of patches for IOSU which provides extra features which can be useful for developers. I'm releasing this because I haven't really touched it since the beginning of january and don't plan on getting back to it.

iosuhax does not contain any exploits or information about vulns o anything. just a custom firmware kind of thing.

iosuhax current only supports fw 5.5.x i think.

i wrote all the code here but iosuhax would not have been possible without the support of plutoo, yellows8, naehrwert and derrek.

# features

iosuhax is pretty barebones, it's mainly just the following :
  - software nand dumping (bunch of ways to do this, dumps slc, slccmpt and mlc, either raw or filetree or something in between)
  - redNAND (redirection of nand read/writes to SD card instead of actual NAND chips)
  - remote shell for development and experimentation (cf wupserver and wupclient, it's super useful)
  - some basic ARM debugging stuff (guru meditation screen)

# todo

I don't plan on doing any of this at this point, but the next things I was going to do for this were :
  - make this version-independent by replacing hardcoded offset with auto-located ones, similar to my 3ds *hax ROP stuff
  - put together a menu for settings and nand-dumping so that there's no need to build with different things enabled (menu would have used power/eject buttons)

# how to use

honestly I don't even remember all the details so anyone who's serious about using this will probably have to ask me for help if they can't figure it out, but the gist of it is :
  - decrypt your ancast image, prepend the raw signature header stuff to it and place it in ./bin/fw.img.full.bin
  - open up ./scripts/anpack.py, add your ancast keys in there
  - make

might be missing some steps, especially for building wupserver. you're going to need devkitarm and latest armips (built from armips git probably, pretty sure this relies on some new features I asked Kingcom to add (blame nintendo for doing big endian arm)).

also, fair warning : do NOT blindly use this. read the patches. running this with the wrong options enabled can/will brick your console. this release is oriented towards devs, not end users.

# license

feel free to use any of this code for your own projects, as long as :
  - you give proper credit to the original creator
  - you don't use any of it for commercial purposes
  - you consider sharing your improvements by making the code available (not required but appreciated)

seriously though if you try to make money off this i will fucking end you

have fun !
