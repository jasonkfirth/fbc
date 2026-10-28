%FBSTRING = type { i8*, i64, i64 }
%T_1 = type { i32, i32, i8*, i8* }
declare void @llvm.memset.p0i8.i64(i8*, i8, i64, i32, i1) nounwind
declare void @llvm.va_start.p0i8(i8*) nounwind
declare void @llvm.va_end.p0i8(i8*) nounwind
declare void @llvm.va_copy.p0i8.p0i8(i8*, i8*) nounwind

declare void @fb_PrintString( i32, %FBSTRING*, i32 ) nounwind
declare %FBSTRING* @fb_StrAllocTempDescZEx( i8*, i64 ) nounwind
declare i64 @fb_StrLen( i8*, i64 ) nounwind
declare void @fb_End( i32 ) nounwind

@Lt_0012 = private constant [3 x i8] c"%s\00"
@Lt_0013 = private constant [6 x i8] c"MATCH\00"
@Lt_0016 = private constant [28 x i8] c"C va_list forwarding passed\00"
@llvm.global_ctors = appending global [1 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* @fb_ctor__cvazforwardzabi, i8* null }]

define i64 @READ_STRING( %T_1* %ARGS.0.1$ ) nounwind
{
	%fb$result.1.2 = alloca i64
	%COPIED_ARGS.2.3 = alloca %T_1
	%VALUE.3.4 = alloca i8*
	%vr11 = alloca %T_1

	; paramvar ARGS
	%ARGS.0.1 = alloca %T_1*
	store %T_1* %ARGS.0.1$, %T_1** %ARGS.0.1

	; localvar fb$result

	; localvar COPIED_ARGS

	; localvar VALUE

	; addrof fb$result

	; memfill fb$result
	%vr1 = bitcast i64* %fb$result.1.2 to i8*
	call void @llvm.memset.p0i8.i64( i8* %vr1, i8 0, i64 8, i32 1, i1 false )

	; label .L_0005
	br label %.L_0005
.L_0005:

	; addrof COPIED_ARGS

	; memfill COPIED_ARGS
	%vr3 = bitcast %T_1* %COPIED_ARGS.2.3 to i8*
	call void @llvm.memset.p0i8.i64( i8* %vr3, i8 0, i64 24, i32 1, i1 false )
	%vr4 = bitcast %T_1* %COPIED_ARGS.2.3 to %T_1*
	%vr6 = bitcast %T_1* %vr4 to i8*
	%vr7 = load %T_1, %T_1* %ARGS.0.1
	%vr8 = inttoptr %T_1 %vr7 to i8*
	call void @llvm.va_copy.p0i8.p0i8(i8* %vr6, i8* %vr8)
	%vr10 = bitcast %T_1* %COPIED_ARGS.2.3 to %T_1*
	%vr12 = load %T_1, %T_1* %vr10
	store %T_1 %vr12, %T_1* %vr11
	%vr9 = va_arg %T_1* %vr11, i8*
	%vr13 = load %T_1, %T_1* %vr11
	store %T_1 %vr13, %T_1* %vr10

	; store VALUE := vr9
	store i8* %vr9, i8** %VALUE.3.4
	%vr14 = bitcast %T_1* %COPIED_ARGS.2.3 to %T_1*
	%vr16 = bitcast %T_1* %vr14 to i8*
	call void @llvm.va_end.p0i8(i8* %vr16)

	; branchbop VALUE <> 0
	%vr18 = load i8*, i8** %VALUE.3.4
	%vr19 = bitcast i8* null to i8*
	%vr17 = icmp ne i8* %vr18, %vr19
	br i1 %vr17, label %.L_0008, label %.L_0009
.L_0009:

	; store fb$result := -1
	store i64 -1, i64* %fb$result.1.2

	; goto .L_0006
	br label %.L_0006
.L_000A:

	; label .L_0008
	br label %.L_0008
.L_0008:

	; call fb_StrLen()
	; arg VALUE
	%vr21 = bitcast i8** %VALUE.3.4 to i8**
	%vr22 = load i8*, i8** %vr21
	; arg 0
	%vr20 = call i64 @fb_StrLen( i8* %vr22, i64 0 )

	; store fb$result := vr20
	store i64 %vr20, i64* %fb$result.1.2

	; goto .L_0006
	br label %.L_0006
.L_000B:

	; label .L_0006
	br label %.L_0006
.L_0006:

	; loadres fb$result
	%vr25 = load i64, i64* %fb$result.1.2
	ret i64 %vr25
}

