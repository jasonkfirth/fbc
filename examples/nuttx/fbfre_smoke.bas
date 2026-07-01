''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbfre_smoke.bas
''
'' Purpose:
''
''     Prove that FRE() reports real NuttX heap availability instead of the
''     temporary "unknown" value used during early bring-up.
''
'' Responsibilities:
''
''     - call the FreeBASIC FRE() runtime entry point
''     - require a nonzero heap availability value under the QEMU NuttX profile
''     - prove that the value changes when heap memory is allocated and freed
''     - keep the memory-budget smoke independent from the larger hello test
''
'' This file intentionally does NOT contain:
''
''     - board-specific memory sizing rules
''     - graphics, audio, storage, or networking checks
''     - assumptions about the exact number of free bytes
''

const allocation_bytes as uinteger = 8192

dim as uinteger free_before
dim as uinteger free_after_alloc
dim as uinteger free_after_free
dim as ubyte ptr probe

free_before = fre(0)

print "fre initial nonzero ="; iif(free_before > 0, 1, 0)

probe = callocate(allocation_bytes)

if probe = 0 then
    print "fre allocation failed"
    end 1
end if

probe[0] = 123
print "fre allocation sample ="; probe[0]

free_after_alloc = fre(0)

print "fre after alloc lower ="; iif(free_after_alloc < free_before, 1, 0)

deallocate probe

free_after_free = fre(0)

print "fre after free sane ="; iif(free_after_free >= free_after_alloc, 1, 0)

print "FB_NUTTX_FRE_SMOKE_OK"

'' end of fbfre_smoke.bas
