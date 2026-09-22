'' examples/manual/memory/allocate2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'ALLOCATE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgAllocate
'' --------

'' Ownership policy: this bad example deliberately overwrites an owning pointer
'' to demonstrate how incorrect Allocate usage causes memory leaks.

Sub BadAllocateExample()

	Dim p As Byte Ptr

	'' This unchecked allocation is intentionally retained for the leak demonstration. FB-LINTER: DISABLE-NEXT-LINE FBL800
	p = Allocate(420)   '' assign pointer to new memory

	'' This second unchecked allocation intentionally loses the first owner. FB-LINTER: DISABLE-NEXT-LINE FBL800
	p = Allocate(420)   '' reassign same pointer to different memory,
						'' old address is lost and that memory is leaked

	Deallocate(p)

End Sub

	'' Main
	BadAllocateExample() '' Creates a memory leak
	Print "Memory leak!"
	BadAllocateExample() '' ... and another
	Print "Memory leak!"
	End