define i64 @FORWARD_STRING( %T_1* %ARGS.4.3$ ) nounwind
{
	%fb$result.5.4 = alloca i64

	; paramvar ARGS
	%ARGS.4.3 = alloca %T_1*
	store %T_1* %ARGS.4.3$, %T_1** %ARGS.4.3

	; localvar fb$result

	; addrof fb$result

	; memfill fb$result
	%vr1 = bitcast i64* %fb$result.5.4 to i8*
	call void @llvm.memset.p0i8.i64( i8* %vr1, i8 0, i64 8, i32 1, i1 false )

	; label .L_000C
	br label %.L_000C
.L_000C:

	; call READ_STRING()
	; arg ARGS
	%vr3 = load %T_1, %T_1* %ARGS.4.3
	%vr4 = inttoptr %T_1 %vr3 to %T_1*
	%vr2 = call i64 @READ_STRING( %T_1* %vr4 )

	; store fb$result := vr2
	store i64 %vr2, i64* %fb$result.5.4

	; goto .L_000D
	br label %.L_000D
.L_000E:

	; label .L_000D
	br label %.L_000D
.L_000D:

	; loadres fb$result
	%vr6 = load i64, i64* %fb$result.5.4
	ret i64 %vr6
}

define i64 @FORMAT_STRING( i8* %FORMAT.6.5$, ... ) nounwind
{
	%fb$result.7.6 = alloca i64
	%ARGS.8.7 = alloca %T_1
	%RESULT.9.8 = alloca i64

	; paramvar FORMAT
	%FORMAT.6.5 = alloca i8*
	store i8* %FORMAT.6.5$, i8** %FORMAT.6.5

	; localvar fb$result

	; localvar ARGS

	; localvar RESULT

	; addrof fb$result

	; memfill fb$result
	%vr1 = bitcast i64* %fb$result.7.6 to i8*
	call void @llvm.memset.p0i8.i64( i8* %vr1, i8 0, i64 8, i32 1, i1 false )

	; label .L_000F
	br label %.L_000F
.L_000F:

	; addrof ARGS

	; memfill ARGS
	%vr3 = bitcast %T_1* %ARGS.8.7 to i8*
	call void @llvm.memset.p0i8.i64( i8* %vr3, i8 0, i64 24, i32 1, i1 false )
	%vr4 = bitcast %T_1* %ARGS.8.7 to %T_1*
	%vr6 = bitcast %T_1* %vr4 to i8*
	call void @llvm.va_start.p0i8(i8* %vr6)

	; call FORWARD_STRING()
	; arg ARGS
	%vr7 = call i64 @FORWARD_STRING( %T_1* %ARGS.8.7 )

	; store RESULT := vr7
	store i64 %vr7, i64* %RESULT.9.8
	%vr8 = bitcast %T_1* %ARGS.8.7 to %T_1*
	%vr10 = bitcast %T_1* %vr8 to i8*
	call void @llvm.va_end.p0i8(i8* %vr10)

	; store fb$result := RESULT
	%vr11 = load i64, i64* %RESULT.9.8
	store i64 %vr11, i64* %fb$result.7.6

	; goto .L_0010
	br label %.L_0010
.L_0011:

	; label .L_0010
	br label %.L_0010
.L_0010:

	; loadres fb$result
	%vr13 = load i64, i64* %fb$result.7.6
	ret i64 %vr13
}

define private void @fb_ctor__cvazforwardzabi(  ) nounwind
{

	; label .L_0003
	br label %.L_0003
.L_0003:

	; call FORMAT_STRING()
	; arg Lt_0012
	%vr1 = bitcast i8* bitcast ([3 x i8]* @Lt_0012 to i8*) to i8*
	; arg Lt_0013
	%vr0 = call i64 @FORMAT_STRING( i8* %vr1, i8* bitcast ([6 x i8]* @Lt_0013 to i8*) )

	; branchbop vr0 == 5
	%vr2 = icmp eq i64 %vr0, 5
	br i1 %vr2, label %.L_0015, label %.L_0017
.L_0017:

	; call fb_End()
	; arg 1
	call void @fb_End( i32 1 )

	; label .L_0015
	br label %.L_0015
.L_0015:

	; call fb_StrAllocTempDescZEx()
	; arg Lt_0016
	%vr5 = bitcast i8* bitcast ([28 x i8]* @Lt_0016 to i8*) to i8*
	; arg 27
	%vr4 = call %FBSTRING* @fb_StrAllocTempDescZEx( i8* %vr5, i64 27 )

	; call fb_PrintString()
	; arg 0
	; arg vr4
	%vr8 = bitcast %FBSTRING* %vr4 to %FBSTRING*
	; arg 1
	call void @fb_PrintString( i32 0, %FBSTRING* %vr8, i32 1 )

	; label .L_0004
	br label %.L_0004
.L_0004:
	ret void
}
