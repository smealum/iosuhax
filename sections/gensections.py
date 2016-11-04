#!/usr/bin/python

with open('../bin/fw.img.full.bin', 'rb') as infw:
	with open('0x5100000.bin', 'wb') as fout:
		infw.seek(0x89F1C, 0)
		fout.write(infw.read(0x15D6C))

	with open('0x8140000.bin', 'wb') as fout:
		infw.seek(0xB4C88, 0)
		fout.write(infw.read(0x2478))
