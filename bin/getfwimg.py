import os, urllib
from Crypto.Cipher import AES

# insert keys here, your keys were set correctly if the crc32 of the fw.img
# is d674201b and the crc32 of the fw.img.full.bin is 9f2c91ff in the end
wiiu_common_key = "you have to insert this yourself"
starbuck_ancast_key = "you have to insert this yourself"
starbuck_ancast_iv = "you have to insert this yourself"

print "somewhat simple 5.5.1 fw.img downloader"

#prepare keys
wiiu_common_key = wiiu_common_key.decode('hex')
starbuck_ancast_key = starbuck_ancast_key.decode('hex')
starbuck_ancast_iv = starbuck_ancast_iv.decode('hex')

print "downloading osv10 cetk"
#download osv10 cetk
urllib.urlretrieve("http://ccs.cdn.wup.shop.nintendo.net/ccs/download/000500101000400A/cetk","cetk")
f = open("cetk","rb")
if(f == None):
    print "cetk download failed!"
    exit()

#get cetk encrypted key
f.seek(0x1BF,0)
enc_key = f.read(0x10)
f.close()
os.remove("cetk")

#decrypt cetk key using wiiu common key
iv="000500101000400A0000000000000000"
iv=iv.decode("hex")
cipher = AES.new(wiiu_common_key, AES.MODE_CBC,iv)
dec_key = cipher.decrypt(enc_key)

print "downloading fw.img"
#download encrypted 5.5.1 fw img
urllib.urlretrieve("http://ccs.cdn.wup.shop.nintendo.net/ccs/download/000500101000400A/0000136e","0000136e")
f = open("0000136e","rb")
if(f == None):
    print "0000136e download failed!"
    exit()
f.close()

print "decrypt first"
#decrypt fw img with our decrypted key
buffer = ""
f = open("0000136e","rb")
fout = open("fw.img","wb")
iv = "00090000000000000000000000000000"
iv = iv.decode("hex")
cipher = AES.new(dec_key, AES.MODE_CBC, iv)
while True:
        dec = f.read(0x4000)
        if len(dec) < 0x10:
                break
        enc = cipher.decrypt(dec)
        fout.write(enc)
f.close()
fout.close()
os.remove("0000136e")

print "decrypt second"
#decrypt ancast image with ancast key and iv
f = open("fw.img","rb")
fout = open("fw.img.full.bin","wb")
fout.write(f.read(0x200))
cipher = AES.new(starbuck_ancast_key, AES.MODE_CBC, starbuck_ancast_iv)
while True:
        dec = f.read(0x4000)
        if len(dec) < 0x10:
                break
        enc = cipher.decrypt(dec)
        fout.write(enc)
f.close()
fout.close()

print "done!"
