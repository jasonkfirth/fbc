' TEST_MODE : COMPILE_ONLY_OK

'' This should compile fine even under -gen gcc

namespace ns
	type a as function( ) as integer

	sub mythread( byval userdata as any ptr ) 
	end sub

#if defined(__FB_DOS__)
	sub use_procptr( byval callback as sub( byval as any ptr ) )
	end sub
#endif

	sub foo( )
#if defined(__FB_DOS__)
		use_procptr( @mythread )
#else
		threadcreate( @mythread ) 
#endif
	end sub

	dim shared as sub( ) global
end namespace
