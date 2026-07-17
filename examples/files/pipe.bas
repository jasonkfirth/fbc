#ifdef __FB_LINUX__
const SHELL_COMMAND = "ls *"
#else
const SHELL_COMMAND = "dir *.*"
#endif
const SEPARATOR_WIDTH = 60

dim inpline as string
dim as integer pipeFile = freefile

if open pipe( SHELL_COMMAND, for input, as #pipeFile ) <> 0 then
	print "Unable to run: "; SHELL_COMMAND
	end 1
end if

print string( SEPARATOR_WIDTH, "-" )

do while( not eof(pipeFile) )
	line input #pipeFile, inpline
	print inpline
loop

print string( SEPARATOR_WIDTH, "-" )

close #pipeFile
