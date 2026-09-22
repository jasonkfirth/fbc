	.intel_syntax noprefix

.section .text
.balign 16

.globl _LOADTGA@8
_LOADTGA@8:
push ebp
mov ebp, esp
sub esp, 196
push ebx
push esi
mov dword ptr [ebp-4], 0
.L_004F:
mov dword ptr [ebp-8], 0
lea eax, [ebp-20]
mov dword ptr [ebp-56], eax
lea eax, [ebp-20]
mov dword ptr [ebp-52], eax
mov dword ptr [ebp-48], 12
mov dword ptr [ebp-44], 1
mov dword ptr [ebp-40], 1
mov dword ptr [ebp-36], 49
mov dword ptr [ebp-32], 12
mov dword ptr [ebp-28], 0
mov dword ptr [ebp-24], 11
mov byte ptr [ebp-20], 0
mov byte ptr [ebp-19], 0
mov byte ptr [ebp-18], 2
mov byte ptr [ebp-17], 0
mov byte ptr [ebp-16], 0
mov byte ptr [ebp-15], 0
mov byte ptr [ebp-14], 0
mov byte ptr [ebp-13], 0
mov byte ptr [ebp-12], 0
mov byte ptr [ebp-11], 0
mov byte ptr [ebp-10], 0
mov byte ptr [ebp-9], 0
mov dword ptr [ebp-68], 0
mov dword ptr [ebp-64], 0
mov dword ptr [ebp-60], 0
lea eax, [ebp-68]
mov dword ptr [ebp-104], eax
lea eax, [ebp-68]
mov dword ptr [ebp-100], eax
mov dword ptr [ebp-96], 12
mov dword ptr [ebp-92], 1
mov dword ptr [ebp-88], 1
mov dword ptr [ebp-84], 49
mov dword ptr [ebp-80], 12
mov dword ptr [ebp-76], 0
mov dword ptr [ebp-72], 11
mov dword ptr [ebp-112], 0
mov word ptr [ebp-108], 0
lea eax, [ebp-112]
mov dword ptr [ebp-148], eax
lea eax, [ebp-112]
mov dword ptr [ebp-144], eax
mov dword ptr [ebp-140], 6
mov dword ptr [ebp-136], 1
mov dword ptr [ebp-132], 1
mov dword ptr [ebp-128], 49
mov dword ptr [ebp-124], 6
mov dword ptr [ebp-120], 0
mov dword ptr [ebp-116], 5
mov dword ptr [ebp-152], 0
mov dword ptr [ebp-156], 0
mov dword ptr [ebp-164], 0
mov dword ptr [ebp-160], 0
mov byte ptr [ebp-168], 0
mov dword ptr [ebp-172], 6408
mov dword ptr [ebp-176], 0
mov dword ptr [ebp-180], 0
mov byte ptr [ebp-184], 0
call _fb_FileFree@0
mov dword ptr [ebp-176], eax
push 0
push dword ptr [ebp-176]
push 0
push 0
push 0
push dword ptr [ebp+12]
call _fb_FileOpen@24
test eax, eax
je .L_0055
mov dword ptr [ebp-4], 0
jmp .L_0050
.L_0055:
.L_0054:
push dword ptr [ebp-176]
call _fb_FileSize@4
cmp edx, 0
jl .L_0057
jg .L_007A
cmp eax, 18
jbe .L_0057
.L_007A:
lea eax, [ebp-104]
push eax
push 0
push dword ptr [ebp-176]
call _fb_FileGetArray@12
push 1
lea eax, [ebp-68]
push eax
lea eax, [ebp-20]
push eax
call _memcmp
add esp, 12
test eax, eax
jne .L_0059
lea eax, [ebp-148]
push eax
push 0
push dword ptr [ebp-176]
call _fb_FileGetArray@12
jmp .L_0058
.L_0059:
push dword ptr [ebp-176]
call _fb_FileClose@4
mov dword ptr [ebp-4], 0
jmp .L_0050
.L_0058:
jmp .L_0056
.L_0057:
push dword ptr [ebp-176]
call _fb_FileClose@4
mov dword ptr [ebp-4], 0
jmp .L_0050
.L_0056:
movzx eax, byte ptr [ebp-111]
sal eax, 8
movzx edx, byte ptr [ebp-112]
add eax, edx
mov ebx, eax
mov edx, dword ptr [ebp+8]
mov dword ptr [edx+8], ebx
movzx ebx, byte ptr [ebp-109]
sal ebx, 8
movzx edx, byte ptr [ebp-110]
add ebx, edx
mov eax, ebx
mov edx, dword ptr [ebp+8]
mov dword ptr [edx+12], eax
mov eax, dword ptr [ebp+8]
cmp dword ptr [eax+8], 0
mov edx, -1
jbe .L_007B
xor edx, edx
.L_007B:
mov eax, dword ptr [ebp+8]
cmp dword ptr [eax+12], 0
mov ebx, -1
jbe .L_007C
xor ebx, ebx
.L_007C:
or edx, ebx
mov dword ptr [ebp-192], edx
movzx edx, byte ptr [ebp-108]
cmp edx, 24
je .L_005A
movzx edx, byte ptr [ebp-108]
cmp edx, 32
mov edx, -1
jne .L_007D
xor edx, edx
.L_007D:
mov dword ptr [ebp-188], edx
jmp .L_0075
.L_005A:
mov dword ptr [ebp-188], 0
.L_0075:
mov edx, dword ptr [ebp-188]
or edx, dword ptr [ebp-192]
je .L_005D
push dword ptr [ebp-176]
call _fb_FileClose@4
mov dword ptr [ebp-4], 0
jmp .L_0050
.L_005D:
.L_005C:
movzx edx, byte ptr [ebp-108]
mov ebx, dword ptr [ebp+8]
mov dword ptr [ebx+4], edx
mov edx, dword ptr [ebp+8]
push 0
push dword ptr [edx+4]
fild qword ptr [esp]
add esp, 8
fdiv qword ptr [_Lt_0077]
sub esp, 8
fistp qword ptr [esp]
pop dword ptr [ebp-152]
add esp, 4
mov edx, dword ptr [ebp+8]
mov eax, dword ptr [edx+8]
mov ebx, 0
push ebx
push eax
fild qword ptr [esp]
add esp, 8
cmp ebx, 0
jns .L_007E
push 0x403f
push 0x80000000
push 0
fldt [esp]
add esp, 12
faddp
.L_007E:
mov eax, dword ptr [ebp+8]
mov edx, dword ptr [eax+12]
mov ebx, 0
push ebx
push edx
fild qword ptr [esp]
add esp, 8
cmp ebx, 0
jns .L_007F
push 0x403f
push 0x80000000
push 0
fldt [esp]
add esp, 12
faddp
.L_007F:
fld qword ptr [_Lt_0078]
fdivrp
fxch st(1)
fcompp
fnstsw ax
sahf
jbe .L_005F
push dword ptr [ebp-176]
call _fb_FileClose@4
mov dword ptr [ebp-4], 0
jmp .L_0050
.L_005F:
.L_005E:
mov edx, dword ptr [ebp+8]
mov eax, dword ptr [edx+8]
mov ebx, 0
mov edx, dword ptr [ebp+8]
mov esi, dword ptr [edx+12]
mov ecx, 0
push ecx
push esi
push ebx
push eax
mov eax, [esp+0]
mul dword ptr [esp+8]
xchg eax, [esp+0]
imul eax, [esp+12]
add eax, edx
mov edx, [esp+4]
imul edx, [esp+8]
add edx, eax
mov [esp+4], edx
pop eax
pop ebx
add esp, 8
mov dword ptr [ebp-164], eax
mov dword ptr [ebp-160], ebx
fild qword ptr [ebp-164]
cmp dword ptr [ebp-160], 0
jns .L_0080
push 0x403f
push 0x80000000
push 0
fldt [esp]
add esp, 12
faddp
.L_0080:
mov ebx, dword ptr [ebp-152]
mov eax, 0
push eax
push ebx
fild qword ptr [esp]
add esp, 8
cmp eax, 0
jns .L_0081
push 0x403f
push 0x80000000
push 0
fldt [esp]
add esp, 12
faddp
.L_0081:
fld qword ptr [_Lt_0078]
fdivrp
fxch st(1)
fcompp
fnstsw ax
sahf
jbe .L_0061
push dword ptr [ebp-176]
call _fb_FileClose@4
mov dword ptr [ebp-4], 0
jmp .L_0050
.L_0061:
.L_0060:
mov eax, dword ptr [ebp-152]
mov ebx, 0
push ebx
push eax
push dword ptr [ebp-160]
push dword ptr [ebp-164]
push eax
mov eax, [esp+4]
mul dword ptr [esp+12]
xchg eax, [esp+4]
imul eax, [esp+16]
add eax, edx
mov edx, [esp+8]
imul edx, [esp+12]
add edx, eax
mov [esp+8], edx
pop eax
pop dword ptr [ebp-164]
pop dword ptr [ebp-160]
add esp, 8
push dword ptr [ebp-176]
call _fb_FileSize@4
add eax, 4294967278
adc edx, 4294967295
mov esi, eax
mov ebx, edx
mov edx, dword ptr [ebp-164]
mov eax, dword ptr [ebp-160]
cmp ebx, eax
ja .L_0063
jb .L_0082
cmp esi, edx
jae .L_0063
.L_0082:
push dword ptr [ebp-176]
call _fb_FileClose@4
mov dword ptr [ebp-4], 0
jmp .L_0050
.L_0063:
.L_0062:
mov edx, dword ptr [ebp-164]
mov dword ptr [ebp-156], edx
push dword ptr [ebp-156]
call _malloc
add esp, 4
mov edx, dword ptr [ebp+8]
mov dword ptr [edx], eax
mov eax, dword ptr [ebp+8]
cmp dword ptr [eax], 0
jne .L_0065
push dword ptr [ebp-176]
call _fb_FileClose@4
mov dword ptr [ebp-4], 0
jmp .L_0050
.L_0065:
.L_0064:
mov eax, dword ptr [ebp+8]
mov edx, dword ptr [eax]
mov dword ptr [ebp-180], edx
mov dword ptr [ebp-8], 0
mov edx, dword ptr [ebp-156]
dec edx
mov dword ptr [ebp-192], edx
jmp .L_0067
.L_006A:
push 1
lea edx, [ebp-184]
push edx
push 0
push dword ptr [ebp-176]
call _fb_FileGet@16
mov edx, dword ptr [ebp-180]
mov al, byte ptr [ebp-184]
mov byte ptr [edx], al
inc dword ptr [ebp-180]
.L_0068:
inc dword ptr [ebp-8]
.L_0067:
mov eax, dword ptr [ebp-192]
cmp dword ptr [ebp-8], eax
jle .L_006A
.L_0069:
mov dword ptr [ebp-8], 0
mov eax, dword ptr [ebp-156]
dec eax
mov dword ptr [ebp-192], eax
mov eax, dword ptr [ebp-152]
mov dword ptr [ebp-196], eax
jmp .L_006D
.L_0070:
mov eax, dword ptr [ebp+8]
mov edx, dword ptr [eax]
add edx, dword ptr [ebp-8]
mov al, byte ptr [edx]
mov byte ptr [ebp-168], al
mov eax, dword ptr [ebp+8]
mov edx, dword ptr [eax]
add edx, dword ptr [ebp-8]
mov eax, dword ptr [ebp+8]
mov esi, dword ptr [eax]
add esi, dword ptr [ebp-8]
mov al, byte ptr [edx+2]
mov byte ptr [esi], al
mov eax, dword ptr [ebp+8]
mov esi, dword ptr [eax]
add esi, dword ptr [ebp-8]
mov al, byte ptr [ebp-168]
mov byte ptr [esi+2], al
.L_006E:
mov eax, dword ptr [ebp-196]
add dword ptr [ebp-8], eax
.L_006D:
mov eax, dword ptr [ebp-192]
cmp dword ptr [ebp-8], eax
jle .L_0070
.L_006F:
push dword ptr [ebp-176]
call _fb_FileClose@4
mov eax, dword ptr [ebp+8]
lea esi, [eax+16]
push esi
push 1
call _glGenTextures@8
mov esi, dword ptr [ebp+8]
cmp dword ptr [esi+16], 0
jne .L_0072
mov esi, dword ptr [ebp+8]
push dword ptr [esi]
call _free
add esp, 4
mov esi, dword ptr [ebp+8]
mov dword ptr [esi], 0
mov dword ptr [ebp-4], 0
jmp .L_0050
.L_0072:
.L_0071:
mov esi, dword ptr [ebp+8]
push dword ptr [esi+16]
push 3553
call _glBindTexture@8
push dword ptr [_Lt_0079]
push 10241
push 3553
call _glTexParameterf@12
push dword ptr [_Lt_0079]
push 10240
push 3553
call _glTexParameterf@12
mov esi, dword ptr [ebp+8]
cmp dword ptr [esi+4], 24
jne .L_0074
mov dword ptr [ebp-172], 6407
.L_0074:
.L_0073:
mov esi, dword ptr [ebp+8]
push dword ptr [esi]
push 5121
push dword ptr [ebp-172]
push 0
mov esi, dword ptr [ebp+8]
push dword ptr [esi+12]
mov esi, dword ptr [ebp+8]
push dword ptr [esi+8]
push dword ptr [ebp-172]
push 0
push 3553
call _glTexImage2D@36
mov esi, dword ptr [ebp+8]
push dword ptr [esi]
call _free
add esp, 4
mov esi, dword ptr [ebp+8]
mov dword ptr [esi], 0
mov dword ptr [ebp-4], -1
.L_0050:
mov eax, dword ptr [ebp-4]
pop esi
pop ebx
mov esp, ebp
pop ebp
ret 8
.balign 16

