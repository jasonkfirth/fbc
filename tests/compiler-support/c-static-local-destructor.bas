/'
    Project: FreeBASIC compiler reachability regression
    File: c-static-local-destructor.bas
    Purpose: Preserve calls made by generated static destructor wrappers.
    Responsibilities: Check scalar and array cleanup and an unused owner.
    This fixture intentionally does NOT require user interaction.
'/
private function cleanup_leaf() as long
    return 42
end function
private function unused_caller() as long
    return cleanup_leaf()
end function
type CleanupItem
    value as long
    declare destructor()
end type
private destructor CleanupItem()
    print "DTOR="; cleanup_leaf() + this.value
end destructor
type UnusedItem
    value as long
    declare destructor()
end type
private destructor UnusedItem()
    print "UNEXPECTED="; this.value
end destructor
private sub unused_static_owner()
    static unused as UnusedItem
end sub
private sub create_static()
    static item as CleanupItem
    static items(0 to 1) as CleanupItem
end sub
create_static()
end 0
' end of c-static-local-destructor.bas
