'' Project: FreeBASIC semantic sidecar tests
'' File: credential-context-inputs.bas
'' Purpose: Preserve named assignment, call and optional-default inputs.
'' Responsibilities: Variables, parameter variables, fields and selected formals.
'' This file intentionally does NOT execute unresolved declarations or expose real credentials.
#lang "fb"

type CredentialRecord
	password as string
	token as string
end type

declare sub AcceptSecrets(byval api_token as string, byval ordinary as string = "ordinary-call-default")
declare sub WithDefaults(byval password as string = "password-default", _
	byval ordinary as string = "ordinary-default")

sub ExerciseCredentialInputs(byval password as string)
	dim secret as string = "initializer-secret"
	dim token as string
	token = "assignment-token"
	password = "parameter-assignment"
	dim item as CredentialRecord
	item.password = "field-assignment"
	dim ordinary as string = "ordinary-initializer"
	AcceptSecrets("call-token", "ordinary-call")
	if( password = "comparison-value" ) then print password
end sub

'' end of credential-context-inputs.bas