.globl _BUILDFONT@0
_BUILDFONT@0:
push ebp
mov ebp, esp
sub esp, 12
.L_0083:
mov dword ptr [ebp-4], 0
mov dword ptr [ebp-8], 0
mov dword ptr [ebp-12], 0
push 256
call _glGenLists@4
mov dword ptr [_GBASE], eax
push dword ptr [_TEXTURES+16]
push 3553
call _glBindTexture@8
mov dword ptr [ebp-4], 0
.L_0088:
mov eax, dword ptr [ebp-4]
mov ecx, 16
cdq
idiv ecx
mov eax, edx
push eax
fild dword ptr [esp]
add esp, 4
fdiv qword ptr [_Lt_0089]
fstp dword ptr [ebp-8]
mov eax, dword ptr [ebp-4]
sar eax, 31
and eax, 15
add eax, dword ptr [ebp-4]
sar eax, 4
push eax
fild dword ptr [esp]
add esp, 4
fdiv qword ptr [_Lt_0089]
fstp dword ptr [ebp-12]
push 4864
mov eax, dword ptr [ebp-4]
add eax, dword ptr [_GBASE]
push eax
call _glNewList@8
push 7
call _glBegin@4
fld dword ptr [_Lt_008A]
fsub dword ptr [ebp-12]
fadd qword ptr [_Lt_008B]
sub esp,4
fstp dword ptr [esp]
push dword ptr [ebp-8]
call _glTexCoord2f@8
push 16
push 0
call _glVertex2i@8
fld dword ptr [_Lt_008A]
fsub dword ptr [ebp-12]
fadd qword ptr [_Lt_008B]
sub esp,4
fstp dword ptr [esp]
fld dword ptr [ebp-8]
fadd qword ptr [_Lt_008C]
sub esp,4
fstp dword ptr [esp]
call _glTexCoord2f@8
push 16
push 16
call _glVertex2i@8
fld dword ptr [_Lt_008A]
fsub dword ptr [ebp-12]
sub esp,4
fstp dword ptr [esp]
fld dword ptr [ebp-8]
fadd qword ptr [_Lt_008C]
sub esp,4
fstp dword ptr [esp]
call _glTexCoord2f@8
push 0
push 16
call _glVertex2i@8
fld dword ptr [_Lt_008A]
fsub dword ptr [ebp-12]
sub esp,4
fstp dword ptr [esp]
push dword ptr [ebp-8]
call _glTexCoord2f@8
push 0
push 0
call _glVertex2i@8
call _glEnd@0
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
push dword ptr [_Lt_008E+4]
push dword ptr [_Lt_008E]
call _glTranslated@24
call _glEndList@0
.L_0086:
inc dword ptr [ebp-4]
.L_0085:
cmp dword ptr [ebp-4], 255
jle .L_0088
.L_0087:
.L_0084:
mov esp, ebp
pop ebp
ret
.balign 16

