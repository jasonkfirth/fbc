/'
    Project: FreeBASIC C backend regression tests
    File: c-generated-procedure-graph.bas
    Purpose: Keep reachable generated methods across nested object copies.
    Responsibilities: Exercise private calls, callback roots and object lifetimes.
    This file intentionally does NOT change runtime error or graphics behavior.
'/
'' TEST_MODE : COMPILE_AND_RUN_OK
#macro CHECK(condition)
    if( (condition) = 0 ) then end 1
#endmacro

dim shared as integer live_objects, copies_seen, callbacks_seen

type TrackedLeaf
    text as string
    declare constructor()
    declare constructor(byref source as const TrackedLeaf)
    declare destructor()
end type

constructor TrackedLeaf()
    live_objects += 1
    text = "new"
end constructor

constructor TrackedLeaf(byref source as const TrackedLeaf)
    live_objects += 1
    copies_seen += 1
    text = source.text
end constructor

destructor TrackedLeaf()
    CHECK(live_objects > 0)
    live_objects -= 1
end destructor

type Branch
    one as TrackedLeaf
    pair(0 to 1) as TrackedLeaf
    description as string
end type

type Root
    child as Branch
    title as string
end type

type DormantRoot
    child as Branch
    title as string
end type

'' Explicit source procedures remain roots, even when the source never calls them.
private function DormantExplicit(byval source as DormantRoot) as DormantRoot
    return source
end function

private function CallbackValue(byval value as integer) as integer
    callbacks_seen += 1
    return value + 1
end function

type Callback as function(byval as integer) as integer
dim shared as Callback callback_slot = @CallbackValue

private function CopyRoot(byval source as Root) as Root
    dim result as Root = source
    result.child.one.text += ":copy"
    result.child.description += ":copy"
#ifdef R36_OPAQUE
    asm
        nop
    end asm
#endif
    return result
end function

scope
    dim as Root first, second
    first.title = "title"
    first.child.one.text = "one"
    first.child.pair(0).text = "zero"
    first.child.pair(1).text = "two"
    first.child.description = "branch"
    '' Aggregate assignment need not invoke a member copy constructor.
    '' Request this constructor explicitly to check its side effect.
    dim witness as TrackedLeaf = TrackedLeaf(first.child.one)
    CHECK(witness.text = "one")
    second = CopyRoot(first)
    CHECK(first.child.one.text = "one")
    CHECK(first.child.description = "branch")
    CHECK(second.child.one.text = "one:copy")
    CHECK(second.child.description = "branch:copy")
    CHECK(second.child.pair(0).text = "zero")
    CHECK(second.child.pair(1).text = "two")
    CHECK(second.title = "title")
    CHECK(callback_slot(41) = 42)
    CHECK(callbacks_seen = 1)
    CHECK(copies_seen > 0)
end scope
CHECK(live_objects = 0)
end 0
'' end of c-generated-procedure-graph.bas
