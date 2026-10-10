'' Project: FreeBASIC semantic sidecar tests
'' File: inferred-construction.bas
'' Purpose: Preserve native VAR declarations and their selected constructors.
'' Responsibilities: Inferred storage, reference bindings and expanded declarations.
'' This file intentionally does NOT run its local-static test procedure.
#lang "fb"

type SemanticObject
	value as integer
	declare constructor( byval initial_value as integer )
end type

constructor SemanticObject( byval initial_value as integer )
	value = initial_value
end constructor

var shared InferredGlobal = SemanticObject(1)
static var InferredStatic = SemanticObject(2)
var InferredLocal = SemanticObject(3)
var shared byref InferredAlias = InferredGlobal

namespace InferredSpace
	var InferredNamespaced = SemanticObject(4)
end namespace

sub InferredProcedure()
	static var InferredProcedureStatic = SemanticObject(5)
end sub

#define InferredDeclaration var shared InferredMacro = SemanticObject(6)
InferredDeclaration
var InferredNumeric = 7

'' end of inferred-construction.bas
