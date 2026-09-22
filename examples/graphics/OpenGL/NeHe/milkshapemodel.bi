''
'' MilkshapeModel.bi
'' Resource ownership: MODEL owns the parsed arrays after a successful load;
'' Model_Delete releases them, while the loader releases its temporary buffer.
''

/'
    MilkShape model loader

    Model_LoadModelData owns its temporary file buffer only while parsing.
    A successful call transfers mesh, material, triangle, vertex, index, and
    texture-name storage to MODEL; Model_Delete releases those allocations.

    The MS3D structures below are packed on-disk records. Their field widths
    follow the MilkShape 1.3/1.4 format, not the native FreeBASIC ABI.
'/

#ifndef __milkshapemodel_bi__
#define __milkshapemodel_bi__

#include once "GL/gl.bi"

'' Use FIELD = 1 for all structures that are used to read data from disk.
type MS3DHEADER FIELD = 1
	m_ID(9) as ubyte
	m_version as long
end type

'' Vertex information
type MS3DVERTEX FIELD = 1
	m_flags as ubyte
	m_vertex(2) as single
	m_boneID as ubyte
	m_refCount as ubyte
end type

'' Triangle information
type MS3DTRIANGLE FIELD = 1
	m_flags as short
	m_vertexIndices(2) as short
	m_vertexNormals(2, 2) as single
	m_s(2) as single
	m_t(2) as single
	m_smoothingGroup as ubyte
	m_groupIndex as ubyte
end type

'' Material information
type MS3DMATERIAL FIELD = 1
	m_name(31) as ubyte
	m_ambient(3) as single
	m_diffuse(3) as single
	m_specular(3) as single
	m_emissive(3) as single
	m_shininess as single	    '' 0.0f - 128.0f
	m_transparency as single	'' 0.0f - 1.0f
	m_mode as ubyte	            '' 0, 1, 2 is unused now
	m_texture as zstring * 128
	m_alphamap as zstring * 128
end type

'' Keyframe data
type MS3DKEYFRAME FIELD = 1
	m_jointIndex as integer
	m_time as single		      '' millisecs
	m_parameter(2) as single
end type


type MESH
	m_materialIndex as integer
	m_numTriangles as integer
	m_pTriangleIndices as integer ptr
end type

''	Material properties
type MATERIAL
	m_ambient(3) as single
	m_diffuse(3) as single
	m_specular(3) as single
	m_emissive(3) as single
	m_shininess as single
	m_texture as GLuint
	m_pTextureFilename as zstring ptr
end type

''	Triangle structure
type TRIANGLE
	m_vertexNormals(2, 2) as single
	m_s(2) as single
	m_t(2) as single
	m_vertexIndices(2) as integer
end type

''	Vertex structure
type VERTEX
	'' for skeletal animation
	m_boneID as ubyte
	m_location(2) as single
end type

type MODEL
	m_numMeshes as integer
	m_pMeshes as MESH ptr

	''	Materials used
	m_numMaterials as integer
	m_pMaterials as MATERIAL ptr

	''	Triangles used
	m_numTriangles as integer
	m_pTriangles as TRIANGLE ptr

	''	Vertices Used
	m_numVertices as integer
	m_pVertices as VERTEX ptr
end type

'' Declare MilkShape functions
declare function LoadGLTexture (byval filename as zstring ptr) as GLuint
declare function Model_LoadModelData(byval pM as MODEL ptr, byref filename as string) as integer
declare sub Model_Draw(byval pM as MODEL ptr)
declare sub Model_ReloadTextures(byval pM as MODEL ptr)
declare sub Model_Init(byval pModel as MODEL ptr)
declare sub Model_Delete(byval pM as MODEL ptr)

''
'' MilkshapeModel.bas
''



''------------------------------------------------------------------------------
'' Load header files

#include once "bmpload.bi"
#include once "crt.bi"
#include once "GL/glu.bi"
''------------------------------------------------------------------------------

private function Model_HasBytes(byval byteOffset as integer, _
	byval byteCount as integer, byval fileSize as integer) as integer

	if byteOffset < 0 or byteCount < 0 or fileSize < 0 then return false
	if byteOffset > fileSize then return false
	if byteCount > fileSize - byteOffset then return false

	return true