.globl _GLPRINT
_GLPRINT:
push ebp
mov ebp, esp
sub esp, 284
push ebx
.L_008F:
lea eax, [ebp-256]
push eax
push edi
mov edi, eax
mov eax, 538976288
mov ecx, 64
rep stosd
pop edi
pop eax
mov dword ptr [ebp-260], 0
push -1
push dword ptr [ebp+20]
call _fb_StrLen@8
test eax, eax
jne .L_0092
jmp .L_0090
.L_0092:
.L_0091:
lea eax, [ebp+20]
add eax, 4
mov dword ptr [ebp-260], eax
push dword ptr [ebp-260]
mov eax, dword ptr [ebp+20]
push dword ptr [eax]
mov dword ptr [ebp-272], 0
mov dword ptr [ebp-268], 0
mov dword ptr [ebp-264], 0
push 0
push -2147483392
lea eax, [ebp-256]
push eax
push -1
lea eax, [ebp-272]
push eax
call _fb_StrAssign@20
push dword ptr [ebp-272]
call _vsprintf
add esp, 12
push -1
push -1
lea eax, [ebp-272]
push eax
push -2147483392
lea eax, [ebp-256]
push eax
call _fb_StrAssign@20
lea eax, [ebp-272]
push eax
call _fb_StrDelete@4
cmp dword ptr [ebp+16], 1
jle .L_0095
mov dword ptr [ebp+16], 1
.L_0095:
.L_0094:
push 3553
call _glEnable@4
call _glLoadIdentity@0
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
fild dword ptr [ebp+12]
sub esp,8
fstp qword ptr [esp]
fild dword ptr [ebp+8]
sub esp,8
fstp qword ptr [esp]
call _glTranslated@24
mov eax, dword ptr [ebp+16]
sal eax, 7
mov ebx, eax
add ebx, dword ptr [_GBASE]
add ebx, 4294967264
push ebx
call _glListBase@4
push dword ptr [_Lt_008A]
push dword ptr [_Lt_0097]
push dword ptr [_Lt_008A]
call _glScalef@12
lea ebx, [ebp-256]
push ebx
push 5121
mov dword ptr [ebp-284], 0
mov dword ptr [ebp-280], 0
mov dword ptr [ebp-276], 0
push 0
push -2147483392
lea ebx, [ebp-256]
push ebx
push -1
lea ebx, [ebp-284]
push ebx
call _fb_StrAssign@20
push dword ptr [ebp-284]
call _strlen
add esp, 4
push -1
push -1
lea ebx, [ebp-284]
push ebx
push -2147483392
lea ebx, [ebp-256]
push ebx
mov ebx, eax
call _fb_StrAssign@20
push ebx
call _glCallLists@12
lea ebx, [ebp-284]
push ebx
call _fb_StrDelete@4
push 3553
call _glDisable@4
.L_0090:
pop ebx
mov esp, ebp
pop ebp
ret
.balign 16
_fb_ctor__lesson24:
push ebp
mov ebp, esp
sub esp, 176
push ebx
.L_0002:
mov dword ptr [ebp-4], 0
mov dword ptr [ebp-8], 0
mov dword ptr [ebp-12], 0
mov dword ptr [ebp-16], 0
mov dword ptr [ebp-28], 0
mov dword ptr [ebp-24], 0
mov dword ptr [ebp-20], 0
mov dword ptr [ebp-32], 0
mov dword ptr [ebp-44], 0
mov dword ptr [ebp-40], 0
mov dword ptr [ebp-36], 0
mov dword ptr [ebp-48], 0
mov dword ptr [ebp-52], 0
push 59
push offset _Lt_0029
call _fb_StrAllocTempDescZEx@8
push eax
call _fb_GfxSetWindowTitle@4
push 0
push 2
push 0
push 16
push 18
call _fb_GfxScreen@20
mov dword ptr [ebp-12], 640
mov dword ptr [ebp-16], 480
push 480
push 640
push 0
push 0
call _glViewport@16
push 5889
call _glMatrixMode@4
call _glLoadIdentity@0
push dword ptr [_Lt_0098+4]
push dword ptr [_Lt_0098]
push dword ptr [_Lt_0099+4]
push dword ptr [_Lt_0099]
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
push dword ptr [_Lt_009A+4]
push dword ptr [_Lt_009A]
push dword ptr [_Lt_009B+4]
push dword ptr [_Lt_009B]
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
call _glOrtho@48
push 5888
call _glMatrixMode@4
call _glLoadIdentity@0
mov dword ptr [ebp-76], 0
mov dword ptr [ebp-72], 0
mov dword ptr [ebp-68], 0
push 0
push -1
push 15
push offset _Lt_002A
push -1
call _fb_ExePath@0
push eax
mov dword ptr [ebp-64], 0
mov dword ptr [ebp-60], 0
mov dword ptr [ebp-56], 0
lea eax, [ebp-64]
push eax
call _fb_StrConcat@20
push eax
push -1
lea eax, [ebp-76]
push eax
call _fb_StrAssign@20
lea eax, [ebp-76]
push eax
push offset _TEXTURES
call _LOADTGA@8
test eax, eax
mov eax, -1
je .L_00A5
xor eax, eax
.L_00A5:
mov dword ptr [ebp-80], eax
lea eax, [ebp-76]
push eax
call _fb_StrDelete@4
cmp dword ptr [ebp-80], 0
je .L_002E
push 1
call _fb_End@4
.L_002E:
.L_002D:
call _BUILDFONT@0
cmp dword ptr [_GBASE], 0
jne .L_0031
lea eax, [_TEXTURES+16]
push eax
push 1
call _glDeleteTextures@8
push 1
call _fb_End@4
.L_0031:
.L_0030:
push 7425
call _glShadeModel@4
push dword ptr [_Lt_009C]
push dword ptr [_Lt_009D]
push dword ptr [_Lt_009D]
push dword ptr [_Lt_009D]
call _glClearColor@16
push dword ptr [_Lt_0098+4]
push dword ptr [_Lt_0098]
call _glClearDepth@8
push dword ptr [_TEXTURES+16]
push 3553
call _glBindTexture@8
push 0
push 0
push 7937
call _glGetString@4
push eax
push -1
lea eax, [ebp-92]
push eax
call _fb_StrInit@20
push 0
push 0
push 7936
call _glGetString@4
push eax
push -1
lea eax, [ebp-104]
push eax
call _fb_StrInit@20
push 0
push 0
push 7938
call _glGetString@4
push eax
push -1
lea eax, [ebp-116]
push eax
call _fb_StrInit@20
.L_0032:
push 16640
call _glClear@4
push dword ptr [_Lt_009C]
push dword ptr [_Lt_009C]
push dword ptr [_Lt_008A]
call _glColor3f@12
mov dword ptr [ebp-128], 0
mov dword ptr [ebp-124], 0
mov dword ptr [ebp-120], 0
push 0
push 9
push offset _Lt_0035
push -1
lea eax, [ebp-128]
push eax
call _fb_StrAssign@20
lea eax, [ebp-128]
push eax
push 1
push 16
push 50
call _GLPRINT
add esp, 16
lea eax, [ebp-128]
push eax
call _fb_StrDelete@4
mov dword ptr [ebp-140], 0
mov dword ptr [ebp-136], 0
mov dword ptr [ebp-132], 0
push 0
push 7
push offset _Lt_0037
push -1
lea eax, [ebp-140]
push eax
call _fb_StrAssign@20
lea eax, [ebp-140]
push eax
push 1
push 48
push 80
call _GLPRINT
add esp, 16
lea eax, [ebp-140]
push eax
call _fb_StrDelete@4
mov dword ptr [ebp-152], 0
mov dword ptr [ebp-148], 0
mov dword ptr [ebp-144], 0
push 0
push 8
push offset _Lt_0039
push -1
lea eax, [ebp-152]
push eax
call _fb_StrAssign@20
lea eax, [ebp-152]
push eax
push 1
push 80
push 66
call _GLPRINT
add esp, 16
lea eax, [ebp-152]
push eax
call _fb_StrDelete@4
push dword ptr [_Lt_009E]
push dword ptr [_Lt_009F]
push dword ptr [_Lt_008A]
call _glColor3f@12
lea eax, [ebp-92]
push eax
push 1
push 16
push 200
call _GLPRINT
add esp, 16
lea eax, [ebp-104]
push eax
push 1
push 48
push 200
call _GLPRINT
add esp, 16
lea eax, [ebp-116]
push eax
push 1
push 80
push 200
call _GLPRINT
add esp, 16
push dword ptr [_Lt_008A]
push dword ptr [_Lt_009C]
push dword ptr [_Lt_009C]
call _glColor3f@12
mov dword ptr [ebp-164], 0
mov dword ptr [ebp-160], 0
mov dword ptr [ebp-156], 0
push 0
push 17
push offset _Lt_003B
push -1
lea eax, [ebp-164]
push eax
call _fb_StrAssign@20
lea eax, [ebp-164]
push eax
push 1
push 432
push 192
call _GLPRINT
add esp, 16
lea eax, [ebp-164]
push eax
call _fb_StrDelete@4
call _glLoadIdentity@0
push dword ptr [_Lt_008A]
push dword ptr [_Lt_008A]
push dword ptr [_Lt_008A]
call _glColor3f@12
push 3
call _glBegin@4
push dword ptr [_Lt_00A0+4]
push dword ptr [_Lt_00A0]
push dword ptr [_Lt_00A1+4]
push dword ptr [_Lt_00A1]
call _glVertex2d@16
push dword ptr [_Lt_00A0+4]
push dword ptr [_Lt_00A0]
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
call _glVertex2d@16
push dword ptr [_Lt_009A+4]
push dword ptr [_Lt_009A]
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
call _glVertex2d@16
push dword ptr [_Lt_009A+4]
push dword ptr [_Lt_009A]
push dword ptr [_Lt_00A1+4]
push dword ptr [_Lt_00A1]
call _glVertex2d@16
push dword ptr [_Lt_00A2+4]
push dword ptr [_Lt_00A2]
push dword ptr [_Lt_00A1+4]
push dword ptr [_Lt_00A1]
call _glVertex2d@16
call _glEnd@0
push 3
call _glBegin@4
push dword ptr [_Lt_00A2+4]
push dword ptr [_Lt_00A2]
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
call _glVertex2d@16
push dword ptr [_Lt_00A2+4]
push dword ptr [_Lt_00A2]
push dword ptr [_Lt_00A1+4]
push dword ptr [_Lt_00A1]
call _glVertex2d@16
push dword ptr [_Lt_0098+4]
push dword ptr [_Lt_0098]
push dword ptr [_Lt_00A1+4]
push dword ptr [_Lt_00A1]
call _glVertex2d@16
push dword ptr [_Lt_0098+4]
push dword ptr [_Lt_0098]
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
call _glVertex2d@16
push dword ptr [_Lt_00A0+4]
push dword ptr [_Lt_00A0]
push dword ptr [_Lt_008D+4]
push dword ptr [_Lt_008D]
call _glVertex2d@16
call _glEnd@0
fild dword ptr [ebp-16]
fmul qword ptr [_Lt_00A3]
sub esp, 4
fnstcw [esp]
mov eax, [esp]
and eax, 0b1111001111111111
or eax, 0b0000010000000000
push eax
fldcw [esp]
add esp, 4
frndint
fldcw [esp]
add esp, 4
sub esp, 4
fistp dword ptr [esp]
pop eax
push eax
mov eax, dword ptr [ebp-12]
add eax, -2
push eax
fild dword ptr [ebp-16]
fmul qword ptr [_Lt_00A4]
sub esp, 4
fnstcw [esp]
mov eax, [esp]
and eax, 0b1111001111111111
or eax, 0b0000010000000000
push eax
fldcw [esp]
add esp, 4
frndint
fldcw [esp]
add esp, 4
sub esp, 4
fistp dword ptr [esp]
pop eax
push eax
push 1
call _glScissor@16
push 3089
call _glEnable@4
push 0
push 0
push 7939
call _glGetString@4
push eax
push -1
lea eax, [ebp-44]
push eax
call _fb_StrAssign@20
mov dword ptr [ebp-48], 1
push 1
push offset _Lt_003D
call _fb_StrAllocTempDescZEx@8
push eax
lea eax, [ebp-44]
push eax
push dword ptr [ebp-48]
call _fb_StrInstr@12
mov dword ptr [ebp-52], eax
push 0
push -1
mov eax, dword ptr [ebp-52]
sub eax, dword ptr [ebp-48]
push eax
push dword ptr [ebp-48]
lea eax, [ebp-44]
push eax
call _fb_StrMid@12
push eax
push -1
lea eax, [ebp-28]
push eax
call _fb_StrAssign@20
mov eax, dword ptr [ebp-52]
inc eax
mov dword ptr [ebp-48], eax
mov dword ptr [ebp-32], 0
.L_003E:
cmp dword ptr [ebp-52], 0
je .L_003F
inc dword ptr [ebp-32]
mov eax, dword ptr [ebp-8]
cmp dword ptr [ebp-32], eax
jle .L_0041
mov eax, dword ptr [ebp-32]
mov dword ptr [ebp-8], eax
.L_0041:
.L_0040:
push dword ptr [_Lt_009C]
push dword ptr [_Lt_008A]
push dword ptr [_Lt_009C]
call _glColor3f@12
push dword ptr [ebp-32]
mov dword ptr [ebp-176], 0
mov dword ptr [ebp-172], 0
mov dword ptr [ebp-168], 0
push 0
push 3
push offset _Lt_0042
push -1
lea eax, [ebp-176]
push eax
call _fb_StrAssign@20
lea eax, [ebp-176]
push eax
push 0
mov eax, dword ptr [ebp-32]
sal eax, 5
add eax, 96
sub eax, dword ptr [ebp-4]
push eax
push 0
call _GLPRINT
add esp, 20
lea eax, [ebp-176]
push eax
call _fb_StrDelete@4
push dword ptr [_Lt_009C]
push dword ptr [_Lt_008A]
push dword ptr [_Lt_008A]
call _glColor3f@12
lea eax, [ebp-28]
push eax
push 0
mov eax, dword ptr [ebp-32]
sal eax, 5
add eax, 96
sub eax, dword ptr [ebp-4]
push eax
push 50
call _GLPRINT
add esp, 16
push 1
push offset _Lt_003D
call _fb_StrAllocTempDescZEx@8
push eax
lea eax, [ebp-44]
push eax
push dword ptr [ebp-48]
call _fb_StrInstr@12
mov dword ptr [ebp-52], eax
push 0
push -1
mov eax, dword ptr [ebp-52]
sub eax, dword ptr [ebp-48]
push eax
push dword ptr [ebp-48]
lea eax, [ebp-44]
push eax
call _fb_StrMid@12
push eax
push -1
lea eax, [ebp-28]
push eax
call _fb_StrAssign@20
mov eax, dword ptr [ebp-52]
inc eax
mov dword ptr [ebp-48], eax
jmp .L_003E
.L_003F:
push 3089
call _glDisable@4
call _glFlush@0
push 72
call _fb_Multikey@4
mov ebx, dword ptr [ebp-4]
test ebx, ebx
mov ebx, -1
jg .L_00A6
xor ebx, ebx
.L_00A6:
and eax, ebx
je .L_0045
add dword ptr [ebp-4], -2
.L_0045:
.L_0044:
push 80
call _fb_Multikey@4
mov ebx, dword ptr [ebp-8]
sal ebx, 5
add ebx, -288
mov ecx, dword ptr [ebp-4]
cmp ecx, ebx
mov ecx, -1
jl .L_00A7
xor ecx, ecx
.L_00A7:
and eax, ecx
je .L_0047
add dword ptr [ebp-4], 2
.L_0047:
.L_0046:
push -1
push -1
call _fb_GfxFlip@8
push 3
push offset _Lt_004A
push -1
call _fb_Inkey@0
push eax
call _fb_StrCompare@16
test eax, eax
jne .L_004C
jmp .L_0033
.L_004C:
.L_0034:
push 1
call _fb_Multikey@4
test eax, eax
je .L_0032
.L_0033:
.L_004D:
push 1
push offset _Lt_0000
push -1
call _fb_Inkey@0
push eax
call _fb_StrCompare@16
test eax, eax
je .L_004E
jmp .L_004D
.L_004E:
push 256
push dword ptr [_GBASE]
call _glDeleteLists@8
lea eax, [_TEXTURES+16]
push eax
push 1
call _glDeleteTextures@8
push 0
call _fb_End@4
lea eax, [ebp-116]
push eax
call _fb_StrDelete@4
lea eax, [ebp-104]
push eax
call _fb_StrDelete@4
lea eax, [ebp-92]
push eax
call _fb_StrDelete@4
lea eax, [ebp-44]
push eax
call _fb_StrDelete@4
lea eax, [ebp-28]
push eax
call _fb_StrDelete@4
.L_0003:
pop ebx
mov esp, ebp
pop ebp
ret

