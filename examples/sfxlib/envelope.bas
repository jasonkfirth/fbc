'' ENVELOPE example

print "Default envelope."
sound 330, 0.30
sleep 400

wave 1, 2
envelope 1, 0.01, 0.10, 0.50, 0.20
instrument 1, 1, 1
voice 1

print "Custom envelope."
sound 330, 0.30
sleep 350
