'' TEMPO example

print "tempo() before ="; tempo()
print "Playing at the current tempo."
play "L8 CDEFG"
sleep 700

tempo 160
print "tempo() after  ="; tempo()
print "Playing the same notes faster."
play "L8 CDEFG"
sleep 500
