'' Project: FreeBASIC semantic sidecar tests
'' File: accepted-declarations.bas
'' Purpose: Exercise source declaration classes outside procedure/formal lists.
'' Responsibilities: Named constants, enums, types, fields, aliases and labels.
'' This file intentionally does NOT execute generated special members.
#lang "fb"

namespace AcceptedTypes
    const SourceConstant = 7
    enum SourceEnumeration
        SourceElement = SourceConstant
    end enum
    type SourceRecord
        value as long
    end type
    type SourceAlias as SourceRecord
end namespace

sub AcceptedLabelOwner()
SourceLabel:
end sub

'' end of accepted-declarations.bas
