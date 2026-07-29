%FBSTRING = type { i8*, i64, i64 }

declare void @"fb_End"( i32 ) nounwind

%fb_RTTI$ = type { i8*, i8*, %fb_RTTI$* }
@__fb_ZTS6Object = global %fb_RTTI$ zeroinitializer
@VALUES = global [4 x i64] { [4 x i64] 11, [4 x i64] 22, [4 x i64] 33, [4 x i64] 44 }
@llvm.global_ctors = appending global [1 x { i32, void ()* }] [{ i32, void ()* } { i32 0, void ()* @"fb_ctor__llvmzindexedzaddress" }]

define private void @"fb_ctor__llvmzindexedzaddress"(  ) nounwind
{

	; localvar INDEX
	%INDEX.0 = alloca i64

	; localvar P
	%P.1 = alloca i64*

	; label .L_0002
	br label %.L_0002
.L_0002:

	; store INDEX := 2
	store i64 2, i64* %INDEX.0

	; bop INDEX SHL 3
	%vr1 = load i64, i64* %INDEX.0
	%vr0 = shl i64 %vr1, 3

	; branchbop VALUES+vr0*1 == 33
	%vr3 = ptrtoint i64* @VALUES to i64
	%vr4 = add i64 %vr3, %vr0
	%vr5 = inttoptr i64 %vr4 to i64*
	%vr6 = load i64, i64* %vr5
	%vr2 = icmp eq i64 %vr6, 33
	br i1 %vr2, label %.L_0006, label %.L_0009
.L_0009:

	; call fb_End()
	; arg 1
	call void @"fb_End"( i32 1 )

	; label .L_0006
	br label %.L_0006
.L_0006:

	; label .L_0005
	br label %.L_0005
.L_0005:

	; store P := VALUES
	store i64* @VALUES, i64** %P.1

	; bop INDEX SHL 3
	%vr9 = load i64, i64* %INDEX.0
	%vr8 = shl i64 %vr9, 3

	; bop P + vr8
	%vr11 = load i64*, i64** %P.1
	%vr12 = inttoptr i64 %vr8 to i64*
	%vr10 = add i64* %vr11, %vr12

	; branchbop vr10 == 33
	%vr14 = ptrtoint i64* %vr10 to i64
	%vr15 = inttoptr i64 %vr14 to i64*
	%vr16 = load i64, i64* %vr15
	%vr13 = icmp eq i64 %vr16, 33
	br i1 %vr13, label %.L_0008, label %.L_000A
.L_000A:

	; call fb_End()
	; arg 2
	call void @"fb_End"( i32 2 )

	; label .L_0008
	br label %.L_0008
.L_0008:

	; label .L_0007
	br label %.L_0007
.L_0007:

	; call fb_End()
	; arg 0
	call void @"fb_End"( i32 0 )

	; label .L_0003
	br label %.L_0003
.L_0003:
	ret void
}
