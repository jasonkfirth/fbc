	.intel_syntax noprefix

.section .text
.balign 16

.globl _RESETOBJECTS@0
_RESETOBJECTS@0:
push ebp
mov ebp, esp
sub esp, 4
push ebx
.L_00E1:
mov dword ptr [_PLAYER+8], 0
mov dword ptr [_PLAYER+12], 0
mov dword ptr [_PLAYER], 0
mov dword ptr [_PLAYER+4], 0
mov dword ptr [_LOOP1], 0
mov eax, dword ptr [_LEVEL]
imul eax, dword ptr [_STAGE]
dec eax
mov dword ptr [ebp-4], eax
jmp .L_00E4
.L_00E7:
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_00E9]
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
fadd qword ptr [_Lt_00EA]
mov eax, dword ptr [_LOOP1]
imul eax, 20
fistp dword ptr [_ENEMY+eax+8]
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_00EB]
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
mov eax, dword ptr [_LOOP1]
imul eax, 20
fistp dword ptr [_ENEMY+eax+12]
mov eax, dword ptr [_LOOP1]
imul eax, 20
mov ebx, dword ptr [_ENEMY+eax+8]
imul ebx, 60
mov eax, dword ptr [_LOOP1]
imul eax, 20
mov dword ptr [_ENEMY+eax], ebx
mov ebx, dword ptr [_LOOP1]
imul ebx, 20
mov eax, dword ptr [_ENEMY+ebx+12]
imul eax, 40
mov ebx, dword ptr [_LOOP1]
imul ebx, 20
mov dword ptr [_ENEMY+ebx+4], eax
.L_00E5:
inc dword ptr [_LOOP1]
.L_00E4:
mov eax, dword ptr [ebp-4]
cmp dword ptr [_LOOP1], eax
jle .L_00E7
.L_00E6:
.L_00E2:
pop ebx
mov esp, ebp
pop ebp
ret
.balign 16