end function


''Load the model data into the private variables.
'' Boundary checks are deliberately explicit because every branch protects a
'' different field in the packed MS3D record stream.
'' FB-LINTER: DISABLE-NEXT-LINE FBL111
function Model_LoadModelData(byval pM as MODEL ptr, byref filename as string) as integer
	dim i as integer, j as integer, c as integer
	dim ffile as integer
	dim fileSize as integer
	dim byteOffset as integer
	dim fileOpen as integer
	dim textureLength as integer
	dim pBuffer as byte ptr
	dim pPtr as byte ptr

	dim pHeader as MS3DHEADER ptr
	dim pVertex as MS3DVERTEX ptr
	dim pTriangle as MS3DTRIANGLE ptr
	dim pMaterial as MS3DMATERIAL ptr
	dim pTriangleIndices as integer ptr

	dim nVertices as integer
	dim nTriangles as integer
	dim nMaterials as integer
	dim nGroups as integer

	dim vertexIndices(0 to 3) as integer
	dim t(0 to 3) as single
	dim materialIndex as byte


	if pM = 0 then return false

	'' The loader starts from a known ownership state so failure cleanup is safe.
	Model_Init(pM)

	ffile = freefile
	open filename for binary as #ffile
	if err <> 0 then return false
	fileOpen = true

	fileSize = lof(ffile)
	if fileSize < sizeof(MS3DHEADER) then goto load_failed

	pBuffer = allocate(fileSize)
	if pBuffer = 0 then goto load_failed

	'' The raw file buffer is validated against each MS3D record size before parsing.
	for i = 0 to fileSize - 1
		'' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
		get #ffile, , pBuffer[i]
		if err <> 0 then goto load_failed
	next
	close #ffile
	fileOpen = false

	pPtr = pBuffer
	byteOffset = 0
	if Model_HasBytes(byteOffset, sizeof(MS3DHEADER), fileSize) = false then goto load_failed
	pHeader = cast(MS3DHEADER ptr, pPtr)
	pPtr += sizeof(MS3DHEADER)
	byteOffset += sizeof(MS3DHEADER)

	if strncmp(varptr(pHeader->m_ID(0)), "MS3D000000", 10) <> 0 then goto load_failed
	if pHeader->m_version < 3 or pHeader->m_version > 4 then goto load_failed

	if Model_HasBytes(byteOffset, sizeof(short), fileSize) = false then goto load_failed
	nVertices = peek(short, pPtr)
	pPtr += sizeof(short)
	byteOffset += sizeof(short)
	if nVertices < 0 then goto load_failed
	if Model_HasBytes(byteOffset, sizeof(MS3DVERTEX) * nVertices, fileSize) = false then goto load_failed

	if nVertices > 0 then
		pM->m_pVertices = callocate(sizeof(VERTEX) * nVertices)
		if pM->m_pVertices = 0 then goto load_failed
	end if
	pM->m_numVertices = nVertices

	for i = 0 to nVertices - 1
		pVertex = cast(MS3DVERTEX ptr, pPtr)
		pM->m_pVertices[i].m_boneID = pVertex->m_boneID
		memcpy(varptr(pM->m_pVertices[i].m_location(0)), varptr(pVertex->m_vertex(0)), sizeof(single) * 3)
		pPtr += sizeof(MS3DVERTEX)
	next
	byteOffset += sizeof(MS3DVERTEX) * nVertices

	if Model_HasBytes(byteOffset, sizeof(short), fileSize) = false then goto load_failed
	nTriangles = peek(short, pPtr)
	pPtr += sizeof(short)
	byteOffset += sizeof(short)
	if nTriangles < 0 then goto load_failed
	if Model_HasBytes(byteOffset, sizeof(MS3DTRIANGLE) * nTriangles, fileSize) = false then goto load_failed

	if nTriangles > 0 then
		pM->m_pTriangles = callocate(sizeof(TRIANGLE) * nTriangles)
		if pM->m_pTriangles = 0 then goto load_failed
	end if
	pM->m_numTriangles = nTriangles

	for i = 0 to nTriangles - 1
		pTriangle = cast(MS3DTRIANGLE ptr, pPtr)
		vertexIndices(0) = pTriangle->m_vertexIndices(0)
		vertexIndices(1) = pTriangle->m_vertexIndices(1)
		vertexIndices(2) = pTriangle->m_vertexIndices(2)
		if vertexIndices(0) < 0 or vertexIndices(0) >= nVertices then goto load_failed
		if vertexIndices(1) < 0 or vertexIndices(1) >= nVertices then goto load_failed
		if vertexIndices(2) < 0 or vertexIndices(2) >= nVertices then goto load_failed
		t(0) = 1.0 - pTriangle->m_t(0)
		t(1) = 1.0 - pTriangle->m_t(1)
		t(2) = 1.0 - pTriangle->m_t(2)
		memcpy(varptr(pM->m_pTriangles[i].m_vertexNormals(0, 0)), varptr(pTriangle->m_vertexNormals(0, 0)), sizeof(single) * 3 * 3)
		memcpy(varptr(pM->m_pTriangles[i].m_s(0)), varptr(pTriangle->m_s(0)), sizeof(single) * 3)
		memcpy(varptr(pM->m_pTriangles[i].m_t(0)), varptr(t(0)), sizeof(single) * 3)
		memcpy(varptr(pM->m_pTriangles[i].m_vertexIndices(0)), varptr(vertexIndices(0)), sizeof(integer) * 3)
		pPtr += sizeof(MS3DTRIANGLE)
	next
	byteOffset += sizeof(MS3DTRIANGLE) * nTriangles

	if Model_HasBytes(byteOffset, sizeof(short), fileSize) = false then goto load_failed
	nGroups = peek(short, pPtr)
	pPtr += sizeof(short)
	byteOffset += sizeof(short)
	if nGroups < 0 then goto load_failed

	if nGroups > 0 then
		pM->m_pMeshes = callocate(sizeof(MESH) * nGroups)
		if pM->m_pMeshes = 0 then goto load_failed
	end if
	pM->m_numMeshes = nGroups

	for i = 0 to nGroups - 1
		if Model_HasBytes(byteOffset, sizeof(byte) + 32 + sizeof(short), fileSize) = false then goto load_failed
		pPtr += sizeof(byte) + 32
		byteOffset += sizeof(byte) + 32
		nTriangles = peek(short, pPtr)
		pPtr += sizeof(short)
		byteOffset += sizeof(short)
		if nTriangles < 0 then goto load_failed
		if Model_HasBytes(byteOffset, sizeof(short) * nTriangles + sizeof(byte), fileSize) = false then goto load_failed

		if nTriangles > 0 then
			pTriangleIndices = callocate(sizeof(integer) * nTriangles)
			if pTriangleIndices = 0 then goto load_failed
		end if

		for j = 0 to nTriangles - 1
			pTriangleIndices[j] = peek(short, pPtr)
			if pTriangleIndices[j] < 0 or pTriangleIndices[j] >= pM->m_numTriangles then goto load_failed
			pPtr += sizeof(short)
		next
		byteOffset += sizeof(short) * nTriangles

		materialIndex = peek(byte, pPtr)
		pPtr += sizeof(byte)
		byteOffset += sizeof(byte)

		pM->m_pMeshes[i].m_materialIndex = materialIndex
		pM->m_pMeshes[i].m_numTriangles = nTriangles
		pM->m_pMeshes[i].m_pTriangleIndices = pTriangleIndices
		pTriangleIndices = 0
	next

	if Model_HasBytes(byteOffset, sizeof(short), fileSize) = false then goto load_failed
	nMaterials = peek(short, pPtr)
	pPtr += sizeof(short)
	byteOffset += sizeof(short)
	if nMaterials < 0 then goto load_failed
	if Model_HasBytes(byteOffset, sizeof(MS3DMATERIAL) * nMaterials, fileSize) = false then goto load_failed

	if nMaterials > 0 then
		pM->m_pMaterials = callocate(sizeof(MATERIAL) * nMaterials)
		if pM->m_pMaterials = 0 then goto load_failed
	end if
	pM->m_numMaterials = nMaterials

	for i = 0 to nMaterials - 1
		pMaterial = cast(MS3DMATERIAL ptr, pPtr)
		memcpy(varptr(pM->m_pMaterials[i].m_ambient(0)), varptr(pMaterial->m_ambient(0)), sizeof(single) * 4)
		memcpy(varptr(pM->m_pMaterials[i].m_diffuse(0)), varptr(pMaterial->m_diffuse(0)), sizeof(single) * 4)
		memcpy(varptr(pM->m_pMaterials[i].m_specular(0)), varptr(pMaterial->m_specular(0)), sizeof(single) * 4)
		memcpy(varptr(pM->m_pMaterials[i].m_emissive(0)), varptr(pMaterial->m_emissive(0)), sizeof(single) * 4)
		pM->m_pMaterials[i].m_shininess = pMaterial->m_shininess

		textureLength = 0
		while textureLength < sizeof(pMaterial->m_texture)
			if pMaterial->m_texture[textureLength] = 0 then exit while
			textureLength += 1
		wend
		pM->m_pMaterials[i].m_pTextureFilename = callocate(textureLength + 1)
		if pM->m_pMaterials[i].m_pTextureFilename = 0 then goto load_failed
		for c = 0 to textureLength - 1
			pM->m_pMaterials[i].m_pTextureFilename[c] = pMaterial->m_texture[c]
		next

		pPtr += sizeof(MS3DMATERIAL)
	next
	byteOffset += sizeof(MS3DMATERIAL) * nMaterials

	for i = 0 to pM->m_numMeshes - 1
		if pM->m_pMeshes[i].m_materialIndex < -1 or _
		   pM->m_pMeshes[i].m_materialIndex >= nMaterials then goto load_failed
	next

	deallocate(pBuffer)
	pBuffer = 0
	Model_ReloadTextures(pM)
	return true

