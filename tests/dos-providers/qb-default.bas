/'
    FreeBASIC DOS provider tests: qb-default.bas
    Check the ordinary DOS profile retains QB integer/string behavior and
    foreground SOUND/PLAY. No thread provider is requested by this test.
'/

#lang "qb"
dim value as integer
dim text as string
value = 32767
text = "DOS" + " QB"
if len(value) <> 2 or text <> "DOS QB" then
    print "FAIL QB type or string behavior"
    end 1
end if
sound 440, 2
play "MF T255 O4 L64 CEG"
print "PASS default DOS QB strings and foreground audio"
end 0

' end of qb-default.bas
