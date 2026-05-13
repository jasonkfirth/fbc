' TEST_MODE : COMPILE_ONLY_OK

dim buttons as integer
dim lx as single
dim status as long

status = getxpad(0)
status = getxpad(0, buttons, lx)
