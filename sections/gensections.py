
#create missing section bins
f = open("../bin/fw.img.full.bin","rb")
fout = open("0x5100000.bin","wb")
f.seek(0x89f1c,0)
fout.write(f.read(0x15D6C))
fout.close()
f.seek(0xb4c88,0)
fout = open("0x8140000.bin","wb")
fout.write(f.read(0x2478))
fout.close()
f.close()

