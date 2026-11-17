'' FreeBASIC compiler parser regression, c-deep-source-locations.bas
'' Checks nested casts with ordinary and exported source locations.
'' The parser owns temporary source records while the expression is active.
#lang "fb"

private function nested_value(byval value as long) as long
	'' Sixty-four written casts exercise recursive parser frames directly.
	return _
		cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, ( _
		cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, ( _
		cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, ( _
		cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, ( _
		cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, ( _
		cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, ( _
		cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, ( _
		cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, (cast(long, ( _
		value _
		)))))))))))))))) _
		)))))))))))))))) _
		)))))))))))))))) _
		)))))))))))))))) _
		)))))))))))))))) _
		)))))))))))))))) _
		)))))))))))))))) _
		))))))))))))))))
end function

if nested_value(0) <> 0 then end 1
if nested_value(1234567) <> 1234567 then end 2
if nested_value(-1234567) <> -1234567 then end 3
print "DEEP_SOURCE_LOCATIONS_PASS"

'' end of c-deep-source-locations.bas