load_failed:
	if fileOpen then close #ffile
	if pBuffer <> 0 then
		deallocate(pBuffer)
		pBuffer = 0
	end if
	Model_Delete(pM)
	return false
end function

''------------------------------------------------------------------------------
'' Set everything to NULL - possibly not needed in FreeBASIC?
sub Model_Init(byval pM as MODEL ptr)
	pM->m_numMeshes = 0
	pM->m_pMeshes = 0
	pM->m_numMaterials = 0
	pM->m_pMaterials = 0
	pM->m_numTriangles = 0
	pM->m_pTriangles = 0
	pM->m_numVertices = 0
	pM->m_pVertices = 0
end sub

''------------------------------------------------------------------------------
'' Free the memory of our model
sub Model_Delete(byval pM as MODEL ptr)
	dim i as integer

	if pM = 0 then exit sub

	if pM->m_pMeshes <> 0 then
		for i = 0 to pM->m_numMeshes - 1
			if pM->m_pMeshes[i].m_pTriangleIndices <> 0 then
				deallocate(pM->m_pMeshes[i].m_pTriangleIndices)
				pM->m_pMeshes[i].m_pTriangleIndices = 0
			end if
		next
		deallocate(pM->m_pMeshes)
		pM->m_pMeshes = 0
	end if
	pM->m_numMeshes = 0

	if pM->m_pMaterials <> 0 then
		for i = 0 to pM->m_numMaterials - 1
			if pM->m_pMaterials[i].m_pTextureFilename <> 0 then
				deallocate(pM->m_pMaterials[i].m_pTextureFilename)
				pM->m_pMaterials[i].m_pTextureFilename = 0
			end if
		next
		deallocate(pM->m_pMaterials)
		pM->m_pMaterials = 0
	end if
	pM->m_numMaterials = 0

	pM->m_numTriangles = 0
	if pM->m_pTriangles <> 0 then
		deallocate(pM->m_pTriangles)
		pM->m_pTriangles = 0
	end if

	pM->m_numVertices = 0
	if pM->m_pVertices <> 0 then
		deallocate(pM->m_pVertices)
		pM->m_pVertices = 0
	end if
