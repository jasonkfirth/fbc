'' INSTRUMENT example

wave 1, 1
envelope 1, 0.01, 0.04, 0.65, 0.05
instrument 1, 1, 1
instrument 2, 1

print "Default voice."
sound 494, 0.20
sleep 300

print "Instrument 2 on channel 2."
sound 2, 494, 0.20, 0.70
sleep 300
