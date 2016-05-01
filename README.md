# iosuhax

this repo contains some of the tools I wrote when working on the wii u. iosuhax is essentially a set of patches for IOSU which provides extra features which can be useful for developers. I'm releasing this because I haven't really touched it since the beginning of january and don't plan on getting back to it.

this would not have been possible without the support of plutoo, yellows8, naehrwert and derrek.

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