end sub

''------------------------------------------------------------------------------
'' Draw the model
sub Model_Draw(byval pM as MODEL ptr)
	dim  i as integer, j as integer, k as integer
	dim texEnabled as GLboolean
	dim materialIndex as integer
	dim triangleIndex as integer
	dim pTri as TRIANGLE ptr
	dim index as integer

	texEnabled = glIsEnabled (GL_TEXTURE_2D)

	'' Draw by group
	for i = 0 to pM->m_numMeshes - 1
		materialIndex = pM->m_pMeshes[i].m_materialIndex
		if materialIndex >= 0 then
			glMaterialfv (GL_FRONT, GL_AMBIENT, varptr(pM->m_pMaterials[materialIndex].m_ambient(0)))
			glMaterialfv (GL_FRONT, GL_DIFFUSE, varptr(pM->m_pMaterials[materialIndex].m_diffuse(0)))
			glMaterialfv (GL_FRONT, GL_SPECULAR, varptr(pM->m_pMaterials[materialIndex].m_specular(0)))
			glMaterialfv (GL_FRONT, GL_EMISSION, varptr(pM->m_pMaterials[materialIndex].m_emissive(0)))
			glMaterialf (GL_FRONT, GL_SHININESS, pM->m_pMaterials[materialIndex].m_shininess)
			if pM->m_pMaterials[materialIndex].m_texture > 0 then
				glBindTexture (GL_TEXTURE_2D, pM->m_pMaterials[materialIndex].m_texture)
				glEnable (GL_TEXTURE_2D)
			else
				glDisable (GL_TEXTURE_2D)
			end if
		else
			'' Material properties?
			glDisable (GL_TEXTURE_2D)
		end if

		glBegin (GL_TRIANGLES)

			for j = 0 to pM->m_pMeshes[i].m_numTriangles - 1
				triangleIndex = pM->m_pMeshes[i].m_pTriangleIndices[j]
				pTri = varptr(pM->m_pTriangles[triangleIndex])
				for k = 0 to 2
					index = pTri->m_vertexIndices(k)
					glNormal3fv (varptr(pTri->m_vertexNormals(k, 0)))
					glTexCoord2f (pTri->m_s(k), pTri->m_t(k))
					glVertex3fv (varptr(pM->m_pVertices[index].m_location(0)))
				next
			next
		glEnd ()
	next

	if texEnabled <> 0 then
		glEnable (GL_TEXTURE_2D)
	else
		glDisable (GL_TEXTURE_2D)
	end if
