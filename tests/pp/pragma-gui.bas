' TEST_MODE : COMPILE_ONLY_OK

#if __FB_GUI__ <> 0
	#error __FB_GUI__ should default to console mode
#endif

#pragma gui

#if __FB_GUI__ = 0
	#error #pragma gui should enable GUI mode
#endif

#pragma push(gui, 0)

#if __FB_GUI__ <> 0
	#error #pragma push(gui, 0) should enter console mode
#endif

#pragma pop(gui)

#if __FB_GUI__ = 0
	#error #pragma pop(gui) should restore GUI mode
#endif

#pragma gui = 0

#if __FB_GUI__ <> 0
	#error #pragma gui = 0 should restore console mode
#endif
