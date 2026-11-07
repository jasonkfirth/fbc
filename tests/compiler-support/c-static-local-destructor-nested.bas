/'
    Project: FreeBASIC compiler reachability regression
    File: c-static-local-destructor-nested.bas
    Purpose: Preserve nested static destructor dependencies across AST flushes.
    Responsibilities: Check nested destructor and string storage cleanup.
    This fixture intentionally does NOT use platform-specific cleanup code.
'/
type InnerItem
    value as long
    declare destructor()
end type
private destructor InnerItem()
    print "INNER="; this.value
end destructor

type OuterItem
    value as long
    declare destructor()
end type
private destructor OuterItem()
    static inner as InnerItem
    print "OUTER="; this.value
end destructor

private sub use_outer()
    static outer as OuterItem
end sub

private sub use_mixed_storage()
    static text as string
    static entries() as string
    text = "retained"
    redim entries(0 to 1)
    entries(0) = text
    if entries(0) <> "retained" then end 1
end sub

use_mixed_storage()
use_outer()
end 0
' end of c-static-local-destructor-nested.bas
