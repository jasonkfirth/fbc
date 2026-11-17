/'
    Project: FreeBASIC C-backend regression tests
    File: c-error-external-fields.bi
    Purpose: Share managed objects whose user members live in another module.
    Responsibilities: Declare nested fields and observable member-call counts.
    Member bodies and the executable checks have their own source files.
'/
#ifndef __C_ERROR_EXTERNAL_FIELDS_BI__
#define __C_ERROR_EXTERNAL_FIELDS_BI__

namespace CleanupFields
    type ChildState
        text as string
        words(0 to 1) as string
    end type

    type ParentState
        child as ChildState
        children(0 to 1) as ChildState
        declare constructor()
        declare destructor()
    end type

    extern failure_kind as long
    extern constructor_calls as long
    extern destructor_calls as long
end namespace

#endif
' end of c-error-external-fields.bi
