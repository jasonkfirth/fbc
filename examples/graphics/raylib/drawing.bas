'' Project: FreeBASIC raylib 5.5 examples
'' File: drawing.bas
'' Purpose: Draw a small scene and check a rendered pixel.
'' Responsibilities: Exercise raylib's texture, model, and font structures.
'' This file intentionally does not contain asset loading or audio.

#ifdef __FB_JS__
	#include once "raylib.bi"
#else
	#include once "raylib55.bi"
#endif

InitWindow(320, 240, "FreeBASIC raylib drawing")
if IsWindowReady() = 0 then end 1

dim as RenderTexture2D target = LoadRenderTexture(32, 32)
if IsRenderTextureValid(target) = 0 then
	CloseWindow()
	end 1
end if

BeginTextureMode(target)
ClearBackground(RED)
EndTextureMode()
dim as Image pixels = LoadImageFromTexture(target.texture)
dim as Color pixel
if IsImageValid(pixels) then
	pixel = GetImageColor(pixels, 16, 16)
	UnloadImage(pixels)
end if
UnloadRenderTexture(target)

dim as Mesh cube = GenMeshCube(1, 1, 1)
dim as Font default_font = GetFontDefault()
dim as integer status
if pixel.r <> 230 orelse pixel.g <> 41 orelse pixel.b <> 55 orelse _
   cube.vertexCount <= 0 orelse IsFontValid(default_font) = 0 then
	status = 1
else
	dim as Camera3D view_camera = type<Camera3D>( _
		type<Vector3>(3, 3, 3), type<Vector3>(0, 0, 0), _
		type<Vector3>(0, 1, 0), 45, CAMERA_PERSPECTIVE)
	'' WindowShouldClose yields to the browser through Asyncify on the web.
	'' Bound the run because that backend does not report a close event.
	for frame_number as long = 1 to 60
		if WindowShouldClose() then exit for
		BeginDrawing()
		ClearBackground(RAYWHITE)
		BeginMode3D(view_camera)
		DrawCube(type<Vector3>(0, 0, 0), 1, 1, 1, BLUE)
		EndMode3D()
		DrawTextEx(default_font, "raylib 5.5", type<Vector2>(12, 12), 20, 1, BLACK)
		EndDrawing()
	next
end if
UnloadMesh(cube)
CloseWindow()
if status = 0 then print "raylib drawing passed"
end status

'' end of drawing.bas
