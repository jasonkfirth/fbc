'' Project: FreeBASIC semantic sidecar tests
'' File: startup-lifecycle.bas
'' Purpose: Exercise registration order, generated wrappers and guarded statics.
'' Responsibilities: Priorities, forward/reverse instance actions and status flags.
'' This file intentionally does NOT prescribe loader tie order across modules.

type StartupItem
	value as long
	declare constructor(byval value as long)
	declare destructor()
end type

constructor StartupItem(byval value as long)
	this.value = value
	print "I" + ltrim(str(value))
end constructor

destructor StartupItem()
	print "D" + ltrim(str(value))
end destructor

sub EarlyStartup() constructor 101
	print "EARLY"
end sub

sub SameStartupA() constructor 201
	print "A"
end sub

sub SameStartupB() constructor 201
	print "B"
end sub

sub LateTeardown() destructor 101
	print "LATE"
end sub

dim shared first_startup as StartupItem = StartupItem(1)
dim shared second_startup as StartupItem = StartupItem(2)
dim shared plain_startup as long = 9

type StartupPlainCtor
	value as long
	declare constructor()
end type

constructor StartupPlainCtor()
	value = 17
end constructor

function StartupBound() as long
	return 1
end function

sub UseStartupStatics()
	static local_startup as StartupItem = StartupItem(10)
	static scalar_startup as StartupPlainCtor
	static array_startup(0 to StartupBound()) as string
	if scalar_startup.value <> 17 then end 1
	array_startup(0) = "used"
end sub

private sub UnusedStartupStatics()
	static unused_startup as StartupItem = StartupItem(20)
end sub

function AssignedStartupResult() as long
	AssignedStartupResult = 1
end function

function ReturnedStartupResult() as long
	return 2
end function

UseStartupStatics()
UseStartupStatics()
if AssignedStartupResult() <> 1 or ReturnedStartupResult() <> 2 then end 2
print "MAIN"

'' end of startup-lifecycle.bas
