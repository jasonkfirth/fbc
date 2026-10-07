'' Project: FreeBASIC semantic sidecar tests
'' File: aggregate-access.bas
'' Purpose: Preserve accepted aggregate bodies and written access sections.
'' Responsibilities: Complete ordinals, nested owners, macros and conditionals.
'' This file intentionally does NOT execute member or initialization behavior.
#lang "fb"
#define PUBLIC_SECTION public:
type AccessOwner
    public:
    ValueOne as long
    PUBLIC_SECTION
    public: ValueTwo as long
    private:
    PrivateValue as long
    private:
    protected:
    ProtectedValue as long
    protected:
    public:
    type NestedOwner
        public:
        NestedValue as long
        public:
    end type
    union
        UnionOne as long
        UnionTwo as long
    end union
    public:
end type
#if 1
type ActiveOwner
    public:
    ActiveValue as long
    public:
end type
#else
type InactiveOwner
    public:
    public:
end type
#endif
type DefaultOwner
    DefaultValue as long
end type
'' end of aggregate-access.bas
