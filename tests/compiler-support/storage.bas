'' Project: FreeBASIC compiler support regression tests
'' -----------------------------------------
''
'' File: storage.bas
''
'' Purpose:
''
''     Exercise compiler storage directly, including small initial pools.
''
'' Responsibilities:
''
''     - check alignment, growth, traversal, clearing, and buffer lengths
''     - provide isolated invalid-size cases for the regression runner
''
'' This file intentionally does NOT contain:
''
''     - parser or backend mocks
''

#include once "support/common.bi"
#include once "support/containers/list.bi"
#include once "support/containers/flist.bi"
#include once "support/containers/stack.bi"
#include once "support/containers/pool.bi"
#include once "support/strings/dstr.bi"

'' Each invalid case must stop before allocating or returning a wrapped size.
select case command(1)
case "add-overflow": print xcheckedAdd(XALLOC_MAX_SIZE, 1): end 0
case "multiply-overflow": print xcheckedMultiply(XALLOC_MAX_SIZE, 2): end 0
case "align-overflow": print xcheckedAlign(XALLOC_MAX_SIZE, 16): end 0
case "negative-size": print xcheckedMultiply(-1, 1): end 0
case "zero-allocation": print xallocate(0): end 0
case "invalid-alignment": print xcheckedAlign(1, 3): end 0
case "negative-buffer"
	dim as DZSTRING text
	DZstrAllocate(text, -1)
	end 0
end select

assert(xcheckedAdd(7, 9) = 16)
assert(xcheckedMultiply(7, 9) = 63)
assert(xcheckedMultiply(0, XALLOC_MAX_SIZE) = 0)
assert(xcheckedAlign(17, 16) = 32)
assert(xgrowthCount(1, 4) = 1)
assert(xgrowthCount(16, 4) = 4)

'' Odd payload sizes expose a stride that would misalign later nodes.
for initial as integer = 1 to 3
	dim as TLIST nodes
	dim as ubyte ptr payloads(0 to 31)
	listInit(@nodes, initial, 3)
	for i as integer = 0 to 31
		payloads(i) = listNewNode(@nodes)
		assert((cuint(payloads(i)) mod len(any ptr)) = 0)
		assert(payloads(i)[0] = 0)
		payloads(i)[0] = i + 1
	next
	dim as integer count = 0
	dim as ubyte ptr node = listGetHead(@nodes)
	while node <> NULL
		assert(node[0] = count + 1)
		node = listGetNext(node)
		count += 1
	wend
	assert(count = 32)
	listDelNode(@nodes, payloads(15))
	node = listNewNode(@nodes)
	assert(node = payloads(15))
	assert(node[0] = 0)
	listEnd(@nodes)
	assert(listGetHead(@nodes) = NULL)
next

dim as TFLIST arena
flistInit(@arena, 1, 3)
dim as ubyte ptr firstitem = flistNewItem(@arena)
firstitem[0] = 1
for i as integer = 2 to 32
	dim as ubyte ptr item = flistNewItem(@arena)
	assert((cuint(item) mod len(any ptr)) = 0)
	item[0] = i
next
dim as ubyte ptr item = flistGetHead(@arena)
for i as integer = 1 to 32
	assert(item <> NULL)
	assert(item[0] = i)
	item = flistGetNext(item)
next
assert(item = NULL)
flistReset(@arena)
assert(flistNewItem(@arena) = firstitem)
flistEnd(@arena)

for initial as integer = 0 to 3
	dim as TSTACK frames
	stackNew(@frames, initial, 3)
	for i as integer = 1 to 32
		dim as ubyte ptr frame = stackPush(@frames)
		assert((cuint(frame) mod len(any ptr)) = 0)
		assert(frame[0] = 0)
		frame[0] = i
	next
	for i as integer = 32 to 1 step -1
		dim as ubyte ptr frame = stackGetTOS(@frames)
		assert(frame[0] = i)
		stackPop(@frames)
	next
	assert(stackGetTOS(@frames) = NULL)
	dim as ubyte ptr reused = stackPush(@frames)
	assert(reused[0] = 0)
	stackPop(@frames)
	stackFree(@frames)
	assert(stackGetTOS(@frames) = NULL)
next

dim as TPOOL classes
poolInit(@classes, 1, 4, 16)
for bytes as integer = 1 to 32
	dim as ubyte ptr payload = poolNewItem(@classes, bytes)
	assert(payload <> NULL)
	for i as integer = 0 to bytes - 1
		payload[i] = i
	next
	poolDelItem(@classes, payload)
next
poolEnd(@classes)

dim as DZSTRING text
DZstrAssign(text, "a")
for i as integer = 1 to 100
	DZstrConcatAssignC(text, asc("b"))
next
assert(text.len = 101)
assert(*text.data = "a" + string(100, "b"))
DZstrAssign(text, "short")
assert(text.len = 5)
assert(*text.data = "short")
DZstrAllocate(text, 0)
assert(text.data = NULL)

dim as DWSTRING wide
DWstrAssign(wide, wstr("a"))
for i as integer = 1 to 100
	DWstrConcatAssignC(wide, asc("b"))
next
assert(wide.len = 101)
assert(*wide.data = wstr("a" + string(100, "b")))
DWstrAllocate(wide, 0)
assert(wide.data = NULL)

print "compiler storage passed"

'' end of storage.bas
