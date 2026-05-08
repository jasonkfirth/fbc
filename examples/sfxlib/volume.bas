'' VOLUME example

print "volume() before ="; volume()
print "Playing at the current global volume."
sound 440, 0.25
sleep 350

volume 0.50
print "volume() after  ="; volume()
print "Playing the same tone at half global volume."
sound 440, 0.25
sleep 350

volume 1, 0.25
print "volume(1)      ="; volume(1)
print "Playing channel 1 with its own lower volume."
sound 1, 440, 0.20, 1.00
sleep 300
