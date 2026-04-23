'' VOICE example

wave 1, 1
envelope 1, 0.01, 0.05, 0.70, 0.05
instrument 1, 1, 1

print "voice() before ="; voice()
voice 1
print "voice() after  ="; voice()

sound 440, 0.20
sleep 300
