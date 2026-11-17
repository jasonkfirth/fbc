/'
    Project: FreeBASIC C-backend regression tests
    File: c-error-external-fields-provider.bas
    Purpose: Define foreign-module user construction and destruction.
    Responsibilities: Allocate nested fields and interrupt either user body.
    The consumer owns error handlers and allocation observations.
'/
#lang "fb"
#include once "c-error-external-fields.bi"

namespace CleanupFields
    dim shared failure_kind as long
    dim shared constructor_calls as long
    dim shared destructor_calls as long

    constructor ParentState()
        constructor_calls += 1
        child.text = "scalar child"
        child.words(1) = "scalar array field"
        children(0).text = "first child"
        children(1).words(0) = "last child"
        if failure_kind = 1 then
            failure_kind = 0
            error 24
        end if
    end constructor

    destructor ParentState()
        destructor_calls += 1
        child.text = ""
        if failure_kind = 2 then
            failure_kind = 0
            error 25
        end if
    end destructor
end namespace

' end of c-error-external-fields-provider.bas
