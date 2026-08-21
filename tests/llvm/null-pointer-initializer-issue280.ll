%FBSTRING = type { i8*, i64, i64 }
declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i32, i1) nounwind


%fb_RTTI$ = type { i8*, i8*, %fb_RTTI$* }
@__fb_ZTS6Object = global %fb_RTTI$ zeroinitializer
@llvm.global_ctors = appending global [1 x { i32, void ()* }] [{ i32, void ()* } { i32 0, void ()* @fb_ctor__nullzpointerzinitializerzissue280 }]

define i64 @EntryPoint(  ) nounwind
{

	; localvar fb$result
	%fb$result.0 = alloca i64

	; localvar PVALUE
	%PVALUE.1 = alloca i64*

	; addrof fb$result

	; memfill fb$result
	%vr1 = bitcast i64* %fb$result.0 to i8*
	call void @llvm.memset.p0i8.i32( i8* %vr1, i8 0, i32 8, i32 1, i1 false )

	; label .L_0004
	br label %.L_0004
.L_0004:

	; store PVALUE := 0
	store i64* null, i64** %PVALUE.1

	; store fb$result := 0
	store i64 0, i64* %fb$result.0

	; goto .L_0005
	br label %.L_0005
.L_0006:

	; label .L_0005
	br label %.L_0005
.L_0005:

	; loadres fb$result
	%vr3 = load i64, i64* %fb$result.0
	ret i64 %vr3
}