.globl _BUILDFONT@0
_BUILDFONT@0:
push ebp
mov ebp, esp
sub esp, 8
.L_00EC:
mov dword ptr [ebp-4], 0
mov dword ptr [ebp-8], 0
push 256
call _glGenLists@4
mov dword ptr [_GBASE], eax
push dword ptr [_TEXTURE]
push 3553
call _glBindTexture@8
mov dword ptr [_LOOP1], 0
.L_00F1:
mov eax, dword ptr [_LOOP1]
mov ecx, 16
cdq
idiv ecx
mov eax, edx
push eax
fild dword ptr [esp]
add esp, 4
fdiv qword ptr [_Lt_00F2]
fstp dword ptr [ebp-4]
mov eax, dword ptr [_LOOP1]
sar eax, 31
and eax, 15
add eax, dword ptr [_LOOP1]
sar eax, 4
push eax
fild dword ptr [esp]
add esp, 4
fmul dword ptr [_Lt_00F3]
fstp dword ptr [ebp-8]
push 4864
mov eax, dword ptr [_LOOP1]
add eax, dword ptr [_GBASE]
push eax
call _glNewList@8
push 7
call _glBegin@4
fld dword ptr [_Lt_00E8]
fsub dword ptr [ebp-8]
fadd qword ptr [_Lt_00F4]
sub esp,4
fstp dword ptr [esp]
push dword ptr [ebp-4]
call _glTexCoord2f@8
push 16
push 0
call _glVertex2i@8
fld dword ptr [_Lt_00E8]
fsub dword ptr [ebp-8]
fadd qword ptr [_Lt_00F4]
sub esp,4
fstp dword ptr [esp]
fld dword ptr [ebp-4]
fadd qword ptr [_Lt_00F5]
sub esp,4
fstp dword ptr [esp]
call _glTexCoord2f@8
push 16
push 16
call _glVertex2i@8
fld dword ptr [_Lt_00E8]
fsub dword ptr [ebp-8]
sub esp,4
fstp dword ptr [esp]
fld dword ptr [ebp-4]
fadd qword ptr [_Lt_00F5]
sub esp,4
fstp dword ptr [esp]
call _glTexCoord2f@8
push 0
push 16
call _glVertex2i@8
fld dword ptr [_Lt_00E8]
fsub dword ptr [ebp-8]
sub esp,4
fstp dword ptr [esp]
push dword ptr [ebp-4]
call _glTexCoord2f@8
push 0
push 0
call _glVertex2i@8
call _glEnd@0
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_00F7+4]
push dword ptr [_Lt_00F7]
call _glTranslated@24
call _glEndList@0
.L_00EF:
inc dword ptr [_LOOP1]
.L_00EE:
cmp dword ptr [_LOOP1], 255
jle .L_00F1
.L_00F0:
.L_00ED:
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
.L_00F8:
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
jne .L_00FB
jmp .L_00F9
.L_00FB:
.L_00FA:
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
jle .L_00FE
mov dword ptr [ebp+16], 1
.L_00FE:
.L_00FD:
push 3553
call _glEnable@4
call _glLoadIdentity@0
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
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
cmp dword ptr [ebp+16], 0
jne .L_0100
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0102]
push dword ptr [_Lt_0103]
call _glScalef@12
.L_0100:
.L_00FF:
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
.L_00F9:
pop ebx
mov esp, ebp
pop ebp
ret
.balign 16
_fb_ctor__lesson21:
push ebp
mov ebp, esp
sub esp, 1216
push ebx
push esi
.L_0002:
lea eax, [ebp-440]
push eax
push edi
mov edi, eax
xor eax, eax
mov ecx, 110
rep stosd
pop edi
pop eax
lea eax, [ebp-440]
mov dword ptr [ebp-488], eax
lea eax, [ebp-440]
mov dword ptr [ebp-484], eax
mov dword ptr [ebp-480], 440
mov dword ptr [ebp-476], 4
mov dword ptr [ebp-472], 2
mov dword ptr [ebp-468], 50
mov dword ptr [ebp-464], 11
mov dword ptr [ebp-460], 0
mov dword ptr [ebp-456], 10
mov dword ptr [ebp-452], 10
mov dword ptr [ebp-448], 0
mov dword ptr [ebp-444], 9
lea eax, [ebp-928]
push eax
push edi
mov edi, eax
xor eax, eax
mov ecx, 110
rep stosd
pop edi
pop eax
lea eax, [ebp-928]
mov dword ptr [ebp-976], eax
lea eax, [ebp-928]
mov dword ptr [ebp-972], eax
mov dword ptr [ebp-968], 440
mov dword ptr [ebp-964], 4
mov dword ptr [ebp-960], 2
mov dword ptr [ebp-956], 50
mov dword ptr [ebp-952], 10
mov dword ptr [ebp-948], 0
mov dword ptr [ebp-944], 9
mov dword ptr [ebp-940], 11
mov dword ptr [ebp-936], 0
mov dword ptr [ebp-932], 10
mov dword ptr [ebp-980], 0
mov dword ptr [ebp-984], 0
mov dword ptr [ebp-988], 0
mov dword ptr [ebp-992], -1
mov dword ptr [ebp-996], 0
mov dword ptr [ebp-1000], 0
mov dword ptr [ebp-1004], 0
mov dword ptr [ebp-1008], 3
mov dword ptr [ebp-1012], 5
mov dword ptr [ebp-1016], 1
lea eax, [ebp-1036]
push eax
push edi
mov edi, eax
xor eax, eax
mov ecx, 5
rep stosd
pop edi
pop eax
lea eax, [ebp-1060]
mov dword ptr [ebp-1096], eax
lea eax, [ebp-1060]
mov dword ptr [ebp-1092], eax
mov dword ptr [ebp-1088], 24
mov dword ptr [ebp-1084], 4
mov dword ptr [ebp-1080], 1
mov dword ptr [ebp-1076], 49
mov dword ptr [ebp-1072], 6
mov dword ptr [ebp-1068], 0
mov dword ptr [ebp-1064], 5
mov dword ptr [ebp-1060], 1
mov dword ptr [ebp-1056], 2
mov dword ptr [ebp-1052], 4
mov dword ptr [ebp-1048], 5
mov dword ptr [ebp-1044], 10
mov dword ptr [ebp-1040], 20
push 0
push 2
push 0
push 16
push 18
call _fb_GfxScreen@20
push 480
push 640
push 0
push 0
call _glViewport@16
push 5889
call _glMatrixMode@4
call _glLoadIdentity@0
push dword ptr [_Lt_0104+4]
push dword ptr [_Lt_0104]
push dword ptr [_Lt_0105+4]
push dword ptr [_Lt_0105]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_0106+4]
push dword ptr [_Lt_0106]
push dword ptr [_Lt_0107+4]
push dword ptr [_Lt_0107]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
call _glOrtho@48
push 5888
call _glMatrixMode@4
call _glLoadIdentity@0
mov dword ptr [ebp-1132], 0
mov dword ptr [ebp-1128], 0
mov dword ptr [ebp-1124], 0
mov dword ptr [ebp-1120], 1
mov dword ptr [ebp-1116], 1
mov dword ptr [ebp-1112], 17
mov dword ptr [ebp-1108], 0
mov dword ptr [ebp-1104], 0
mov dword ptr [ebp-1100], 0
push 262148
push 0
push 1
push 0
push -1
push 1
lea eax, [ebp-1132]
push eax
call _fb_ArrayRedimEx
add esp, 28
push 1
lea eax, [ebp-1132]
push eax
call _fb_ArrayUBound@8
push 1
lea ebx, [ebp-1132]
push ebx
mov ebx, eax
call _fb_ArrayLBound@8
cmp ebx, eax
jge .L_0049
push 1
call _fb_End@4
.L_0049:
push 0
push 1
lea eax, [ebp-1132]
push eax
call _fb_ArrayLBound@8
add eax, dword ptr [ebp-1132]
lea ebx, [eax]
push ebx
push 15
push offset _Lt_004A
push -1
call _fb_ExePath@0
push eax
mov dword ptr [ebp-1144], 0
mov dword ptr [ebp-1140], 0
mov dword ptr [ebp-1136], 0
lea eax, [ebp-1144]
push eax
call _fb_StrConcat@20
push eax
call _fb_GfxBload@12
push 0
push 1
lea eax, [ebp-1132]
push eax
call _fb_ArrayLBound@8
add eax, dword ptr [ebp-1132]
lea ebx, [eax]
push ebx
call _CREATETEXTURE@8
mov dword ptr [_TEXTURE], eax
push 0
push 1
lea eax, [ebp-1132]
push eax
call _fb_ArrayLBound@8
add eax, dword ptr [ebp-1132]
lea ebx, [eax]
push ebx
push 18
push offset _Lt_004C
push -1
call _fb_ExePath@0
push eax
mov dword ptr [ebp-1156], 0
mov dword ptr [ebp-1152], 0
mov dword ptr [ebp-1148], 0
lea eax, [ebp-1156]
push eax
call _fb_StrConcat@20
push eax
call _fb_GfxBload@12
push 0
push 1
lea eax, [ebp-1132]
push eax
call _fb_ArrayLBound@8
add eax, dword ptr [ebp-1132]
lea ebx, [eax]
push ebx
call _CREATETEXTURE@8
mov dword ptr [_TEXTURE+4], eax
cmp dword ptr [_TEXTURE], 0
mov eax, -1
je .L_011D
xor eax, eax
.L_011D:
cmp dword ptr [_TEXTURE+4], 0
mov ebx, -1
je .L_011E
xor ebx, ebx
.L_011E:
or eax, ebx
je .L_004F
push 1
call _fb_End@4
.L_004F:
call _BUILDFONT@0
push 7425
call _glShadeModel@4
push dword ptr [_Lt_0108]
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0109]
call _glClearColor@16
push dword ptr [_Lt_0104+4]
push dword ptr [_Lt_0104]
call _glClearDepth@8
push 4354
push 3154
call _glHint@8
push 3042
call _glEnable@4
push 771
push 770
call _glBlendFunc@8
push 0
call _fb_Timer@0
sub esp,8
fstp qword ptr [esp]
call _fb_Randomize@12
call _RESETOBJECTS@0
.L_0050:
call _fb_Timer@0
fstp dword ptr [ebp-996]
push 16640
call _glClear@4
push dword ptr [_TEXTURE]
push 3553
call _glBindTexture@8
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0108]
push dword ptr [_Lt_00E8]
call _glColor3f@12
mov dword ptr [ebp-1168], 0
mov dword ptr [ebp-1164], 0
mov dword ptr [ebp-1160], 0
push 0
push 11
push offset _Lt_0053
push -1
lea ebx, [ebp-1168]
push ebx
call _fb_StrAssign@20
lea ebx, [ebp-1168]
push ebx
push 0
push 24
push 207
call _GLPRINT
add esp, 16
lea ebx, [ebp-1168]
push ebx
call _fb_StrDelete@4
push dword ptr [_Lt_0109]
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_00E8]
call _glColor3f@12
push dword ptr [ebp-1016]
mov dword ptr [ebp-1180], 0
mov dword ptr [ebp-1176], 0
mov dword ptr [ebp-1172], 0
push 0
push 10
push offset _Lt_0055
push -1
lea ebx, [ebp-1180]
push ebx
call _fb_StrAssign@20
lea ebx, [ebp-1180]
push ebx
push 1
push 20
push 20
call _GLPRINT
add esp, 20
lea ebx, [ebp-1180]
push ebx
call _fb_StrDelete@4
push dword ptr [_STAGE]
mov dword ptr [ebp-1192], 0
mov dword ptr [ebp-1188], 0
mov dword ptr [ebp-1184], 0
push 0
push 10
push offset _Lt_0057
push -1
lea ebx, [ebp-1192]
push ebx
call _fb_StrAssign@20
lea ebx, [ebp-1192]
push ebx
push 1
push 40
push 20
call _GLPRINT
add esp, 20
lea ebx, [ebp-1192]
push ebx
call _fb_StrDelete@4
cmp dword ptr [ebp-988], 0
je .L_005A
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_010A]
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
mov bl, byte ptr [esp]
add esp, 4
push ebx
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_010A]
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
mov bl, byte ptr [esp]
add esp, 4
push ebx
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_010A]
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
mov bl, byte ptr [esp]
add esp, 4
push ebx
call _glColor3ub@12
mov dword ptr [ebp-1204], 0
mov dword ptr [ebp-1200], 0
mov dword ptr [ebp-1196], 0
push 0
push 10
push offset _Lt_005B
push -1
lea ebx, [ebp-1204]
push ebx
call _fb_StrAssign@20
lea ebx, [ebp-1204]
push ebx
push 1
push 20
push 472
call _GLPRINT
add esp, 16
lea ebx, [ebp-1204]
push ebx
call _fb_StrDelete@4
mov dword ptr [ebp-1216], 0
mov dword ptr [ebp-1212], 0
mov dword ptr [ebp-1208], 0
push 0
push 12
push offset _Lt_005D
push -1
lea ebx, [ebp-1216]
push ebx
call _fb_StrAssign@20
lea ebx, [ebp-1216]
push ebx
push 1
push 40
push 456
call _GLPRINT
add esp, 16
lea ebx, [ebp-1216]
push ebx
call _fb_StrDelete@4
.L_005A:
.L_0059:
mov dword ptr [_LOOP1], 0
mov ebx, dword ptr [ebp-1012]
add ebx, -2
mov dword ptr [ebp-1196], ebx
jmp .L_0060
.L_0063:
call _glLoadIdentity@0
push dword ptr [_Lt_0109]
push dword ptr [_Lt_010B]
fild dword ptr [_LOOP1]
fmul qword ptr [_Lt_010C]
fadd qword ptr [_Lt_010D]
sub esp,4
fstp dword ptr [esp]
call _glTranslatef@12
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0109]
fld dword ptr [_PLAYER+16]
fchs
sub esp,4
fstp dword ptr [esp]
call _glRotatef@16
push dword ptr [_Lt_0109]
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0109]
call _glColor3f@12
push 1
call _glBegin@4
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
call _glVertex2d@16
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
call _glVertex2d@16
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
call _glVertex2d@16
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
call _glVertex2d@16
call _glEnd@0
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0109]
fld dword ptr [_PLAYER+16]
fchs
fmul qword ptr [_Lt_010F]
sub esp,4
fstp dword ptr [esp]
call _glRotatef@16
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0110]
push dword ptr [_Lt_0109]
call _glColor3f@12
push 1
call _glBegin@4
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
call _glVertex2d@16
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
call _glVertex2d@16
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
call _glVertex2d@16
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
call _glVertex2d@16
call _glEnd@0
.L_0061:
inc dword ptr [_LOOP1]
.L_0060:
mov ebx, dword ptr [ebp-1196]
cmp dword ptr [_LOOP1], ebx
jle .L_0063
.L_0062:
mov dword ptr [ebp-984], -1
push dword ptr [_Lt_0102]
call _glLineWidth@4
push 2848
call _glDisable@4
call _glLoadIdentity@0
mov dword ptr [_LOOP1], 0
.L_0067:
mov dword ptr [ebp-1000], 0
.L_006B:
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0108]
push dword ptr [_Lt_0109]
call _glColor3f@12
cmp dword ptr [_LOOP1], 10
jge .L_006D
mov ebx, dword ptr [_LOOP1]
imul ebx, 11
add ebx, dword ptr [ebp-1000]
cmp dword ptr [ebp+ebx*4-928], 0
je .L_006F
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_00E8]
call _glColor3f@12
.L_006F:
.L_006E:
mov ebx, dword ptr [_LOOP1]
imul ebx, 11
add ebx, dword ptr [ebp-1000]
mov eax, dword ptr [ebp+ebx*4-928]
not eax
test eax, eax
je .L_0071
mov dword ptr [ebp-984], 0
.L_0071:
.L_0070:
push 1
call _glBegin@4
mov eax, dword ptr [ebp-1000]
imul eax, 40
add eax, 70
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
mov eax, dword ptr [_LOOP1]
imul eax, 60
add eax, 20
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
call _glVertex2d@16
mov eax, dword ptr [ebp-1000]
imul eax, 40
add eax, 70
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
mov eax, dword ptr [_LOOP1]
imul eax, 60
add eax, 80
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
call _glVertex2d@16
call _glEnd@0
.L_006D:
.L_006C:
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0108]
push dword ptr [_Lt_0109]
call _glColor3f@12
cmp dword ptr [ebp-1000], 10
jge .L_0073
mov eax, dword ptr [_LOOP1]
imul eax, 10
add eax, dword ptr [ebp-1000]
cmp dword ptr [ebp+eax*4-440], 0
je .L_0075
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_00E8]
call _glColor3f@12
.L_0075:
.L_0074:
mov eax, dword ptr [_LOOP1]
imul eax, 10
add eax, dword ptr [ebp-1000]
mov ebx, dword ptr [ebp+eax*4-440]
not ebx
test ebx, ebx
je .L_0077
mov dword ptr [ebp-984], 0
.L_0077:
.L_0076:
push 1
call _glBegin@4
mov ebx, dword ptr [ebp-1000]
imul ebx, 40
add ebx, 70
push ebx
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
mov ebx, dword ptr [_LOOP1]
imul ebx, 60
add ebx, 20
push ebx
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
call _glVertex2d@16
mov ebx, dword ptr [ebp-1000]
imul ebx, 40
add ebx, 110
push ebx
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
mov ebx, dword ptr [_LOOP1]
imul ebx, 60
add ebx, 20
push ebx
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
call _glVertex2d@16
call _glEnd@0
.L_0073:
.L_0072:
push 3553
call _glEnable@4
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_00E8]
call _glColor3f@12
push dword ptr [_TEXTURE+4]
push 3553
call _glBindTexture@8
mov ebx, dword ptr [_LOOP1]
cmp ebx, 10
mov ebx, -1
jl .L_011F
xor ebx, ebx
.L_011F:
mov eax, dword ptr [ebp-1000]
cmp eax, 10
mov eax, -1
jl .L_0120
xor eax, eax
.L_0120:
and ebx, eax
je .L_0079
mov eax, dword ptr [_LOOP1]
imul eax, 11
add eax, dword ptr [ebp-1000]
mov ebx, dword ptr [_LOOP1]
imul ebx, 11
add ebx, dword ptr [ebp-1000]
mov ecx, dword ptr [ebp+ebx*4-924]
and ecx, dword ptr [ebp+eax*4-928]
mov eax, dword ptr [_LOOP1]
imul eax, 10
add eax, dword ptr [ebp-1000]
and ecx, dword ptr [ebp+eax*4-440]
mov eax, dword ptr [_LOOP1]
imul eax, 10
add eax, dword ptr [ebp-1000]
and ecx, dword ptr [ebp+eax*4-400]
je .L_007B
push 7
call _glBegin@4
fild dword ptr [ebp-1000]
fdiv qword ptr [_Lt_0113]
fchs
fadd qword ptr [_Lt_0104]
sub esp,4
fstp dword ptr [esp]
fild dword ptr [_LOOP1]
fdiv qword ptr [_Lt_0113]
fadd qword ptr [_Lt_0114]
sub esp,4
fstp dword ptr [esp]
call _glTexCoord2f@8
mov eax, dword ptr [ebp-1000]
imul eax, 40
add eax, 71
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
mov eax, dword ptr [_LOOP1]
imul eax, 60
add eax, 79
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
call _glVertex2d@16
fild dword ptr [ebp-1000]
fdiv qword ptr [_Lt_0113]
fchs
fadd qword ptr [_Lt_0104]
sub esp,4
fstp dword ptr [esp]
fild dword ptr [_LOOP1]
fdiv qword ptr [_Lt_0113]
sub esp,4
fstp dword ptr [esp]
call _glTexCoord2f@8
mov eax, dword ptr [ebp-1000]
imul eax, 40
add eax, 71
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
mov eax, dword ptr [_LOOP1]
imul eax, 60
add eax, 21
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
call _glVertex2d@16
fild dword ptr [ebp-1000]
fdiv qword ptr [_Lt_0113]
fadd qword ptr [_Lt_0114]
fchs
fadd qword ptr [_Lt_0104]
sub esp,4
fstp dword ptr [esp]
fild dword ptr [_LOOP1]
fdiv qword ptr [_Lt_0113]
sub esp,4
fstp dword ptr [esp]
call _glTexCoord2f@8
mov eax, dword ptr [ebp-1000]
imul eax, 40
add eax, 109
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
mov eax, dword ptr [_LOOP1]
imul eax, 60
add eax, 21
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
call _glVertex2d@16
fild dword ptr [ebp-1000]
fdiv qword ptr [_Lt_0113]
fadd qword ptr [_Lt_0114]
fchs
fadd qword ptr [_Lt_0104]
sub esp,4
fstp dword ptr [esp]
fild dword ptr [_LOOP1]
fdiv qword ptr [_Lt_0113]
fadd qword ptr [_Lt_0114]
sub esp,4
fstp dword ptr [esp]
call _glTexCoord2f@8
mov eax, dword ptr [ebp-1000]
imul eax, 40
add eax, 109
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
mov eax, dword ptr [_LOOP1]
imul eax, 60
add eax, 79
push eax
fild dword ptr [esp]
add esp, 4
sub esp,8
fstp qword ptr [esp]
call _glVertex2d@16
call _glEnd@0
.L_007B:
.L_007A:
.L_0079:
.L_0078:
push 3553
call _glDisable@4
.L_0069:
inc dword ptr [ebp-1000]
.L_0068:
cmp dword ptr [ebp-1000], 10
jle .L_006B
.L_006A:
.L_0065:
inc dword ptr [_LOOP1]
.L_0064:
cmp dword ptr [_LOOP1], 10
jle .L_0067
.L_0066:
push dword ptr [_Lt_00E8]
call _glLineWidth@4
cmp dword ptr [ebp-992], 0
je .L_007D
push 2848
call _glEnable@4
.L_007D:
.L_007C:
cmp dword ptr [ebp-1036], 1
jne .L_007F
call _glLoadIdentity@0
push dword ptr [_Lt_0109]
mov eax, dword ptr [ebp-1024]
imul eax, 40
push eax
fild dword ptr [esp]
add esp, 4
fadd qword ptr [_Lt_0115]
sub esp,4
fstp dword ptr [esp]
mov eax, dword ptr [ebp-1028]
imul eax, 60
push eax
fild dword ptr [esp]
add esp, 4
fadd qword ptr [_Lt_0116]
sub esp,4
fstp dword ptr [esp]
call _glTranslatef@12
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0109]
push dword ptr [ebp-1020]
call _glRotatef@16
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_010A]
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
mov al, byte ptr [esp]
add esp, 4
push eax
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_010A]
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
mov al, byte ptr [esp]
add esp, 4
push eax
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_010A]
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
mov al, byte ptr [esp]
add esp, 4
push eax
call _glColor3ub@12
push 1
call _glBegin@4
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
call _glVertex2d@16
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
call _glVertex2d@16
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
call _glVertex2d@16
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
call _glVertex2d@16
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
call _glVertex2d@16
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
call _glVertex2d@16
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
call _glVertex2d@16
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
call _glVertex2d@16
call _glEnd@0
.L_007F:
.L_007E:
call _glLoadIdentity@0
push dword ptr [_Lt_0109]
fild dword ptr [_PLAYER+4]
fadd qword ptr [_Lt_0115]
sub esp,4
fstp dword ptr [esp]
fild dword ptr [_PLAYER]
fadd qword ptr [_Lt_0116]
sub esp,4
fstp dword ptr [esp]
call _glTranslatef@12
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0109]
push dword ptr [_PLAYER+16]
call _glRotatef@16
push dword ptr [_Lt_0109]
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0109]
call _glColor3f@12
push 1
call _glBegin@4
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
call _glVertex2d@16
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
call _glVertex2d@16
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
call _glVertex2d@16
push dword ptr [_Lt_00EA+4]
push dword ptr [_Lt_00EA]
push dword ptr [_Lt_010E+4]
push dword ptr [_Lt_010E]
call _glVertex2d@16
call _glEnd@0
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0109]
fld dword ptr [_PLAYER+16]
fmul qword ptr [_Lt_010F]
sub esp,4
fstp dword ptr [esp]
call _glRotatef@16
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0110]
push dword ptr [_Lt_0109]
call _glColor3f@12
push 1
call _glBegin@4
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
call _glVertex2d@16
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
call _glVertex2d@16
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
call _glVertex2d@16
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
call _glVertex2d@16
call _glEnd@0
mov dword ptr [_LOOP1], 0
mov eax, dword ptr [_LEVEL]
imul eax, dword ptr [_STAGE]
dec eax
mov dword ptr [ebp-1196], eax
jmp .L_0081
.L_0084:
call _glLoadIdentity@0
push dword ptr [_Lt_0109]
mov eax, dword ptr [_LOOP1]
imul eax, 20
fild dword ptr [_ENEMY+eax+4]
fadd qword ptr [_Lt_0115]
sub esp,4
fstp dword ptr [esp]
mov eax, dword ptr [_LOOP1]
imul eax, 20
fild dword ptr [_ENEMY+eax]
fadd qword ptr [_Lt_0116]
sub esp,4
fstp dword ptr [esp]
call _glTranslatef@12
push dword ptr [_Lt_0108]
push dword ptr [_Lt_0108]
push dword ptr [_Lt_00E8]
call _glColor3f@12
push 1
call _glBegin@4
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
call _glVertex2d@16
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
call _glVertex2d@16
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
call _glVertex2d@16
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
call _glVertex2d@16
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
call _glVertex2d@16
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
call _glVertex2d@16
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
call _glVertex2d@16
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
push dword ptr [_Lt_00F6+4]
push dword ptr [_Lt_00F6]
call _glVertex2d@16
call _glEnd@0
push dword ptr [_Lt_00E8]
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0109]
mov eax, dword ptr [_LOOP1]
imul eax, 20
push dword ptr [_ENEMY+eax+16]
call _glRotatef@16
push dword ptr [_Lt_0109]
push dword ptr [_Lt_0109]
push dword ptr [_Lt_00E8]
call _glColor3f@12
push 1
call _glBegin@4
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
call _glVertex2d@16
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
call _glVertex2d@16
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
call _glVertex2d@16
push dword ptr [_Lt_0111+4]
push dword ptr [_Lt_0111]
push dword ptr [_Lt_0112+4]
push dword ptr [_Lt_0112]
call _glVertex2d@16
call _glEnd@0
.L_0082:
inc dword ptr [_LOOP1]
.L_0081:
mov eax, dword ptr [ebp-1196]
cmp dword ptr [_LOOP1], eax
jle .L_0084
.L_0083:
.L_0085:
call _fb_Timer@0
fld dword ptr [ebp-996]
mov eax, dword ptr [ebp-1008]
fild dword ptr [ebp+eax*4-1060]
fmul qword ptr [_Lt_0117]
fdiv qword ptr [_Lt_0118]
fxch st(1)
faddp
fxch st(1)
fxch
fcompp
fnstsw ax
sahf
jbe .L_0086
jmp .L_0085
.L_0086:
mov eax, dword ptr [ebp-988]
not eax
test eax, eax
je .L_0088
mov dword ptr [_LOOP1], 0
mov eax, dword ptr [_LEVEL]
imul eax, dword ptr [_STAGE]
dec eax
mov dword ptr [ebp-1196], eax
jmp .L_008A
.L_008D:
mov eax, dword ptr [_LOOP1]
imul eax, 20
mov ecx, dword ptr [_ENEMY+eax+8]
cmp ecx, dword ptr [_PLAYER+8]
mov ecx, -1
jl .L_0121
xor ecx, ecx
.L_0121:
mov eax, dword ptr [_LOOP1]
imul eax, 20
mov ebx, dword ptr [_LOOP1]
imul ebx, 20
mov esi, dword ptr [_ENEMY+ebx+12]
imul esi, 40
cmp esi, dword ptr [_ENEMY+eax+4]
mov esi, -1
je .L_0122
xor esi, esi
.L_0122:
and ecx, esi
je .L_008F
mov esi, dword ptr [_LOOP1]
imul esi, 20
inc dword ptr [_ENEMY+esi+8]
.L_008F:
.L_008E:
mov esi, dword ptr [_LOOP1]
imul esi, 20
mov ecx, dword ptr [_ENEMY+esi+8]
cmp ecx, dword ptr [_PLAYER+8]
mov ecx, -1
jg .L_0123
xor ecx, ecx
.L_0123:
mov esi, dword ptr [_LOOP1]
imul esi, 20
mov eax, dword ptr [_LOOP1]
imul eax, 20
mov ebx, dword ptr [_ENEMY+eax+12]
imul ebx, 40
cmp ebx, dword ptr [_ENEMY+esi+4]
mov ebx, -1
je .L_0124
xor ebx, ebx
.L_0124:
and ecx, ebx
je .L_0091
mov ebx, dword ptr [_LOOP1]
imul ebx, 20
dec dword ptr [_ENEMY+ebx+8]
.L_0091:
.L_0090:
mov ebx, dword ptr [_LOOP1]
imul ebx, 20
mov ecx, dword ptr [_ENEMY+ebx+12]
cmp ecx, dword ptr [_PLAYER+12]
mov ecx, -1
jl .L_0125
xor ecx, ecx
.L_0125:
mov ebx, dword ptr [_LOOP1]
imul ebx, 20
mov esi, dword ptr [_LOOP1]
imul esi, 20
mov eax, dword ptr [_ENEMY+esi+8]
imul eax, 60
cmp eax, dword ptr [_ENEMY+ebx]
mov eax, -1
je .L_0126
xor eax, eax
.L_0126:
and ecx, eax
je .L_0093
mov eax, dword ptr [_LOOP1]
imul eax, 20
inc dword ptr [_ENEMY+eax+12]
.L_0093:
.L_0092:
mov eax, dword ptr [_LOOP1]
imul eax, 20
mov ecx, dword ptr [_ENEMY+eax+12]
cmp ecx, dword ptr [_PLAYER+12]
mov ecx, -1
jg .L_0127
xor ecx, ecx
.L_0127:
mov eax, dword ptr [_LOOP1]
imul eax, 20
mov ebx, dword ptr [_LOOP1]
imul ebx, 20
mov esi, dword ptr [_ENEMY+ebx+8]
imul esi, 60
cmp esi, dword ptr [_ENEMY+eax]
mov esi, -1
je .L_0128
xor esi, esi
.L_0128:
and ecx, esi
je .L_0095
mov esi, dword ptr [_LOOP1]
imul esi, 20
dec dword ptr [_ENEMY+esi+12]
.L_0095:
.L_0094:
mov esi, 3
sub esi, dword ptr [_LEVEL]
mov ecx, dword ptr [ebp-1004]
cmp ecx, esi
mov ecx, -1
jg .L_0129
xor ecx, ecx
.L_0129:
mov esi, dword ptr [ebp-1036]
cmp esi, 2
mov esi, -1
jne .L_012A
xor esi, esi
.L_012A:
and ecx, esi
je .L_0097
mov dword ptr [ebp-1004], 0
mov dword ptr [ebp-1000], 0
mov esi, dword ptr [_LEVEL]
imul esi, dword ptr [_STAGE]
dec esi
mov dword ptr [ebp-1200], esi
jmp .L_0099
.L_009C:
mov esi, dword ptr [ebp-1000]
imul esi, 20
mov ecx, dword ptr [ebp-1000]
imul ecx, 20
mov eax, dword ptr [_ENEMY+ecx+8]
imul eax, 60
cmp dword ptr [_ENEMY+esi], eax
jge .L_009E
mov eax, dword ptr [ebp-1000]
imul eax, 20
mov esi, dword ptr [ebp-1008]
mov ecx, dword ptr [ebp+esi*4-1060]
add dword ptr [_ENEMY+eax], ecx
mov ecx, dword ptr [ebp-1000]
imul ecx, 20
mov eax, dword ptr [ebp-1008]
fld dword ptr [_ENEMY+ecx+16]
fiadd dword ptr [ebp+eax*4-1060]
mov eax, dword ptr [ebp-1000]
imul eax, 20
fstp dword ptr [_ENEMY+eax+16]
.L_009E:
.L_009D:
mov eax, dword ptr [ebp-1000]
imul eax, 20
mov ecx, dword ptr [ebp-1000]
imul ecx, 20
mov esi, dword ptr [_ENEMY+ecx+8]
imul esi, 60
cmp dword ptr [_ENEMY+eax], esi
jle .L_00A0
mov esi, dword ptr [ebp-1000]
imul esi, 20
mov eax, dword ptr [ebp-1008]
mov ecx, dword ptr [ebp+eax*4-1060]
sub dword ptr [_ENEMY+esi], ecx
mov ecx, dword ptr [ebp-1000]
imul ecx, 20
mov esi, dword ptr [ebp-1008]
fld dword ptr [_ENEMY+ecx+16]
fisub dword ptr [ebp+esi*4-1060]
mov esi, dword ptr [ebp-1000]
imul esi, 20
fstp dword ptr [_ENEMY+esi+16]
.L_00A0:
.L_009F:
mov esi, dword ptr [ebp-1000]
imul esi, 20
mov ecx, dword ptr [ebp-1000]
imul ecx, 20
mov eax, dword ptr [_ENEMY+ecx+12]
imul eax, 40
cmp dword ptr [_ENEMY+esi+4], eax
jge .L_00A2
mov eax, dword ptr [ebp-1000]
imul eax, 20
mov esi, dword ptr [ebp-1008]
mov ecx, dword ptr [ebp+esi*4-1060]
add dword ptr [_ENEMY+eax+4], ecx
mov ecx, dword ptr [ebp-1000]
imul ecx, 20
mov eax, dword ptr [ebp-1008]
fld dword ptr [_ENEMY+ecx+16]
fiadd dword ptr [ebp+eax*4-1060]
mov eax, dword ptr [ebp-1000]
imul eax, 20
fstp dword ptr [_ENEMY+eax+16]
.L_00A2:
.L_00A1:
mov eax, dword ptr [ebp-1000]
imul eax, 20
mov ecx, dword ptr [ebp-1000]
imul ecx, 20
mov esi, dword ptr [_ENEMY+ecx+12]
imul esi, 40
cmp dword ptr [_ENEMY+eax+4], esi
jle .L_00A4
mov esi, dword ptr [ebp-1000]
imul esi, 20
mov eax, dword ptr [ebp-1008]
mov ecx, dword ptr [ebp+eax*4-1060]
sub dword ptr [_ENEMY+esi+4], ecx
mov ecx, dword ptr [ebp-1000]
imul ecx, 20
mov esi, dword ptr [ebp-1008]
fld dword ptr [_ENEMY+ecx+16]
fisub dword ptr [ebp+esi*4-1060]
mov esi, dword ptr [ebp-1000]
imul esi, 20
fstp dword ptr [_ENEMY+esi+16]
.L_00A4:
.L_00A3:
.L_009A:
inc dword ptr [ebp-1000]
.L_0099:
mov esi, dword ptr [ebp-1200]
cmp dword ptr [ebp-1000], esi
jle .L_009C
.L_009B:
.L_0097:
.L_0096:
mov esi, dword ptr [_LOOP1]
imul esi, 20
mov ecx, dword ptr [_PLAYER]
cmp ecx, dword ptr [_ENEMY+esi]
mov ecx, -1
je .L_012B
xor ecx, ecx
.L_012B:
mov esi, dword ptr [_LOOP1]
imul esi, 20
mov eax, dword ptr [_PLAYER+4]
cmp eax, dword ptr [_ENEMY+esi+4]
mov eax, -1
je .L_012C
xor eax, eax
.L_012C:
and ecx, eax
je .L_00A6
dec dword ptr [ebp-1012]
cmp dword ptr [ebp-1012], 0
jne .L_00A8
mov dword ptr [ebp-988], -1
.L_00A8:
.L_00A7:
call _RESETOBJECTS@0
.L_00A6:
.L_00A5:
.L_008B:
inc dword ptr [_LOOP1]
.L_008A:
mov eax, dword ptr [ebp-1196]
cmp dword ptr [_LOOP1], eax
jle .L_008D
.L_008C:
push 77
call _fb_Multikey@4
mov ecx, dword ptr [_PLAYER+8]
cmp ecx, 10
mov ecx, -1
jl .L_012D
xor ecx, ecx
.L_012D:
and eax, ecx
mov ecx, dword ptr [_PLAYER+8]
imul ecx, 60
cmp ecx, dword ptr [_PLAYER]
mov ecx, -1
je .L_012E
xor ecx, ecx
.L_012E:
and eax, ecx
mov ecx, dword ptr [_PLAYER+12]
imul ecx, 40
cmp ecx, dword ptr [_PLAYER+4]
mov ecx, -1
je .L_012F
xor ecx, ecx
.L_012F:
and eax, ecx
je .L_00AA
mov ecx, dword ptr [_PLAYER+8]
imul ecx, 11
add ecx, dword ptr [_PLAYER+12]
mov dword ptr [ebp+ecx*4-928], -1
inc dword ptr [_PLAYER+8]
.L_00AA:
.L_00A9:
push 75
call _fb_Multikey@4
mov ecx, dword ptr [_PLAYER+8]
test ecx, ecx
mov ecx, -1
jg .L_0130
xor ecx, ecx
.L_0130:
and eax, ecx
mov ecx, dword ptr [_PLAYER+8]
imul ecx, 60
cmp ecx, dword ptr [_PLAYER]
mov ecx, -1
je .L_0131
xor ecx, ecx
.L_0131:
and eax, ecx
mov ecx, dword ptr [_PLAYER+12]
imul ecx, 40
cmp ecx, dword ptr [_PLAYER+4]
mov ecx, -1
je .L_0132
xor ecx, ecx
.L_0132:
and eax, ecx
je .L_00AC
dec dword ptr [_PLAYER+8]
mov ecx, dword ptr [_PLAYER+8]
imul ecx, 11
add ecx, dword ptr [_PLAYER+12]
mov dword ptr [ebp+ecx*4-928], -1
.L_00AC:
.L_00AB:
push 80
call _fb_Multikey@4
mov ecx, dword ptr [_PLAYER+12]
cmp ecx, 10
mov ecx, -1
jl .L_0133
xor ecx, ecx
.L_0133:
and eax, ecx
mov ecx, dword ptr [_PLAYER+8]
imul ecx, 60
cmp ecx, dword ptr [_PLAYER]
mov ecx, -1
je .L_0134
xor ecx, ecx
.L_0134:
and eax, ecx
mov ecx, dword ptr [_PLAYER+12]
imul ecx, 40
cmp ecx, dword ptr [_PLAYER+4]
mov ecx, -1
je .L_0135
xor ecx, ecx
.L_0135:
and eax, ecx
je .L_00AE
mov ecx, dword ptr [_PLAYER+8]
imul ecx, 10
add ecx, dword ptr [_PLAYER+12]
mov dword ptr [ebp+ecx*4-440], -1
inc dword ptr [_PLAYER+12]
.L_00AE:
.L_00AD:
push 72
call _fb_Multikey@4
mov ecx, dword ptr [_PLAYER+12]
test ecx, ecx
mov ecx, -1
jg .L_0136
xor ecx, ecx
.L_0136:
and eax, ecx
mov ecx, dword ptr [_PLAYER+8]
imul ecx, 60
cmp ecx, dword ptr [_PLAYER]
mov ecx, -1
je .L_0137
xor ecx, ecx
.L_0137:
and eax, ecx
mov ecx, dword ptr [_PLAYER+12]
imul ecx, 40
cmp ecx, dword ptr [_PLAYER+4]
mov ecx, -1
je .L_0138
xor ecx, ecx
.L_0138:
and eax, ecx
je .L_00B0
dec dword ptr [_PLAYER+12]
mov ecx, dword ptr [_PLAYER+8]
imul ecx, 10
add ecx, dword ptr [_PLAYER+12]
mov dword ptr [ebp+ecx*4-440], -1
.L_00B0:
.L_00AF:
mov ecx, dword ptr [_PLAYER+8]
imul ecx, 60
cmp dword ptr [_PLAYER], ecx
jge .L_00B2
mov ecx, dword ptr [ebp-1008]
mov eax, dword ptr [ebp+ecx*4-1060]
add dword ptr [_PLAYER], eax
.L_00B2:
.L_00B1:
mov eax, dword ptr [_PLAYER+8]
imul eax, 60
cmp dword ptr [_PLAYER], eax
jle .L_00B4
mov eax, dword ptr [ebp-1008]
mov ecx, dword ptr [ebp+eax*4-1060]
sub dword ptr [_PLAYER], ecx
.L_00B4:
.L_00B3:
mov ecx, dword ptr [_PLAYER+12]
imul ecx, 40
cmp dword ptr [_PLAYER+4], ecx
jge .L_00B6
mov ecx, dword ptr [ebp-1008]
mov eax, dword ptr [ebp+ecx*4-1060]
add dword ptr [_PLAYER+4], eax
.L_00B6:
.L_00B5:
mov eax, dword ptr [_PLAYER+12]
imul eax, 40
cmp dword ptr [_PLAYER+4], eax
jle .L_00B8
mov eax, dword ptr [ebp-1008]
mov ecx, dword ptr [ebp+eax*4-1060]
sub dword ptr [_PLAYER+4], ecx
.L_00B8:
.L_00B7:
jmp .L_0087
.L_0088:
push 57
call _fb_Multikey@4
test eax, eax
je .L_00BA
mov dword ptr [ebp-988], 0
mov dword ptr [ebp-984], -1
mov dword ptr [_LEVEL], 1
mov dword ptr [ebp-1016], 1
mov dword ptr [_STAGE], 0
mov dword ptr [ebp-1012], 5
.L_00BA:
.L_00B9:
.L_0087:
cmp dword ptr [ebp-984], 0
je .L_00BC
inc dword ptr [_STAGE]
cmp dword ptr [_STAGE], 3
jle .L_00BE
mov dword ptr [_STAGE], 1
inc dword ptr [_LEVEL]
inc dword ptr [ebp-1016]
cmp dword ptr [_LEVEL], 3
jle .L_00C0
mov dword ptr [_LEVEL], 3
inc dword ptr [ebp-1012]
cmp dword ptr [ebp-1012], 5
jle .L_00C2
mov dword ptr [ebp-1012], 5
.L_00C2:
.L_00C1:
.L_00C0:
.L_00BF:
.L_00BE:
.L_00BD:
call _RESETOBJECTS@0
mov dword ptr [_LOOP1], 0
.L_00C6:
mov dword ptr [ebp-1000], 0
.L_00CA:
cmp dword ptr [_LOOP1], 10
jge .L_00CC
mov eax, dword ptr [_LOOP1]
imul eax, 11
add eax, dword ptr [ebp-1000]
mov dword ptr [ebp+eax*4-928], 0
.L_00CC:
.L_00CB:
cmp dword ptr [ebp-1000], 10
jge .L_00CE
mov eax, dword ptr [_LOOP1]
imul eax, 10
add eax, dword ptr [ebp-1000]
mov dword ptr [ebp+eax*4-440], 0
.L_00CE:
.L_00CD:
.L_00C8:
inc dword ptr [ebp-1000]
.L_00C7:
cmp dword ptr [ebp-1000], 10
jle .L_00CA
.L_00C9:
.L_00C4:
inc dword ptr [_LOOP1]
.L_00C3:
cmp dword ptr [_LOOP1], 10
jle .L_00C6
.L_00C5:
.L_00BC:
.L_00BB:
mov eax, dword ptr [ebp-1028]
imul eax, 60
cmp eax, dword ptr [_PLAYER]
mov eax, -1
je .L_0139
xor eax, eax
.L_0139:
mov ecx, dword ptr [ebp-1024]
imul ecx, 40
cmp ecx, dword ptr [_PLAYER+4]
mov ecx, -1
je .L_013A
xor ecx, ecx
.L_013A:
and eax, ecx
mov ecx, dword ptr [ebp-1036]
cmp ecx, 1
mov ecx, -1
je .L_013B
xor ecx, ecx
.L_013B:
and eax, ecx
je .L_00D0
mov dword ptr [ebp-1036], 2
mov dword ptr [ebp-1032], 0
.L_00D0:
.L_00CF:
fld dword ptr [_PLAYER+16]
mov ecx, dword ptr [ebp-1008]
fild dword ptr [ebp+ecx*4-1060]
fmul qword ptr [_Lt_010F]
fxch st(1)
faddp
fstp dword ptr [_PLAYER+16]
fld dword ptr [_PLAYER+16]
fcomp qword ptr [_Lt_0119]
fnstsw ax
sahf
jbe .L_00D2
fld dword ptr [_Lt_011A]
fadd dword ptr [_PLAYER+16]
fstp dword ptr [_PLAYER+16]
.L_00D2:
.L_00D1:
fld dword ptr [ebp-1020]
mov ecx, dword ptr [ebp-1008]
fild dword ptr [ebp+ecx*4-1060]
fmul qword ptr [_Lt_011B]
fxch st(1)
fsubrp
fstp dword ptr [ebp-1020]
fld dword ptr [ebp-1020]
fld qword ptr [_Lt_00F6]
fcompp
fnstsw ax
sahf
jbe .L_00D4
fld dword ptr [ebp-1020]
fadd qword ptr [_Lt_0119]
fstp dword ptr [ebp-1020]
.L_00D4:
.L_00D3:
mov ecx, dword ptr [ebp-1008]
mov eax, dword ptr [ebp+ecx*4-1060]
add dword ptr [ebp-1032], eax
mov eax, dword ptr [ebp-1036]
test eax, eax
mov eax, -1
je .L_013C
xor eax, eax
.L_013C:
fild dword ptr [ebp-1032]
fld qword ptr [_Lt_011C]
fidiv dword ptr [_LEVEL]
fxch st(1)
fcompp
push eax
fnstsw ax
sahf
pop eax
mov ecx, -1
ja .L_013D
xor ecx, ecx
.L_013D:
and eax, ecx
je .L_00D6
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_0113]
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
fadd qword ptr [_Lt_0104]
fistp dword ptr [ebp-1028]
push dword ptr [_Lt_00E8]
call _fb_Rnd@4
fmul qword ptr [_Lt_00EB]
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
fistp dword ptr [ebp-1024]
mov dword ptr [ebp-1036], 1
mov dword ptr [ebp-1032], 0
.L_00D6:
.L_00D5:
mov ecx, dword ptr [ebp-1036]
cmp ecx, 1
mov ecx, -1
je .L_013E
xor ecx, ecx
.L_013E:
fild dword ptr [ebp-1032]
fld qword ptr [_Lt_011C]
fidiv dword ptr [_LEVEL]
fxch st(1)
fcompp
fnstsw ax
sahf
mov eax, -1
ja .L_013F
xor eax, eax
.L_013F:
and ecx, eax
je .L_00D8
mov dword ptr [ebp-1036], 0
mov dword ptr [ebp-1032], 0
.L_00D8:
.L_00D7:
mov eax, dword ptr [ebp-1036]
cmp eax, 2
mov eax, -1
je .L_0140
xor eax, eax
.L_0140:
mov ecx, dword ptr [_LEVEL]
imul ecx, 500
add ecx, 500
mov esi, dword ptr [ebp-1032]
cmp esi, ecx
mov esi, -1
jg .L_0141
xor esi, esi
.L_0141:
and eax, esi
je .L_00DA
mov dword ptr [ebp-1036], 0
mov dword ptr [ebp-1032], 0
.L_00DA:
.L_00D9:
inc dword ptr [ebp-1004]
push 30
call _fb_Multikey@4
mov esi, dword ptr [ebp-980]
not esi
and eax, esi
je .L_00DC
mov dword ptr [ebp-980], -1
not dword ptr [ebp-992]
.L_00DC:
.L_00DB:
push 30
call _fb_Multikey@4
test eax, eax
jne .L_00DE
mov dword ptr [ebp-980], 0
.L_00DE:
push -1
push -1
call _fb_GfxFlip@8
.L_0052:
push 1
call _fb_Multikey@4
test eax, eax
je .L_0050
.L_0051:
.L_00DF:
push 1
push offset _Lt_0000
push -1
call _fb_Inkey@0
push eax
call _fb_StrCompare@16
test eax, eax
je .L_00E0
jmp .L_00DF
.L_00E0:
push 256
push dword ptr [_GBASE]
call _glDeleteLists@8
push 0
call _fb_End@4
lea eax, [ebp-1132]
push eax
call _fb_ArrayErase@4
.L_0003:
pop esi
pop ebx
mov esp, ebp
pop ebp
ret
.balign 16
_CREATETEXTURE@8:
push ebp
mov ebp, esp
sub esp, 88
push ebx
mov dword ptr [ebp-4], 0
.L_0028:
mov dword ptr [ebp-8], 0
mov dword ptr [ebp-12], 0
mov dword ptr [ebp-16], 0
mov dword ptr [ebp-20], 0
mov dword ptr [ebp-24], 0
mov dword ptr [ebp-28], 0
mov dword ptr [ebp-32], 0
mov dword ptr [ebp-36], 0
mov dword ptr [ebp-40], 0
mov dword ptr [ebp-44], 0
mov eax, dword ptr [ebp+8]
mov dword ptr [ebp-48], eax
mov dword ptr [ebp-4], 0
mov eax, dword ptr [ebp-48]
cmp dword ptr [eax], 7
jne .L_002B
mov eax, dword ptr [ebp-48]
mov ebx, dword ptr [eax+8]
mov dword ptr [ebp-12], ebx
mov ebx, dword ptr [ebp-48]
mov eax, dword ptr [ebx+12]
mov dword ptr [ebp-16], eax
jmp .L_002A
.L_002B:
mov eax, dword ptr [ebp-48]
movzx ebx, word ptr [eax]
sar ebx, 3
mov eax, ebx
and eax, 8191
mov ebx, eax
mov dword ptr [ebp-12], ebx
mov ebx, dword ptr [ebp-48]
movzx eax, word ptr [ebx+2]
mov dword ptr [ebp-16], eax
.L_002A:
mov eax, dword ptr [ebp-12]
cmp eax, 64
mov eax, -1
jl .L_0142
xor eax, eax
.L_0142:
mov ebx, dword ptr [ebp-16]
cmp ebx, 64
mov ebx, -1
jl .L_0143
xor ebx, ebx
.L_0143:
or eax, ebx
je .L_002D
jmp .L_0029
.L_002D:
.L_002C:
mov ebx, dword ptr [ebp-12]
dec ebx
and ebx, dword ptr [ebp-12]
mov eax, dword ptr [ebp-16]
dec eax
and eax, dword ptr [ebp-16]
or ebx, eax
je .L_002F
jmp .L_0029
.L_002F:
.L_002E:
mov dword ptr [ebp-84], 0
mov dword ptr [ebp-80], 0
mov dword ptr [ebp-76], 0
mov dword ptr [ebp-72], 4
mov dword ptr [ebp-68], 1
mov dword ptr [ebp-64], 17
mov dword ptr [ebp-60], 0
mov dword ptr [ebp-56], 0
mov dword ptr [ebp-52], 0
mov eax, dword ptr [ebp-16]
imul eax, dword ptr [ebp-12]
dec eax
push eax
push 0
push 1
push 0
push -1
push 4
lea eax, [ebp-84]
push eax
call _fb_ArrayRedimEx
add esp, 28
mov eax, dword ptr [ebp-84]
lea ebx, [eax]
mov dword ptr [ebp-8], ebx
lea ebx, [ebp-32]
push ebx
push 1
call _glGenTextures@8
push dword ptr [ebp-32]
push 3553
call _glBindTexture@8
mov ebx, dword ptr [ebp-16]
dec ebx
mov dword ptr [ebp-24], ebx
jmp .L_0030
.L_0033:
mov dword ptr [ebp-20], 0
mov ebx, dword ptr [ebp-12]
dec ebx
mov dword ptr [ebp-88], ebx
jmp .L_0035
.L_0038:
fild dword ptr [ebp-24]
sub esp,4
fstp dword ptr [esp]
fild dword ptr [ebp-20]
sub esp,4
fstp dword ptr [esp]
push dword ptr [ebp+8]
call _fb_GfxPoint@12
mov dword ptr [ebp-28], eax
mov eax, dword ptr [ebp-28]
and eax, 255
mov bl, al
movzx eax, bl
shl eax, 16
mov ebx, dword ptr [ebp-28]
sar ebx, 8
and ebx, 255
mov cl, bl
movzx ebx, cl
shl ebx, 8
or eax, ebx
mov ebx, dword ptr [ebp-28]
sar ebx, 16
and ebx, 255
mov cl, bl
movzx ebx, cl
or eax, ebx
or eax, 4278190080
mov ebx, eax
mov dword ptr [ebp-28], ebx
mov ebx, dword ptr [ebp+12]
and ebx, 1
mov eax, dword ptr [ebp-28]
cmp eax, 16711935
mov eax, -1
je .L_0144
xor eax, eax
.L_0144:
and ebx, eax
je .L_003A
mov eax, dword ptr [ebp-8]
mov dword ptr [eax], 0
jmp .L_0039
.L_003A:
mov eax, dword ptr [ebp-28]
or eax, -16777216
mov ebx, eax
mov eax, dword ptr [ebp-8]
mov dword ptr [eax], ebx
.L_0039:
add dword ptr [ebp-8], 4
.L_0036:
inc dword ptr [ebp-20]
.L_0035:
mov ebx, dword ptr [ebp-88]
cmp dword ptr [ebp-20], ebx
jle .L_0038
.L_0037:
.L_0031:
dec dword ptr [ebp-24]
.L_0030:
cmp dword ptr [ebp-24], 0
jge .L_0033
.L_0032:
mov ebx, dword ptr [ebp+12]
and ebx, 9
je .L_003C
mov dword ptr [ebp-36], 6408
jmp .L_003B
.L_003C:
mov dword ptr [ebp-36], 6407
.L_003B:
mov ebx, dword ptr [ebp+12]
and ebx, 4
je .L_003E
mov dword ptr [ebp-44], 9728
jmp .L_003D
.L_003E:
mov dword ptr [ebp-44], 9729
.L_003D:
mov ebx, dword ptr [ebp+12]
and ebx, 2
je .L_0040
mov ebx, dword ptr [ebp-84]
lea eax, [ebx]
push eax
push 5121
push 6408
push dword ptr [ebp-16]
push dword ptr [ebp-12]
push dword ptr [ebp-36]
push 3553
call _gluBuild2DMipmaps@28
mov eax, dword ptr [ebp+12]
and eax, 4
je .L_0042
mov dword ptr [ebp-40], 9985
jmp .L_0041
.L_0042:
mov dword ptr [ebp-40], 9987
.L_0041:
jmp .L_003F
.L_0040:
mov eax, dword ptr [ebp-84]
lea ebx, [eax]
push ebx
push 5121
push 6408
push 0
push dword ptr [ebp-16]
push dword ptr [ebp-12]
push dword ptr [ebp-36]
push 0
push 3553
call _glTexImage2D@36
mov ebx, dword ptr [ebp-44]
mov dword ptr [ebp-40], ebx
.L_003F:
push dword ptr [ebp-40]
push 10241
push 3553
call _glTexParameteri@12
push dword ptr [ebp-44]
push 10240
push 3553
call _glTexParameteri@12
mov ebx, dword ptr [ebp-32]
mov dword ptr [ebp-4], ebx
lea ebx, [ebp-84]
push ebx
call _fb_ArrayErase@4
.L_0029:
mov eax, dword ptr [ebp-4]
pop ebx
mov esp, ebp
pop ebp
ret 8