end sub

''------------------------------------------------------------------------------
'' Reload the textures.
'' Note that the filenames of materials stored in the Model
'' are in Null terminated strings and will need to be converted later. Relative
'' paths must also be correct.
sub Model_ReloadTextures(byval pM as MODEL ptr)
	dim  i as integer

	if pM = 0 then exit sub
	if pM->m_pMaterials = 0 then exit sub

	for i = 0 to pM->m_numMaterials - 1
		if pM->m_pMaterials[i].m_pTextureFilename <> 0 andalso _
		   strlen(*pM->m_pMaterials[i].m_pTextureFilename) > 0 then
			pM->m_pMaterials[i].m_texture = LoadGLTexture(pM->m_pMaterials[i].m_pTextureFilename)
		else
			pM->m_pMaterials[i].m_texture = 0
		end if
	next
end sub


''------------------------------------------------------------------------------
function LoadGLTexture (byval filename as zstring ptr) as GLuint         '' Load Bitmaps And Convert To Textures
	dim pImage as BITMAP_RGBImageRec ptr   	     '' Create Storage Space For The Texture
	dim texture as GLuint           			 '' Texture ID
	dim fbfilename as string

	texture = 0
	if filename = 0 then return texture

	'' Convert filename in model from a zstring to a FB string.
	'' Notice that NeHe created the model with the "data" directory hard coded in the Milkshape model
	'' The FreeBASIC file runtime accepts this portable separator on its supported hosts.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-IO-012
	fbfilename = exepath + "/" + *filename

	pImage = LoadBMP (fbfilename)

	if pImage <> NULL  then
		if  pImage->buffer <> NULL then
			glGenTextures (1, varptr(texture))          '' Create The Texture

			 '' Typical Texture Generation Using Data From The Bitmap
			glBindTexture (GL_TEXTURE_2D, texture)
			glTexImage2D (GL_TEXTURE_2D, 0, 3, pImage->sizeX, pImage->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, pImage->buffer)
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)

			deallocate(pImage->buffer)                  '' DEALLOCATE The Texture Image Memory
			pImage->buffer = 0
		end if
		deallocate(pImage)                          '' DEALLOCATE The Image Structure
		pImage = 0
	end if

	return texture
end function


#endif
