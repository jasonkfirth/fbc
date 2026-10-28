'' Project: FreeBASIC semantic sidecar tests
'' File: lexical-lifetimes.bas
'' Purpose: Exercise normal lexical destruction and its storage exclusions.
'' Responsibilities: Nested groups, reverse object order, strings, arrays and aliases.
'' This file intentionally does NOT infer destruction from a type spelling.

type LexicalValue
	value as integer
	declare constructor( byval number as integer )
	declare destructor( )
end type

constructor LexicalValue( byval number as integer )
	value = number
end constructor

destructor LexicalValue( )
	print value
end destructor

type LexicalPlain
	value as long
end type

sub ObserveBorrowedLexical( byref borrowed_lexical as LexicalValue )
	dim borrowed_pointer as LexicalValue ptr = @borrowed_lexical
	if borrowed_pointer->value < 0 then end 1
end sub

sub NormalLexicalTeardown( )
	dim root_lexical_one as LexicalValue = LexicalValue(1)
	dim root_lexical_two as LexicalValue = LexicalValue(2)
	static retained_lexical as LexicalValue = LexicalValue(99)
	scope
		dim inner_lexical_one as LexicalValue = LexicalValue(3)
		dim inner_lexical_two as LexicalValue = LexicalValue(4)
		dim lexical_text as string = "managed"
		dim lexical_array(0 to 1) as string
		dim plain_lexical as LexicalPlain
		dim fixed_lexical as string * 8
		lexical_array(0) = lexical_text
		plain_lexical.value = len(fixed_lexical)
		ObserveBorrowedLexical(root_lexical_one)
	end scope
	scope
	end scope
end sub

NormalLexicalTeardown( )

'' end of lexical-lifetimes.bas