.section .fbctinf
.ascii "-l\0"
.ascii "opengl32\0"
.ascii "-l\0"
.ascii "glu32\0"
.ascii "-l\0"
.ascii "msvcrt\0"
.ascii "-l\0"
.ascii "advapi32\0"
.ascii "-gfx\0"


.section .data
.balign 4
_Lt_0000:	.ascii	"\0"

.section .bss
.balign 4
	.lcomm	_Lt_0024,36
.balign 4
	.lcomm	_PLAYER,20
.balign 4
	.lcomm	_LOOP1,4

.section .data
.balign 4
_STAGE:
.int 1
.balign 4
_LEVEL:
.int 1

.section .bss
.balign 4
	.lcomm	_ENEMY,180
.balign 4
	.lcomm	_GBASE,4
.balign 4
	.lcomm	_TEXTURE,8

.section .data
.balign 4
_Lt_004A:	.ascii	"/data/Font.bmp\0"
.balign 4
_Lt_004C:	.ascii	"/data/colpatt.bmp\0"
.balign 4
_Lt_0053:	.ascii	"GRID CRAZY\0"
.balign 4
_Lt_0055:	.ascii	"Level:%2i\0"
.balign 4
_Lt_0057:	.ascii	"Stage:%2i\0"
.balign 4
_Lt_005B:	.ascii	"GAME OVER\0"
.balign 4
_Lt_005D:	.ascii	"PRESS SPACE\0"
.balign 4
_Lt_00E8:	.long	0x3F800000
.balign 8
_Lt_00E9:	.quad	0x4018000000000000
.balign 8
_Lt_00EA:	.quad	0x4014000000000000
.balign 8
_Lt_00EB:	.quad	0x4026000000000000
.balign 8
_Lt_00F2:	.quad	0x4030000000000000
.balign 4
_Lt_00F3:	.long	0x3D800000
.balign 8
_Lt_00F4:	.quad	0xBFB0000000000000
.balign 8
_Lt_00F5:	.quad	0x3FB0000000000000
.balign 8
_Lt_00F6:	.quad	0x0000000000000000
.balign 8
_Lt_00F7:	.quad	0x402E000000000000
.balign 4
_Lt_0102:	.long	0x40000000
.balign 4
_Lt_0103:	.long	0x3FC00000
.balign 8
_Lt_0104:	.quad	0x3FF0000000000000
.balign 8
_Lt_0105:	.quad	0xBFF0000000000000
.balign 8
_Lt_0106:	.quad	0x407E000000000000
.balign 8
_Lt_0107:	.quad	0x4084000000000000
.balign 4
_Lt_0108:	.long	0x3F000000
.balign 4
_Lt_0109:	.long	0x00000000
.balign 8
_Lt_010A:	.quad	0x406FE00000000000
.balign 4
_Lt_010B:	.long	0x42200000
.balign 8
_Lt_010C:	.quad	0x4044000000000000
.balign 8
_Lt_010D:	.quad	0x407EA00000000000
.balign 8
_Lt_010E:	.quad	0xC014000000000000
.balign 8
_Lt_010F:	.quad	0x3FE0000000000000
.balign 4
_Lt_0110:	.long	0x3F400000
.balign 8
_Lt_0111:	.quad	0xC01C000000000000
.balign 8
_Lt_0112:	.quad	0x401C000000000000
.balign 8
_Lt_0113:	.quad	0x4024000000000000
.balign 8
_Lt_0114:	.quad	0x3FB999999999999A
.balign 8
_Lt_0115:	.quad	0x4051800000000000
.balign 8
_Lt_0116:	.quad	0x4034000000000000
.balign 8
_Lt_0117:	.quad	0x4000000000000000
.balign 8
_Lt_0118:	.quad	0x408F400000000000
.balign 8
_Lt_0119:	.quad	0x4076800000000000
.balign 4
_Lt_011A:	.long	0xC3B40000
.balign 8
_Lt_011B:	.quad	0x3FD0000000000000
.balign 8
_Lt_011C:	.quad	0x40B7700000000000

.section .ctors
.int _fb_ctor__lesson21
