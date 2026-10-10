'' Project: FreeBASIC SDL3 binding tests
'' File: inline-smoke.bi
'' Purpose: Compare the translated header helpers with the original C code.
'' Responsibilities: Check bits, byte order, rectangles, and size arithmetic.
'' This file intentionally does NOT contain: SDL initialization or hardware tests.

#pragma once

extern "C"
	declare function SDL3_test_swap16(byval value as Uint16) as Uint16
	declare function SDL3_test_swap32(byval value as Uint32) as Uint32
	declare function SDL3_test_swap64(byval value as Uint64) as Uint64
	declare function SDL3_test_swap_float_bits(byval bits as Uint32) as Uint32
	declare function SDL3_test_bit_index(byval value as Uint32) as long
	declare function SDL3_test_one_bit(byval value as Uint32) as boolean
	declare sub SDL3_test_rect_to_float(byval r as const SDL_Rect ptr, byval f as SDL_FRect ptr)
	declare function SDL3_test_point_in_rect(byval p as const SDL_Point ptr, byval r as const SDL_Rect ptr) as boolean
	declare function SDL3_test_rect_empty(byval r as const SDL_Rect ptr) as boolean
	declare function SDL3_test_rect_equal(byval a as const SDL_Rect ptr, byval b as const SDL_Rect ptr) as boolean
	declare function SDL3_test_point_in_float(byval p as const SDL_FPoint ptr, byval r as const SDL_FRect ptr) as boolean
	declare function SDL3_test_float_empty(byval r as const SDL_FRect ptr) as boolean
	declare function SDL3_test_float_equal_epsilon(byval a as const SDL_FRect ptr, byval b as const SDL_FRect ptr, byval epsilon as single) as boolean
	declare function SDL3_test_float_equal(byval a as const SDL_FRect ptr, byval b as const SDL_FRect ptr) as boolean
	declare function SDL3_test_size_mul(byval a as uinteger, byval b as uinteger, byval result as uinteger ptr) as boolean
	declare function SDL3_test_size_add(byval a as uinteger, byval b as uinteger, byval result as uinteger ptr) as boolean
end extern

scope
	dim values(0 to 7) as Uint32 = {0u, 1u, 2u, 3u, &h12345678u, &h80000000u, &hFFFFFFFEu, &hFFFFFFFFu}
	union FloatBits
		value as single
		bits as Uint32
	end union
	for i as long = 0 to 7
		dim value as Uint32 = values(i)
		check(SDL_Swap16(cushort(value)) = SDL3_test_swap16(cushort(value)), "C inline swap16")
		check(SDL_Swap32(value) = SDL3_test_swap32(value), "C inline swap32")
		dim wide as Uint64 = (culngint(value) shl 32) or &hFEDCBA98ull
		check(SDL_Swap64(wide) = SDL3_test_swap64(wide), "C inline swap64")
		check(SDL_MostSignificantBitIndex32(value) = SDL3_test_bit_index(value), "C inline bit index")
		check(SDL_HasExactlyOneBitSet32(value) = SDL3_test_one_bit(value), "C inline bit count")
		dim input_bits as FloatBits, output_bits as FloatBits
		input_bits.bits = value
		output_bits.value = SDL_SwapFloat(input_bits.value)
		check(output_bits.bits = SDL3_test_swap_float_bits(value), "C inline float swap bits")
	next
	for bit_index as long = 0 to 31
		dim value as Uint32 = culng(1ull shl bit_index)
		check(SDL_MostSignificantBitIndex32(value) = SDL3_test_bit_index(value), "C inline each bit index")
		check(SDL_HasExactlyOneBitSet32(value) = SDL3_test_one_bit(value), "C inline each single bit")
	next
end scope

