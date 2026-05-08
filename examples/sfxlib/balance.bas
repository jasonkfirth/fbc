'' BALANCE example

print "balance() before ="; balance()
print "Playing centered."
play "T120 O4 L8 CEG"
sleep 500

balance -0.75
print "balance() left   ="; balance()
play "T120 O4 L8 CEG"
sleep 500

balance 0.75
print "balance() right  ="; balance()
play "T120 O4 L8 CEG"
sleep 500