.section .fbctinf
.ascii "-l\0"
.ascii "opengl32\0"
.ascii "-l\0"
.ascii "glu32\0"
.ascii "-l\0"
.ascii "msvcrt\0"
.ascii "-gfx\0"


.section .data
.balign 4
_Lt_0000:	.ascii	"\0"

.section .bss
.balign 4
	.lcomm	_Lt_0024,36
.balign 4
	.lcomm	_TEXTURES,20
.balign 4
	.lcomm	_GBASE,4

.section .data
.balign 4
_Lt_0029:	.ascii	"NeHe's Token, Extensions, Scissoring & TGA Loading Tutorial\0"
.balign 4
_Lt_002A:	.ascii	"/data/Font.tga\0"
.balign 4
_Lt_0035:	.ascii	"Renderer\0"
.balign 4
_Lt_0037:	.ascii	"Vendor\0"
.balign 4
_Lt_0039:	.ascii	"Version\0"
.balign 4
_Lt_003B:	.ascii	"NeHe Productions\0"
.balign 4
_Lt_003D:	.ascii	" \0"
.balign 4
_Lt_0042:	.ascii	"%i\0"
.balign 4
_Lt_004A:	.ascii	"\377""k\0"
.balign 8
_Lt_0077:	.quad	0x4020000000000000
.balign 8
_Lt_0078:	.quad	0x41DFFFFFFFC00000
.balign 4
_Lt_0079:	.long	0x46180400
.balign 8
_Lt_0089:	.quad	0x4030000000000000
.balign 4
_Lt_008A:	.long	0x3F800000
.balign 8
_Lt_008B:	.quad	0xBFB0000000000000
.balign 8
_Lt_008C:	.quad	0x3FB0000000000000
.balign 8
_Lt_008D:	.quad	0x0000000000000000
.balign 8
_Lt_008E:	.quad	0x402C000000000000
.balign 4
_Lt_0097:	.long	0x40000000
.balign 8
_Lt_0098:	.quad	0x3FF0000000000000
.balign 8
_Lt_0099:	.quad	0xBFF0000000000000
.balign 8
_Lt_009A:	.quad	0x407E000000000000
.balign 8
_Lt_009B:	.quad	0x4084000000000000
.balign 4
_Lt_009C:	.long	0x3F000000
.balign 4
_Lt_009D:	.long	0x00000000
.balign 4
_Lt_009E:	.long	0x3ECCCCCD
.balign 4
_Lt_009F:	.long	0x3F333333
.balign 8
_Lt_00A0:	.quad	0x407A100000000000
.balign 8
_Lt_00A1:	.quad	0x4083F80000000000
.balign 8
_Lt_00A2:	.quad	0x4060000000000000
.balign 8
_Lt_00A3:	.quad	0x3FE32220BC382A13
.balign 8
_Lt_00A4:	.quad	0x3FC1554FBDAD7519

.section .ctors
.int _fb_ctor__lesson24
