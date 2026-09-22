#cmdline "-strip -R -z optabstract"

#Include "inc\EXCEL.bi"
using Excel
'' This module-level optimizer example reads the two COM output pointers below.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
dim shared as ILine ptr pILine
dim shared as Application Ptr RHS

if pILine then
	pILine->Get_Application(@RHS)
end if
