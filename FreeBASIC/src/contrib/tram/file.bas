#include once "vbcompat.bi"
#include once "zstr.bi"
#include once "list.bi"
#include once "file.bi"

namespace fb.file

	type findCtx
		mode 		as CSearch.searchBy
		union
			serial 	as double
		end union
		mask 		as zstring ptr
	end type

	type intrCtx
		node		as CSearchEntry ptr
	end type
	
	type CSearchCtx_
		root 	as zstring ptr
		list	as CList ptr
		dirCb 	as CSearchDirCallback
		intrCtx	as intrCtx
		findCtx as findCtx
	end type

	declare sub _resetList _
		( _
			byval l as CList ptr _
		)

	declare function _findFiles _
		( _
			byval ctx as CSearchCtx ptr, _
			byval path as zstring ptr _
		) as integer

	'':::::
	constructor CSearch _
		( _
			byval root as zstring ptr, _
			byval dirCb as CSearchDirCallback = NULL _
		) 
	
		ctx = new CSearchCtx
		
		ctx->root = zStr.Dup( root )
		ctx->dirCb = dirCb
		
		ctx->list = new CList( 30, len( CSearchEntry ), CList.flags_NOCLEAR )
		ctx->intrCtx.node = NULL
		
	end constructor

	'':::::
	destructor CSearch _
		( _
		) 
		
		if( ctx = NULL ) then
			return
		end if
		
		_resetList( ctx->list )
		delete ctx->list
		
		zStr.del( ctx->root )
		
		delete ctx
		
	end destructor

	'':::::
	function CSearch.getFirst _
		( _
		) as CSearchEntry ptr
		
		ctx->intrCtx.node = ctx->list->getHead( )
		
		function = ctx->intrCtx.node
		
	end function

	'':::::
	function CSearch.getNext _
		( _
		) as CSearchEntry ptr
		
		if( ctx->intrCtx.node = NULL ) then
			return NULL
		end if
		
		ctx->intrCtx.node = ctx->list->getNext( ctx->intrCtx.node )
		
		function = ctx->intrCtx.node
		
	end function

	'':::::
	function CSearch.byDate _
		( _
			byval mask as zstring ptr, _
			byval serial as double, _
			byval mode as searchBy _
		) as integer
		
		with ctx->findCtx
			.mode = mode
			.serial = serial
			.mask = mask
		end with
		
		_resetList( ctx->list )
		
		function = _findFiles( ctx, NULL ) > 0
		
	end function
	
	'':::::
	private sub _resetList _
		( _
			byval l as CList ptr _
		)
		
		dim as CSearchEntry ptr n = any, nxt = any
		
		n = l->getHead( )
		do while( n <> NULL )
			
			zstr.del( n->path )
			zstr.del( n->name )
			
			nxt = l->getNext( n )
			l->remove( n )
			n = nxt
		loop
		
	end sub
	
	'':::::
	private function _addFile _
		( _
			byval l as CList ptr, _
			byval path as zstring ptr, _
			byval fname as zstring ptr, _
			byval serial as double _
		) as CSearchEntry ptr
		
		dim as CSearchEntry ptr n = any
		
		n = l->insert( )
		
		if( path <> NULL ) then
			n->path = zstr.dup( path )
		else
			n->path = NULL
		end if
		n->name = zstr.dup( fname )
		n->serial = serial
		
		function = n
		
	end function
	
	'':::::
	private function _findFiles _
		( _
			byval ctx as CSearchCtx ptr, _
			byval path as zstring ptr _
		) as integer
		
		dim as string fname, fpath
		dim as integer files = 0
		
		'' add files
		if( ctx->root <> NULL ) then
			fpath = *ctx->root + "/"
		end if	
		
		if( path <> NULL ) then
			fpath += *path + "/" 
		end if
			
		fname = *ctx->findCtx.mask
		
		fname = dir( fpath + fname, vbNormal )
		do while len( fname ) > 0
			dim as double serial = filedatetime( fpath + fname )
				
			select case ctx->findCtx.mode
			case CSearch.searchBy_SerialNewer
				if( serial > ctx->findCtx.serial ) then
					_addFile( ctx->list, path, fname, serial )
					files += 1
				end if
			end select
			
			fname = dir( )
		loop
		
		'' collect dirs (can't use recursion here, DIR() has a global context)
		type DIRENTRY
			name	as zstring ptr
		end type
		
		dim as CList ptr l = new CList( 16, len( DIRENTRY ), CList.flags_NOCLEAR )
		dim as DIRENTRY ptr node = any, nxt = any
		
		fname = dir( fpath + "*.*", vbDirectory )
		do while len( fname ) > 0
			select case fname
			case ".", ".."
			
			case else
				dim as integer doAdd = iif( ctx->dirCb <> NULL, ctx->dirCb( path, fname ), TRUE )
				
				if( doAdd ) then
					node = l->insert( )
					node->name = zStr.dup( fname )
				end if
			end select
			
			fname = dir( )
		loop
		
		'' recurse
		node = l->getHead( )
		do while( node <> NULL )
			nxt = l->getNext( node )
			
			if( path <> NULL ) then
				fname = *path + "/"
			else
				fname = ""
			end if
			fname += *node->name
			
			files += _findFiles( ctx, fname )
			
			zStr.del( node->name )
			l->remove( node )
			
			node = nxt
		loop

		delete l
		
		function = files
	
	end function

end namespace