scope
	dim rectangles(0 to 3) as SDL_Rect = {type<SDL_Rect>(10, 20, 30, 40), _
	                                  type<SDL_Rect>(10, 20, 30, 40), _
	                                  type<SDL_Rect>(0, 0, 0, 0), _
	                                  type<SDL_Rect>(-5, -7, -1, 9)}
	dim floats(0 to 3) as SDL_FRect
	dim points(0 to 3) as SDL_Point = {type<SDL_Point>(10, 20), type<SDL_Point>(40, 60), _
	                                type<SDL_Point>(39, 59), type<SDL_Point>(-5, -7)}
	dim float_points(0 to 3) as SDL_FPoint
	for i as long = 0 to 3
		dim converted as SDL_FRect
		SDL_RectToFRect(@rectangles(i), @floats(i))
		SDL3_test_rect_to_float(@rectangles(i), @converted)
		check(floats(i).x = converted.x andalso floats(i).y = converted.y andalso _
		      floats(i).w = converted.w andalso floats(i).h = converted.h, "C inline rectangle conversion")
		float_points(i).x = points(i).x
		float_points(i).y = points(i).y
	next
	'' Nulls, empty rectangles, identical pointers, and boundary points exercise
	'' the branches that ordinary graphical examples do not reliably reach.
	for i as long = -1 to 3
		dim r as const SDL_Rect ptr = 0, f as const SDL_FRect ptr = 0
		if i >= 0 then
			r = @rectangles(i)
			f = @floats(i)
		end if
		check(SDL_RectEmpty(r) = SDL3_test_rect_empty(r), "C inline empty rectangle")
		check(SDL_RectEmptyFloat(f) = SDL3_test_float_empty(f), "C inline empty float rectangle")
		for j as long = -1 to 3
			dim other as const SDL_Rect ptr = 0, other_float as const SDL_FRect ptr = 0
			dim p as const SDL_Point ptr = 0, fp as const SDL_FPoint ptr = 0
			if j >= 0 then
				other = @rectangles(j)
				other_float = @floats(j)
				p = @points(j)
				fp = @float_points(j)
			end if
			check(SDL_RectsEqual(r, other) = SDL3_test_rect_equal(r, other), "C inline rectangle equality")
			check(SDL_PointInRect(p, r) = SDL3_test_point_in_rect(p, r), "C inline rectangle containment")
			check(SDL_PointInRectFloat(fp, f) = SDL3_test_point_in_float(fp, f), "C inline float containment")
			check(SDL_RectsEqualFloat(f, other_float) = SDL3_test_float_equal(f, other_float), "C inline float equality")
			for epsilon_index as long = -1 to 1
				dim epsilon as single = epsilon_index * SDL_FLT_EPSILON
				check(SDL_RectsEqualEpsilon(f, other_float, epsilon) = _
				      SDL3_test_float_equal_epsilon(f, other_float, epsilon), "C inline epsilon equality")
			next
		next
	next
end scope

scope
	dim sizes(0 to 4) as uinteger = {0, 1, 2, SDL_SIZE_MAX \ 2, SDL_SIZE_MAX}
	for i as long = 0 to 4
		for j as long = 0 to 4
			dim native_result as uinteger, basic_result as uinteger
			dim native_ok as boolean = SDL3_test_size_mul(sizes(i), sizes(j), @native_result)
			dim basic_ok as boolean = SDL_size_mul_check_overflow(sizes(i), sizes(j), @basic_result)
			check(basic_ok = native_ok, "C inline multiplication overflow")
			if native_ok then check(basic_result = native_result, "C inline multiplication result")
			native_ok = SDL3_test_size_add(sizes(i), sizes(j), @native_result)
			basic_ok = SDL_size_add_check_overflow(sizes(i), sizes(j), @basic_result)
			check(basic_ok = native_ok, "C inline addition overflow")
			if native_ok then check(basic_result = native_result, "C inline addition result")
		next
	next
	'' SDL specifies the output only on success. Its compiler-builtin path may
	'' write a wrapped value on failure while the portable C path leaves it alone.
end scope

scope
	'' Unordered comparisons must follow C's behavior instead of treating NaN
	'' as an ordinary less-than/equal value. The same-pointer shortcut remains
	'' true even when a rectangle contains NaN coordinates.
	union FloatBits
		value as single
		bits as Uint32
	end union
	dim nan_value as FloatBits
	nan_value.bits = &h7FC00000u
	dim a as SDL_FRect = (0, 0, 10, 20), b as SDL_FRect
	a.x = nan_value.value
	b = a
	check(SDL_RectsEqualEpsilon(@a, @a, 0) = SDL3_test_float_equal_epsilon(@a, @a, 0), "C inline NaN same-pointer equality")
	check(SDL_RectsEqualEpsilon(@a, @b, 0) = SDL3_test_float_equal_epsilon(@a, @b, 0), "C inline NaN rectangle equality")
	a.w = nan_value.value
	check(SDL_RectEmptyFloat(@a) = SDL3_test_float_empty(@a), "C inline NaN rectangle emptiness")
end scope

SDL_CPUPauseInstruction()

'' end of inline-smoke.bi
