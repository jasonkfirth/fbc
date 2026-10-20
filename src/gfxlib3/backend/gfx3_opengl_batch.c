/*
 * FreeBASIC gfxlib3: gfx3_opengl_batch.c
 * Purpose: Build ordered OpenGL primitive, glyph and blit batches.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_opengl_private.h"

#if (defined(HOST_WIN32) || defined(HOST_LINUX)) && !defined(DISABLE_OPENGL)

static int opengl_blit_batch_winner_ensure(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_OPENGL_SURFACE *destination, int reset_generation);

/* ------------------------------------------------------------------------- */
/* Point and glyph batches                                                  */
/* ------------------------------------------------------------------------- */

uint32_t fb_gfx3_gl_points_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_POINTS_COMMAND *first;
	uint32_t count = 1;
	size_t points_size;
	size_t expected_size;
	uint32_t point_index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_POINTS) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_POINTS_COMMAND, point)))
		return 1;
	first = (const FB_GFX3_POINTS_COMMAND *)commands[0]->payload;
	if ((first->count == 0u) ||
	    (fb_gfx3_size_multiply(first->count, sizeof(first->point[0]),
	     &points_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point), points_size,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(commands[0])))
		return 1;
	for (point_index = 0; point_index < first->count; point_index++) {
		if ((first->point[point_index].flags != 0u) ||
		    (first->point[point_index].color != first->point[0].color))
			return 1;
	}
	if (available > FB_GFX3_OPENGL_POINTS_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_POINTS_BATCH_LIMIT;
	while (count < available) {
		const FB_GFX3_POINTS_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_POINTS) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) <
		     offsetof(FB_GFX3_POINTS_COMMAND, point)))
			break;
		candidate = (const FB_GFX3_POINTS_COMMAND *)commands[count]->payload;
		if ((candidate->count == 0u) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0) ||
		    (fb_gfx3_size_multiply(candidate->count,
		     sizeof(candidate->point[0]), &points_size) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
		     points_size, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(commands[count])))
			break;
		for (point_index = 0; point_index < candidate->count; point_index++) {
			if ((candidate->point[point_index].flags != 0u) ||
			    (candidate->point[point_index].color !=
			     candidate->point[0].color))
				break;
		}
		if (point_index != candidate->count)
			break;
		count++;
	}
	return count;
}

int fb_gfx3_gl_points_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_OPENGL_POINTS_BATCH_ITEM *items = NULL;
	const FB_GFX3_POINTS_COMMAND *first;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_RECT clip;
	size_t total_points = 0;
	size_t item_count;
	size_t item_bytes;
	uint32_t index;
	uint32_t item_index = 0;
	uint32_t generation;
	uint32_t groups_x;
	uint32_t groups_y;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_OPENGL_POINTS_BATCH_LIMIT) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_POINTS_COMMAND, point)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_POINTS_COMMAND *)commands[0]->payload;
	result = fb_gfx3_gl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	if (!fb_gfx3_gl_clip_rect(destination, &first->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	for (index = 0; index < count; index++) {
		const FB_GFX3_POINTS_COMMAND *payload;
		size_t points_size;
		size_t expected_size;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_POINTS) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) <
		     offsetof(FB_GFX3_POINTS_COMMAND, point))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_POINTS_COMMAND *)commands[index]->payload;
		if ((payload->count == 0u) ||
		    (memcmp(&payload->clip, &first->clip, sizeof(payload->clip)) != 0) ||
		    (fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
		     &points_size) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
		     points_size, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(commands[index])) ||
		    (fb_gfx3_size_add(total_points, payload->count,
		     &total_points) != FB_GFX3_OK)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
	}
	if ((total_points > UINT32_MAX) ||
	    (fb_gfx3_size_add(total_points, count, &item_count) != FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(item_count, sizeof(*items),
	     &item_bytes) != FB_GFX3_OK) ||
	    (item_bytes > state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	items = (FB_GFX3_OPENGL_POINTS_BATCH_ITEM *)malloc(item_bytes);
	if (items == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (index = 0; index < count; index++) {
		const FB_GFX3_POINTS_COMMAND *payload =
			(const FB_GFX3_POINTS_COMMAND *)commands[index]->payload;
		uint32_t point_index;

		for (point_index = 0; point_index < payload->count; point_index++) {
			const FB_GFX3_POINT *point = &payload->point[point_index];

			if ((point->flags != 0u) ||
			    (point->color != payload->point[0].color)) {
				result = FB_GFX3_INVALID;
				goto done;
			}
			items[item_index].x = point->x;
			items[item_index].y = point->y;
			items[item_index].command_index = index;
			items[item_index].reserved = 0;
			item_index++;
			if ((point->x >= clip.x1) && (point->x <= clip.x2) &&
			    (point->y >= clip.y1) && (point->y <= clip.y2)) {
				if (point->x < resolve_x1)
					resolve_x1 = point->x;
				if (point->y < resolve_y1)
					resolve_y1 = point->y;
				if (point->x > resolve_x2)
					resolve_x2 = point->x;
				if (point->y > resolve_y2)
					resolve_y2 = point->y;
			}
		}
		items[total_points + index].x = 0;
		items[total_points + index].y = 0;
		items[total_points + index].command_index = 0;
		items[total_points + index].reserved = payload->point[0].color;
	}
	if ((item_index != total_points) || (resolve_x1 > resolve_x2) ||
	    (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	groups_x = ((uint32_t)total_points + FB_GFX3_OPENGL_POINT_GROUP_SIZE - 1u) /
		FB_GFX3_OPENGL_POINT_GROUP_SIZE;
	if (groups_x > state->maximum_compute_groups_x) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)item_bytes,
		items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	if (state->blit_batch_generation >= (UINT32_MAX >> 13))
		result = opengl_blit_batch_winner_ensure(state, destination, TRUE);
	else
		result = opengl_blit_batch_winner_ensure(state, destination, FALSE);
	if (result != FB_GFX3_OK)
		goto done;
	generation = ++state->blit_batch_generation;
	state->gl.use_program(state->points_batch_program);
	state->gl.bind_image_texture(0, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->points_batch_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->points_batch_count_location,
		(uint32_t)total_points);
	state->gl.uniform_1ui(state->points_batch_key_location,
		(generation << 13) | 1u);
	state->gl.dispatch_compute(groups_x, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "point batch selection dispatch");
	if (result != FB_GFX3_OK)
		goto done;
	groups_x = ((uint32_t)(resolve_x2 - resolve_x1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = ((uint32_t)(resolve_y2 - resolve_y1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	state->gl.use_program(state->points_batch_resolve_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->points_batch_resolve_rect_location,
		resolve_x1, resolve_y1, resolve_x2 - resolve_x1 + 1,
		resolve_y2 - resolve_y1 + 1);
	state->gl.uniform_1ui(state->points_batch_resolve_count_location,
		(uint32_t)total_points);
	state->gl.uniform_1ui(state->points_batch_resolve_generation_location,
		generation);
	state->gl.uniform_1ui(state->points_batch_resolve_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "point batch resolve dispatch");

done:
	free(items);
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

uint32_t fb_gfx3_gl_glyph_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_GLYPHS_COMMAND *first;
	size_t glyph_bytes;
	size_t expected_size;
	uint32_t total;
	uint32_t count = 1u;

	if ((commands == NULL) || (available == 0u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_GLYPHS) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
		return 1u;
	first = (const FB_GFX3_GLYPHS_COMMAND *)commands[0]->payload;
	if ((first->count == 0u) ||
	    (fb_gfx3_size_multiply(first->count, sizeof(first->glyph[0]),
	     &glyph_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph), glyph_bytes,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(commands[0])))
		return 1u;
	total = first->count;
	while ((count < available) &&
	       (total < FB_GFX3_OPENGL_GLYPH_BATCH_LIMIT)) {
		const FB_GFX3_GLYPHS_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_GLYPHS) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) <
		     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
			break;
		candidate = (const FB_GFX3_GLYPHS_COMMAND *)commands[count]->payload;
		if ((candidate->count == 0u) ||
		    (candidate->count > FB_GFX3_OPENGL_GLYPH_BATCH_LIMIT - total) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0) ||
		    (fb_gfx3_size_multiply(candidate->count,
		     sizeof(candidate->glyph[0]), &glyph_bytes) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph),
		     glyph_bytes, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(commands[count])))
			break;
		total += candidate->count;
		count++;
	}
	return count;
}

int fb_gfx3_gl_glyph_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t command_count)
{
	const FB_GFX3_GLYPHS_COMMAND *first;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_GLYPH *glyphs = NULL;
	uint32_t *tile_counts = NULL;
	uint32_t *tile_ranges = NULL;
	uint32_t *tile_cursors = NULL;
	uint32_t *tile_indices = NULL;
	FB_GFX3_RECT clip;
	size_t glyph_bytes;
	size_t expected_size;
	uint32_t total = 0u;
	uint32_t command_index;
	uint32_t glyph_index = 0u;
	uint32_t first_tile_x;
	uint32_t first_tile_y;
	uint32_t last_tile_x;
	uint32_t last_tile_y;
	uint32_t tiles_x;
	uint32_t tiles_y;
	uint32_t tile_count;
	uint32_t index_count = 0u;
	uint32_t index;
	uint32_t groups_x;
	uint32_t groups_y;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	int result;

	if ((state == NULL) || (commands == NULL) || (command_count == 0u) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_GLYPHS_COMMAND *)commands[0]->payload;
	result = fb_gfx3_gl_surface_retain(state, commands[0]->target,
		commands[command_count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	if (!fb_gfx3_gl_clip_rect(destination, &first->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	for (command_index = 0u; command_index < command_count; ++command_index) {
		const FB_GFX3_GLYPHS_COMMAND *payload;

		if ((commands[command_index] == NULL) ||
		    (commands[command_index]->type != FB_GFX3_COMMAND_GLYPHS) ||
		    (commands[command_index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[command_index]) <
		     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_GLYPHS_COMMAND *)
			commands[command_index]->payload;
		if ((payload->count == 0u) ||
		    (payload->count > FB_GFX3_OPENGL_GLYPH_BATCH_LIMIT - total) ||
		    (memcmp(&payload->clip, &first->clip, sizeof(payload->clip)) != 0) ||
		    (fb_gfx3_size_multiply(payload->count, sizeof(payload->glyph[0]),
		     &glyph_bytes) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph),
		     glyph_bytes, &expected_size) != FB_GFX3_OK) ||
		    (expected_size !=
		     fb_gfx3_command_payload_size(commands[command_index]))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		total += payload->count;
	}
	if ((total == 0u) || (total > FB_GFX3_OPENGL_GLYPH_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(total, sizeof(glyphs[0]), &glyph_bytes) !=
	     FB_GFX3_OK) || (glyph_bytes > state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	glyphs = (FB_GFX3_GLYPH *)malloc(glyph_bytes);
	if (glyphs == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (command_index = 0u; command_index < command_count; ++command_index) {
		const FB_GFX3_GLYPHS_COMMAND *payload =
			(const FB_GFX3_GLYPHS_COMMAND *)commands[command_index]->payload;
		uint32_t item;

		for (item = 0u; item < payload->count; ++item) {
			const FB_GFX3_GLYPH *glyph = &payload->glyph[item];
			int64_t last_x;
			int64_t last_y;

			if ((glyph->width == 0u) || (glyph->width > 8u) ||
			    (glyph->height == 0u) || (glyph->height > 16u) ||
			    ((glyph->flags &
			      ~(uint32_t)FB_GFX3_GLYPH_BACKGROUND) != 0u)) {
				result = FB_GFX3_INVALID;
				goto done;
			}
			glyphs[glyph_index++] = *glyph;
			last_x = (int64_t)glyph->x + glyph->width - 1u;
			last_y = (int64_t)glyph->y + glyph->height - 1u;
			if ((last_x < clip.x1) || (last_y < clip.y1) ||
			    (glyph->x > clip.x2) || (glyph->y > clip.y2))
				continue;
			if (glyph->x < resolve_x1)
				resolve_x1 = glyph->x;
			if (glyph->y < resolve_y1)
				resolve_y1 = glyph->y;
			if (resolve_x1 < clip.x1)
				resolve_x1 = clip.x1;
			if (resolve_y1 < clip.y1)
				resolve_y1 = clip.y1;
			if (last_x > resolve_x2)
				resolve_x2 = (last_x > INT32_MAX) ? INT32_MAX : (int32_t)last_x;
			if (last_y > resolve_y2)
				resolve_y2 = (last_y > INT32_MAX) ? INT32_MAX : (int32_t)last_y;
			if (resolve_x2 > clip.x2)
				resolve_x2 = clip.x2;
			if (resolve_y2 > clip.y2)
				resolve_y2 = clip.y2;
		}
	}
	if ((glyph_index != total) || (resolve_x1 > resolve_x2) ||
	    (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	first_tile_x = (uint32_t)resolve_x1 / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
	first_tile_y = (uint32_t)resolve_y1 / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
	last_tile_x = (uint32_t)resolve_x2 / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
	last_tile_y = (uint32_t)resolve_y2 / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
	tiles_x = last_tile_x - first_tile_x + 1u;
	tiles_y = last_tile_y - first_tile_y + 1u;
	if ((tiles_x == 0u) || (tiles_y == 0u) ||
	    (tiles_x > UINT32_MAX / tiles_y) ||
	    ((tile_count = tiles_x * tiles_y) >
	     FB_GFX3_OPENGL_ALPHA_TILE_MAX_TILES) ||
	    (tiles_x > state->maximum_compute_groups_x) ||
	    (tiles_y > state->maximum_compute_groups_y)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	tile_counts = (uint32_t *)calloc(tile_count, sizeof(*tile_counts));
	if (tile_counts == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (index = 0u; index < total; ++index) {
		const FB_GFX3_GLYPH *glyph = &glyphs[index];
		int64_t right64 = (int64_t)glyph->x + glyph->width - 1u;
		int64_t bottom64 = (int64_t)glyph->y + glyph->height - 1u;
		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;
		uint32_t tile_y;

		if ((right64 < clip.x1) || (bottom64 < clip.y1) ||
		    (glyph->x > clip.x2) || (glyph->y > clip.y2))
			continue;
		left = (glyph->x > clip.x1) ? glyph->x : clip.x1;
		top = (glyph->y > clip.y1) ? glyph->y : clip.y1;
		right = (right64 < clip.x2) ? (int32_t)right64 : clip.x2;
		bottom = (bottom64 < clip.y2) ? (int32_t)bottom64 : clip.y2;
		for (tile_y = (uint32_t)top / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
		     tile_y <= (uint32_t)bottom / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
		     ++tile_y) {
			uint32_t tile_x;

			for (tile_x = (uint32_t)left / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
			     tile_x <= (uint32_t)right / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
			     ++tile_x) {
				uint32_t tile = (tile_y - first_tile_y) * tiles_x +
					(tile_x - first_tile_x);

				if (tile_counts[tile] == UINT32_MAX) {
					result = FB_GFX3_OUT_OF_MEMORY;
					goto done;
				}
				tile_counts[tile]++;
			}
		}
	}
	for (index = 0u; index < tile_count; ++index) {
		if (tile_counts[index] > UINT32_MAX - index_count) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto done;
		}
		index_count += tile_counts[index];
	}
	/*
		The resolved glyph bounds guarantee at least one covered tile.
		Reject an inconsistent batch explicitly instead of asking malloc()
		or the OpenGL driver to process an empty index buffer.
	*/
	if (index_count == 0u) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	tile_ranges = (uint32_t *)calloc((size_t)tile_count * 2u,
		sizeof(*tile_ranges));
	tile_cursors = (uint32_t *)calloc(tile_count, sizeof(*tile_cursors));
	tile_indices = (uint32_t *)malloc((size_t)index_count *
		sizeof(*tile_indices));
	if ((tile_ranges == NULL) || (tile_cursors == NULL) ||
	    (tile_indices == NULL)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	index_count = 0u;
	for (index = 0u; index < tile_count; ++index) {
		tile_ranges[index * 2u] = index_count;
		tile_ranges[index * 2u + 1u] = tile_counts[index];
		tile_cursors[index] = index_count;
		index_count += tile_counts[index];
	}
	for (index = 0u; index < total; ++index) {
		const FB_GFX3_GLYPH *glyph = &glyphs[index];
		int64_t right64 = (int64_t)glyph->x + glyph->width - 1u;
		int64_t bottom64 = (int64_t)glyph->y + glyph->height - 1u;
		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;
		uint32_t tile_y;

		if ((right64 < clip.x1) || (bottom64 < clip.y1) ||
		    (glyph->x > clip.x2) || (glyph->y > clip.y2))
			continue;
		left = (glyph->x > clip.x1) ? glyph->x : clip.x1;
		top = (glyph->y > clip.y1) ? glyph->y : clip.y1;
		right = (right64 < clip.x2) ? (int32_t)right64 : clip.x2;
		bottom = (bottom64 < clip.y2) ? (int32_t)bottom64 : clip.y2;
		for (tile_y = (uint32_t)top / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
		     tile_y <= (uint32_t)bottom / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
		     ++tile_y) {
			uint32_t tile_x;

			for (tile_x = (uint32_t)left / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
			     tile_x <= (uint32_t)right / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
			     ++tile_x) {
				uint32_t tile = (tile_y - first_tile_y) * tiles_x +
					(tile_x - first_tile_x);

				tile_indices[tile_cursors[tile]++] = index;
			}
		}
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)glyph_bytes,
		glyphs, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER,
		state->blit_tile_range_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)tile_count * 2u * sizeof(*tile_ranges)),
		tile_ranges, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 3,
		state->blit_tile_range_buffer);
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER,
		state->blit_tile_index_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)index_count * sizeof(*tile_indices)),
		tile_indices, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 4,
		state->blit_tile_index_buffer);
	state->gl.use_program(state->glyph_batch_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->glyph_batch_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->glyph_batch_count_location, total);
	state->gl.uniform_4i(state->glyph_batch_tile_origin_location,
		(int32_t)first_tile_x, (int32_t)first_tile_y, 0, 0);
	state->gl.uniform_1ui(state->glyph_batch_tiles_x_location, tiles_x);
	state->gl.uniform_1ui(state->glyph_batch_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	groups_x = tiles_x;
	groups_y = tiles_y;
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "ordered glyph tile dispatch");

done:
	free(tile_indices);
	free(tile_cursors);
	free(tile_ranges);
	free(tile_counts);
	free(glyphs);
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Batch admission and primitive packing                                    */
/* ------------------------------------------------------------------------- */

uint32_t fb_gfx3_gl_line_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_LINE_COMMAND *first;
	uint32_t count = 1;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_LINE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1;
	first = (const FB_GFX3_LINE_COMMAND *)commands[0]->payload;
	if ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0)
		return 1;
	if ((llabs((long long)first->x2 - first->x1) > 32767) ||
	    (llabs((long long)first->y2 - first->y1) > 32767))
		return 1;
	if (available > FB_GFX3_OPENGL_LINE_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_LINE_BATCH_LIMIT;
	while (count < available) {
		const FB_GFX3_LINE_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_LINE) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_LINE_COMMAND *)commands[count]->payload;
		if (((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) ||
		    (candidate->style != first->style) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0) ||
		    (llabs((long long)candidate->x2 - candidate->x1) > 32767) ||
		    (llabs((long long)candidate->y2 - candidate->y1) > 32767))
			break;
		count++;
	}
	return count;
}

uint32_t fb_gfx3_gl_alpha_blit_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available)
{
	const FB_GFX3_BLIT_COMMAND *first;
	uint32_t count;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_BLIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	if (((first->mode != FB_GFX3_BLIT_TRANS) &&
	    (first->mode != FB_GFX3_BLIT_PSET) &&
	    (first->mode != FB_GFX3_BLIT_PRESET) &&
	    (first->mode != FB_GFX3_BLIT_AND) &&
	    (first->mode != FB_GFX3_BLIT_OR) &&
	    (first->mode != FB_GFX3_BLIT_XOR) &&
	    (first->mode != FB_GFX3_BLIT_ALPHA) &&
	    (first->mode != FB_GFX3_BLIT_BLEND) &&
	    (first->mode != FB_GFX3_BLIT_ADD)) ||
	    (first->source == commands[0]->target))
		return 1u;
	if (available > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT;
	for (count = 1u; count < available; ++count) {
		const FB_GFX3_BLIT_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_BLIT) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_BLIT_COMMAND *)commands[count]->payload;
		if ((candidate->mode != first->mode) ||
		    (candidate->source != first->source) ||
		    (((first->mode == FB_GFX3_BLIT_BLEND) ||
		      (first->mode == FB_GFX3_BLIT_ADD)) &&
		     (candidate->alpha != first->alpha)))
			break;
	}
	return count;
}

uint32_t fb_gfx3_gl_blit_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_BLIT_COMMAND *first;
	const FB_GFX3_BLIT_COMMAND *candidate;
	uint32_t index;

	if ((commands == NULL) || (available == 0) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_BLIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1;
	first = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	if (((first->mode != FB_GFX3_BLIT_TRANS) &&
	     (first->mode != FB_GFX3_BLIT_PSET) &&
	     (first->mode != FB_GFX3_BLIT_PRESET) &&
	     (first->mode != FB_GFX3_BLIT_AND) &&
	     (first->mode != FB_GFX3_BLIT_OR) &&
	     (first->mode != FB_GFX3_BLIT_XOR)) ||
	    (first->source == commands[0]->target) ||
	    (first->source_rect.x1 > first->source_rect.x2) ||
	    (first->source_rect.y1 > first->source_rect.y2))
		return 1;

	if (available > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT;
	for (index = 1; index < available; ++index) {
		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_BLIT) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_BLIT_COMMAND *)commands[index]->payload;
		if ((candidate->source != first->source) ||
		    (commands[index]->target != commands[0]->target) ||
		    (candidate->mode != first->mode) ||
		    (candidate->source_rect.x1 > candidate->source_rect.x2) ||
		    (candidate->source_rect.y1 > candidate->source_rect.y2))
			break;
	}
	return index;
}

uint32_t fb_gfx3_gl_rectangle_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available)
{
	const FB_GFX3_RECTANGLE_COMMAND *first;
	uint32_t index;

	if ((commands == NULL) || (available < 2) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_RECTANGLE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1;
	first = (const FB_GFX3_RECTANGLE_COMMAND *)commands[0]->payload;
	if ((first->filled == 0) ||
	    ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0))
		return 1;
	if (available > FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT;
	for (index = 1; index < available; ++index) {
		const FB_GFX3_RECTANGLE_COMMAND *candidate;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_RECTANGLE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_RECTANGLE_COMMAND *)commands[index]->payload;
		if ((candidate->filled == 0) ||
		    ((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0))
			break;
	}
	return index;
}

uint32_t fb_gfx3_gl_ellipse_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available)
{
	const FB_GFX3_ELLIPSE_COMMAND *first;
	uint32_t index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_ELLIPSE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_ELLIPSE_COMMAND *)commands[0]->payload;
	if ((first->filled == 0u) ||
	    ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u))
		return 1u;
	if (available > FB_GFX3_OPENGL_ELLIPSE_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_ELLIPSE_BATCH_LIMIT;
	for (index = 1u; index < available; ++index) {
		const FB_GFX3_ELLIPSE_COMMAND *candidate;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_ELLIPSE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_ELLIPSE_COMMAND *)commands[index]->payload;
		if ((candidate->filled == 0u) ||
		    ((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u))
			break;
	}
	return index;
}

/*
	Count an adjacent opaque POINTS/LINE/LINES/ELLIPSE stream and its flattened
	GPU operations. A single public command stays on its existing specialized
	path; the mixed winner pass is valuable when it removes at least one
	intermediate resolve.
*/
uint32_t fb_gfx3_gl_primitive_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available,
	uint32_t *primitive_count_result)
{
	FB_GFX3_HANDLE target;
	uint32_t command_count = 0u;
	uint32_t primitive_count = 0u;

	if (primitive_count_result != NULL)
		*primitive_count_result = 0u;
	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (primitive_count_result == NULL))
		return 1u;
	target = commands[0]->target;
	while ((command_count < available) &&
	       (primitive_count < FB_GFX3_OPENGL_LINE_BATCH_LIMIT)) {
		FB_GFX3_COMMAND *command = commands[command_count];
		uint32_t added_count = 0u;

		if ((command == NULL) || (command->target != target))
			break;
		if (command->type == FB_GFX3_COMMAND_POINTS) {
			const FB_GFX3_POINTS_COMMAND *points;
			size_t point_bytes;
			size_t expected_size;
			uint32_t index;

			if (fb_gfx3_command_payload_size(command) <
			    offsetof(FB_GFX3_POINTS_COMMAND, point))
				break;
			points = (const FB_GFX3_POINTS_COMMAND *)command->payload;
			if ((points->count == 0u) ||
			    (fb_gfx3_size_multiply(points->count,
			     sizeof(points->point[0]), &point_bytes) != FB_GFX3_OK) ||
			    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
			     point_bytes, &expected_size) != FB_GFX3_OK) ||
			    (expected_size != fb_gfx3_command_payload_size(command)))
				break;
			for (index = 0u; index < points->count; index++) {
				if ((points->point[index].flags &
				     FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u)
					break;
			}
			if (index != points->count)
				break;
			added_count = points->count;
		} else if (command->type == FB_GFX3_COMMAND_LINE) {
			const FB_GFX3_LINE_COMMAND *line;

			if (fb_gfx3_command_payload_size(command) != sizeof(*line))
				break;
			line = (const FB_GFX3_LINE_COMMAND *)command->payload;
			if (((line->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
			    (llabs((long long)line->x2 - line->x1) > 32767) ||
			    (llabs((long long)line->y2 - line->y1) > 32767))
				break;
			added_count = 1u;
		} else if (command->type == FB_GFX3_COMMAND_LINES) {
			const FB_GFX3_LINES_COMMAND *lines;
			size_t line_bytes;
			size_t expected_size;
			uint32_t index;

			if (fb_gfx3_command_payload_size(command) <
			    offsetof(FB_GFX3_LINES_COMMAND, line))
				break;
			lines = (const FB_GFX3_LINES_COMMAND *)command->payload;
			if ((lines->count == 0u) ||
			    (fb_gfx3_size_multiply(lines->count,
			     sizeof(lines->line[0]), &line_bytes) != FB_GFX3_OK) ||
			    (fb_gfx3_size_add(offsetof(FB_GFX3_LINES_COMMAND, line),
			     line_bytes, &expected_size) != FB_GFX3_OK) ||
			    (expected_size != fb_gfx3_command_payload_size(command)))
				break;
			for (index = 0u; index < lines->count; index++) {
				const FB_GFX3_LINE_COMMAND *line = &lines->line[index];

				if (((line->flags &
				      FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
				    (llabs((long long)line->x2 - line->x1) > 32767) ||
				    (llabs((long long)line->y2 - line->y1) > 32767))
					break;
			}
			if (index != lines->count)
				break;
			added_count = lines->count;
		} else if (command->type == FB_GFX3_COMMAND_ELLIPSE) {
			const FB_GFX3_ELLIPSE_COMMAND *ellipse;

			if (fb_gfx3_command_payload_size(command) != sizeof(*ellipse))
				break;
			ellipse = (const FB_GFX3_ELLIPSE_COMMAND *)command->payload;
			if (((ellipse->flags &
			      FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
			    !(ellipse->radius_x >= 0.0f) ||
			    !(ellipse->radius_x <= 32767.0f) ||
			    !(ellipse->radius_y >= 0.0f) ||
			    !(ellipse->radius_y <= 32767.0f))
				break;
			added_count = 1u;
		} else {
			break;
		}
		if (added_count >
		    FB_GFX3_OPENGL_LINE_BATCH_LIMIT - primitive_count)
			break;
		primitive_count += added_count;
		command_count++;
	}
	if (command_count < 2u)
		return 1u;
	*primitive_count_result = primitive_count;
	return command_count;
}

static int opengl_blit_batch_winner_ensure(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_OPENGL_SURFACE *destination, int reset_generation)
{
	if ((state->blit_batch_winner_texture != 0) &&
	    (state->blit_batch_winner_width == destination->width) &&
	    (state->blit_batch_winner_height == destination->height) &&
	    !reset_generation)
		return FB_GFX3_OK;
	if (state->blit_batch_winner_texture != 0) {
		state->gl.delete_textures(1, &state->blit_batch_winner_texture);
		state->blit_batch_winner_texture = 0;
	}
	state->gl.generate_textures(1, &state->blit_batch_winner_texture);
	if (state->blit_batch_winner_texture == 0)
		return FB_GFX3_OUT_OF_MEMORY;
	state->gl.bind_texture(GL_TEXTURE_2D, state->blit_batch_winner_texture);
	state->gl.texture_storage_2d(GL_TEXTURE_2D, 1, GL_R32UI,
		(GLsizei)destination->width, (GLsizei)destination->height);
	if (fb_gfx3_gl_check_error(state, "blit batch winner allocation") !=
	    FB_GFX3_OK) {
		state->gl.delete_textures(1, &state->blit_batch_winner_texture);
		state->blit_batch_winner_texture = 0;
		return FB_GFX3_OUT_OF_MEMORY;
	}
	state->blit_batch_winner_width = destination->width;
	state->blit_batch_winner_height = destination->height;
	state->blit_batch_generation = 0;
	return FB_GFX3_OK;
}

static void opengl_primitive_batch_expand_bounds(int32_t x1, int32_t y1,
	int32_t x2, int32_t y2, int32_t *resolve_x1, int32_t *resolve_y1,
	int32_t *resolve_x2, int32_t *resolve_y2)
{
	if ((resolve_x1 == NULL) || (resolve_y1 == NULL) ||
	    (resolve_x2 == NULL) || (resolve_y2 == NULL) ||
	    (x1 > x2) || (y1 > y2))
		return;
	if (x1 < *resolve_x1)
		*resolve_x1 = x1;
	if (y1 < *resolve_y1)
		*resolve_y1 = y1;
	if (x2 > *resolve_x2)
		*resolve_x2 = x2;
	if (y2 > *resolve_y2)
		*resolve_y2 = y2;
}

static int opengl_primitive_batch_add_line(
	const FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_LINE_COMMAND *line,
	FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *item, uint32_t order,
	uint32_t *maximum_line_count, int32_t *resolve_x1,
	int32_t *resolve_y1, int32_t *resolve_x2, int32_t *resolve_y2)
{
	FB_GFX3_RECT clip;
	int64_t difference_x;
	int64_t difference_y;
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t line_count;

	if ((destination == NULL) || (line == NULL) || (item == NULL) ||
	    (order == 0u) || (maximum_line_count == NULL))
		return FB_GFX3_INVALID;
	difference_x = llabs((long long)line->x2 - line->x1);
	difference_y = llabs((long long)line->y2 - line->y1);
	if (((line->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
	    (difference_x > 32767) || (difference_y > 32767))
		return FB_GFX3_UNSUPPORTED;
	line_count = (uint32_t)((difference_x > difference_y) ?
		difference_x : difference_y) + 1u;
	if (line_count > *maximum_line_count)
		*maximum_line_count = line_count;
	item->geometry_x1 = line->x1;
	item->geometry_y1 = line->y1;
	item->geometry_x2_or_radius_x = line->x2;
	item->geometry_y2_or_radius_y = line->y2;
	item->color = line->color;
	item->type = FB_GFX3_OPENGL_PRIMITIVE_LINE;
	item->style_or_filled = line->style & 0xFFFFu;
	item->order = order;
	if (!fb_gfx3_gl_clip_rect(destination, &line->clip, &clip)) {
		item->clip_x1 = 1;
		item->clip_y1 = 1;
		item->clip_x2 = 0;
		item->clip_y2 = 0;
		return FB_GFX3_OK;
	}
	item->clip_x1 = clip.x1;
	item->clip_y1 = clip.y1;
	item->clip_x2 = clip.x2;
	item->clip_y2 = clip.y2;
	x1 = (line->x1 < line->x2) ? line->x1 : line->x2;
	y1 = (line->y1 < line->y2) ? line->y1 : line->y2;
	x2 = (line->x1 > line->x2) ? line->x1 : line->x2;
	y2 = (line->y1 > line->y2) ? line->y1 : line->y2;
	if (x1 < clip.x1)
		x1 = clip.x1;
	if (y1 < clip.y1)
		y1 = clip.y1;
	if (x2 > clip.x2)
		x2 = clip.x2;
	if (y2 > clip.y2)
		y2 = clip.y2;
	opengl_primitive_batch_expand_bounds(x1, y1, x2, y2,
		resolve_x1, resolve_y1, resolve_x2, resolve_y2);
	return FB_GFX3_OK;
}

static int opengl_primitive_batch_add_ellipse(
	const FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_ELLIPSE_COMMAND *ellipse,
	FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *item, uint32_t order,
	int32_t *resolve_x1, int32_t *resolve_y1,
	int32_t *resolve_x2, int32_t *resolve_y2)
{
	FB_GFX3_RECT clip;
	double x1;
	double y1;
	double x2;
	double y2;
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;

	if ((destination == NULL) || (ellipse == NULL) || (item == NULL) ||
	    (order == 0u))
		return FB_GFX3_INVALID;
	if (((ellipse->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
	    !(ellipse->radius_x >= 0.0f) ||
	    !(ellipse->radius_x <= 32767.0f) ||
	    !(ellipse->radius_y >= 0.0f) ||
	    !(ellipse->radius_y <= 32767.0f))
		return FB_GFX3_UNSUPPORTED;
	item->geometry_x1 = ellipse->center_x;
	item->geometry_y1 = ellipse->center_y;
	memcpy(&item->geometry_x2_or_radius_x, &ellipse->radius_x,
		sizeof(item->geometry_x2_or_radius_x));
	memcpy(&item->geometry_y2_or_radius_y, &ellipse->radius_y,
		sizeof(item->geometry_y2_or_radius_y));
	item->color = ellipse->color;
	item->type = FB_GFX3_OPENGL_PRIMITIVE_ELLIPSE;
	item->style_or_filled = ellipse->filled != 0u;
	item->order = order;
	if (!fb_gfx3_gl_clip_rect(destination, &ellipse->clip, &clip)) {
		item->clip_x1 = 1;
		item->clip_y1 = 1;
		item->clip_x2 = 0;
		item->clip_y2 = 0;
		return FB_GFX3_OK;
	}
	item->clip_x1 = clip.x1;
	item->clip_y1 = clip.y1;
	item->clip_x2 = clip.x2;
	item->clip_y2 = clip.y2;
	x1 = (double)ellipse->center_x - ellipse->radius_x;
	y1 = (double)ellipse->center_y - ellipse->radius_y;
	x2 = (double)ellipse->center_x + ellipse->radius_x;
	y2 = (double)ellipse->center_y + ellipse->radius_y;
	if ((x1 < INT32_MIN) || (y1 < INT32_MIN) ||
	    (x2 > INT32_MAX) || (y2 > INT32_MAX))
		return FB_GFX3_UNSUPPORTED;
	left = (int32_t)x1;
	top = (int32_t)y1;
	right = (int32_t)x2;
	bottom = (int32_t)y2;
	if (left < clip.x1)
		left = clip.x1;
	if (top < clip.y1)
		top = clip.y1;
	if (right > clip.x2)
		right = clip.x2;
	if (bottom > clip.y2)
		bottom = clip.y2;
	opengl_primitive_batch_expand_bounds(left, top, right, bottom,
		resolve_x1, resolve_y1, resolve_x2, resolve_y2);
	return FB_GFX3_OK;
}

static int opengl_primitive_batch_add_point(
	const FB_GFX3_OPENGL_SURFACE *destination, const FB_GFX3_RECT *command_clip,
	const FB_GFX3_POINT *point, FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *item,
	uint32_t order, int32_t *resolve_x1, int32_t *resolve_y1,
	int32_t *resolve_x2, int32_t *resolve_y2)
{
	FB_GFX3_RECT clip;

	if ((destination == NULL) || (command_clip == NULL) || (point == NULL) ||
	    (item == NULL) || (order == 0u))
		return FB_GFX3_INVALID;
	if ((point->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u)
		return FB_GFX3_UNSUPPORTED;
	item->geometry_x1 = point->x;
	item->geometry_y1 = point->y;
	item->color = point->color;
	item->type = FB_GFX3_OPENGL_PRIMITIVE_POINT;
	item->order = order;
	if (!fb_gfx3_gl_clip_rect(destination, command_clip, &clip)) {
		item->clip_x1 = 1;
		item->clip_y1 = 1;
		item->clip_x2 = 0;
		item->clip_y2 = 0;
		return FB_GFX3_OK;
	}
	item->clip_x1 = clip.x1;
	item->clip_y1 = clip.y1;
	item->clip_x2 = clip.x2;
	item->clip_y2 = clip.y2;
	if ((point->x >= clip.x1) && (point->x <= clip.x2) &&
	    (point->y >= clip.y1) && (point->y <= clip.y2))
		opengl_primitive_batch_expand_bounds(point->x, point->y,
			point->x, point->y, resolve_x1, resolve_y1,
			resolve_x2, resolve_y2);
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Ordered primitive execution                                              */
/* ------------------------------------------------------------------------- */

int fb_gfx3_gl_primitive_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t command_count,
	uint32_t primitive_count)
{
	FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *items = NULL;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	uint32_t item_index = 0u;
	uint32_t command_index;
	uint32_t maximum_line_count = 1u;
	uint32_t generation;
	uint32_t groups_x;
	uint32_t groups_y;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	int result;

	if ((state == NULL) || (commands == NULL) || (command_count < 2u) ||
	    (primitive_count < 2u) ||
	    (primitive_count > FB_GFX3_OPENGL_LINE_BATCH_LIMIT) ||
	    (commands[0] == NULL) || (commands[command_count - 1u] == NULL))
		return FB_GFX3_INVALID;
	items = (FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *)calloc(primitive_count,
		sizeof(*items));
	if (items == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	result = fb_gfx3_gl_surface_retain(state, commands[0]->target,
		commands[command_count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		goto done;
	for (command_index = 0u; command_index < command_count; command_index++) {
		FB_GFX3_COMMAND *command = commands[command_index];

		if ((command == NULL) ||
		    (command->target != commands[0]->target) ||
		    ((command_index > 0u) &&
		     (command->sequence <= commands[command_index - 1u]->sequence))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (command->type == FB_GFX3_COMMAND_POINTS) {
			const FB_GFX3_POINTS_COMMAND *points =
				(const FB_GFX3_POINTS_COMMAND *)command->payload;
			uint32_t point_index;

			result = FB_GFX3_OK;
			for (point_index = 0u;
			     (point_index < points->count) && (result == FB_GFX3_OK);
			     point_index++) {
				result = opengl_primitive_batch_add_point(destination,
					&points->clip, &points->point[point_index],
					&items[item_index], item_index + 1u,
					&resolve_x1, &resolve_y1, &resolve_x2,
					&resolve_y2);
				item_index++;
			}
		} else if (command->type == FB_GFX3_COMMAND_LINE) {
			const FB_GFX3_LINE_COMMAND *line =
				(const FB_GFX3_LINE_COMMAND *)command->payload;

			result = opengl_primitive_batch_add_line(destination, line,
				&items[item_index], item_index + 1u, &maximum_line_count,
				&resolve_x1, &resolve_y1, &resolve_x2, &resolve_y2);
			item_index++;
		} else if (command->type == FB_GFX3_COMMAND_LINES) {
			const FB_GFX3_LINES_COMMAND *lines =
				(const FB_GFX3_LINES_COMMAND *)command->payload;
			uint32_t line_index;

			result = FB_GFX3_OK;
			for (line_index = 0u;
			     (line_index < lines->count) && (result == FB_GFX3_OK);
			     line_index++) {
				result = opengl_primitive_batch_add_line(destination,
					&lines->line[line_index], &items[item_index],
					item_index + 1u, &maximum_line_count,
					&resolve_x1, &resolve_y1, &resolve_x2,
					&resolve_y2);
				item_index++;
			}
		} else if (command->type == FB_GFX3_COMMAND_ELLIPSE) {
			const FB_GFX3_ELLIPSE_COMMAND *ellipse =
				(const FB_GFX3_ELLIPSE_COMMAND *)command->payload;

			result = opengl_primitive_batch_add_ellipse(destination, ellipse,
				&items[item_index], item_index + 1u, &resolve_x1,
				&resolve_y1, &resolve_x2, &resolve_y2);
			item_index++;
		} else {
			result = FB_GFX3_INVALID;
		}
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (item_index != primitive_count) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if ((resolve_x1 > resolve_x2) || (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	groups_x = (maximum_line_count + FB_GFX3_OPENGL_POINT_GROUP_SIZE - 1u) /
		FB_GFX3_OPENGL_POINT_GROUP_SIZE;
	if ((groups_x > state->maximum_compute_groups_x) ||
	    (primitive_count > state->maximum_compute_groups_y) ||
	    ((uint64_t)primitive_count * sizeof(items[0]) >
	     state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)primitive_count * sizeof(items[0])),
		items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	if (state->blit_batch_generation >= (UINT32_MAX >> 13))
		result = opengl_blit_batch_winner_ensure(state, destination, TRUE);
	else
		result = opengl_blit_batch_winner_ensure(state, destination, FALSE);
	if (result != FB_GFX3_OK)
		goto done;
	generation = ++state->blit_batch_generation;
	state->gl.use_program(state->primitive_batch_program);
	state->gl.bind_image_texture(0, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_1ui(state->primitive_batch_key_location,
		generation << 13);
	state->gl.dispatch_compute(groups_x, primitive_count, 1u);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state,
		"mixed primitive selection dispatch");
	if (result != FB_GFX3_OK)
		goto done;
	groups_x = ((uint32_t)(resolve_x2 - resolve_x1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = ((uint32_t)(resolve_y2 - resolve_y1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	state->gl.use_program(state->primitive_batch_resolve_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->primitive_batch_resolve_rect_location,
		resolve_x1, resolve_y1, resolve_x2 - resolve_x1 + 1,
		resolve_y2 - resolve_y1 + 1);
	state->gl.uniform_1ui(
		state->primitive_batch_resolve_generation_location, generation);
	state->gl.uniform_1ui(state->primitive_batch_resolve_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1u);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "mixed primitive resolve dispatch");

done:
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	free(items);
	return result;
}

static int opengl_line_batch_common(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands,
	const FB_GFX3_LINE_COMMAND *packed_lines, FB_GFX3_HANDLE target,
	uint64_t sequence, uint32_t count)
{
	FB_GFX3_OPENGL_LINE_BATCH_ITEM items[FB_GFX3_OPENGL_LINE_BATCH_LIMIT];
	const FB_GFX3_LINE_COMMAND *first;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_RECT clip;
	uint32_t maximum_count = 0;
	uint32_t index;
	uint32_t generation;
	uint32_t groups_x;
	uint32_t groups_y;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	int result;

	if ((state == NULL) || (target == 0) || (sequence == 0u) ||
	    (count == 0u) || (count > FB_GFX3_OPENGL_LINE_BATCH_LIMIT) ||
	    ((commands == NULL) == (packed_lines == NULL)))
		return FB_GFX3_INVALID;
	if (packed_lines != NULL) {
		first = &packed_lines[0];
	} else {
		if ((commands[0] == NULL) ||
		    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
			return FB_GFX3_INVALID;
		first = (const FB_GFX3_LINE_COMMAND *)commands[0]->payload;
	}
	result = fb_gfx3_gl_surface_retain(state, target, sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	if (!fb_gfx3_gl_clip_rect(destination, &first->clip, &clip)) {
		fb_gfx3_resource_release(state->resources, target);
		return FB_GFX3_OK;
	}
	for (index = 0; index < count; index++) {
		const FB_GFX3_LINE_COMMAND *payload;
		int64_t difference_x;
		int64_t difference_y;
		int32_t x1;
		int32_t y1;
		int32_t x2;
		int32_t y2;
		uint32_t line_count;

		if (packed_lines != NULL) {
			payload = &packed_lines[index];
		} else {
			if ((commands[index] == NULL) ||
			    (commands[index]->type != FB_GFX3_COMMAND_LINE) ||
			    (commands[index]->target != target) ||
			    (fb_gfx3_command_payload_size(commands[index]) !=
			     sizeof(*payload))) {
				result = FB_GFX3_INVALID;
				goto done;
			}
			payload =
				(const FB_GFX3_LINE_COMMAND *)commands[index]->payload;
		}
		if ((payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0 ||
		    (payload->style != first->style) ||
		    (memcmp(&payload->clip, &first->clip, sizeof(payload->clip)) != 0)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		difference_x = (int64_t)payload->x2 - payload->x1;
		difference_y = (int64_t)payload->y2 - payload->y1;
		if (difference_x < 0)
			difference_x = -difference_x;
		if (difference_y < 0)
			difference_y = -difference_y;
		if ((difference_x > 32767) || (difference_y > 32767)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		line_count = (uint32_t)((difference_x > difference_y) ?
			difference_x : difference_y) + 1u;
		if (line_count > maximum_count)
			maximum_count = line_count;
		items[index].x1 = payload->x1;
		items[index].y1 = payload->y1;
		items[index].x2 = payload->x2;
		items[index].y2 = payload->y2;
		items[index].color = payload->color;
		items[index].reserved1 = 0;
		items[index].reserved2 = 0;
		items[index].reserved3 = 0;
		x1 = (payload->x1 < payload->x2) ? payload->x1 : payload->x2;
		x2 = (payload->x1 > payload->x2) ? payload->x1 : payload->x2;
		y1 = (payload->y1 < payload->y2) ? payload->y1 : payload->y2;
		y2 = (payload->y1 > payload->y2) ? payload->y1 : payload->y2;
		if (x1 < clip.x1)
			x1 = clip.x1;
		if (y1 < clip.y1)
			y1 = clip.y1;
		if (x2 > clip.x2)
			x2 = clip.x2;
		if (y2 > clip.y2)
			y2 = clip.y2;
		if ((x1 <= x2) && (y1 <= y2)) {
			if (x1 < resolve_x1)
				resolve_x1 = x1;
			if (y1 < resolve_y1)
				resolve_y1 = y1;
			if (x2 > resolve_x2)
				resolve_x2 = x2;
			if (y2 > resolve_y2)
				resolve_y2 = y2;
		}
	}
	if ((resolve_x1 > resolve_x2) || (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	groups_x = (maximum_count + FB_GFX3_OPENGL_POINT_GROUP_SIZE - 1u) /
		FB_GFX3_OPENGL_POINT_GROUP_SIZE;
	if ((groups_x > state->maximum_compute_groups_x) ||
	    (count > state->maximum_compute_groups_y) ||
	    ((uint64_t)count * sizeof(items[0]) >
	     state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)(count * sizeof(items[0])), items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	if (state->blit_batch_generation >= (UINT32_MAX >> 13))
		result = opengl_blit_batch_winner_ensure(state, destination, TRUE);
	else
		result = opengl_blit_batch_winner_ensure(state, destination, FALSE);
	if (result != FB_GFX3_OK)
		goto done;
	generation = ++state->blit_batch_generation;
	state->gl.use_program(state->line_batch_program);
	state->gl.bind_image_texture(0, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->line_batch_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->line_batch_style_location,
		first->style & 0xFFFFu);
	state->gl.uniform_1ui(state->line_batch_key_location,
		(generation << 13) | 1u);
	state->gl.dispatch_compute(groups_x, count, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "line batch selection dispatch");
	if (result != FB_GFX3_OK)
		goto done;
	groups_x = ((uint32_t)(resolve_x2 - resolve_x1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = ((uint32_t)(resolve_y2 - resolve_y1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	state->gl.use_program(state->line_batch_resolve_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->line_batch_resolve_rect_location,
		resolve_x1, resolve_y1, resolve_x2 - resolve_x1 + 1,
		resolve_y2 - resolve_y1 + 1);
	state->gl.uniform_1ui(state->line_batch_resolve_generation_location,
		generation);
	state->gl.uniform_1ui(state->line_batch_resolve_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "line batch resolve dispatch");

done:
	fb_gfx3_resource_release(state->resources, target);
	return result;
}

int fb_gfx3_gl_line_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	if ((commands == NULL) || (count == 0u) || (commands[0] == NULL) ||
	    (commands[count - 1u] == NULL))
		return FB_GFX3_INVALID;
	return opengl_line_batch_common(state, commands, NULL,
		commands[0]->target, commands[count - 1u]->sequence, count);
}

int fb_gfx3_gl_lines(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_LINES_COMMAND *payload;
	size_t line_bytes;
	size_t expected_size;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_LINES_COMMAND, line)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_LINES_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (payload->count > FB_GFX3_OPENGL_LINE_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->line[0]),
	     &line_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_LINES_COMMAND, line), line_bytes,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;
	return opengl_line_batch_common(state, NULL, payload->line,
		command->target, command->sequence, payload->count);
}

/* ------------------------------------------------------------------------- */
/* Raster and compute batch dispatch                                        */
/* ------------------------------------------------------------------------- */

static int opengl_blit_raster_batch(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_OPENGL_SURFACE *source,
	const FB_GFX3_OPENGL_BLIT_BATCH_ITEM *items, uint32_t count,
	uint32_t mode, GLenum logic_operation)
{
	int result;

	if ((state == NULL) || (destination == NULL) || (source == NULL) ||
	    (items == NULL) || (count == 0u) ||
	    (source == destination) || (state->blit_raster_batch_program == 0))
		return FB_GFX3_INVALID;
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
	result = fb_gfx3_gl_bind_color_framebuffer(state, destination->texture);
	if (result != FB_GFX3_OK)
		return result;
	state->gl.viewport(0, 0, (GLsizei)destination->width,
		(GLsizei)destination->height);
	state->gl.use_program(state->blit_raster_batch_program);
	state->gl.active_texture(GL_TEXTURE0);
	state->gl.bind_texture(GL_TEXTURE_2D, source->texture);
	state->gl.uniform_1i(state->blit_raster_batch_source_location, 0);
	state->gl.uniform_2f(state->blit_raster_batch_size_location,
		(GLfloat)destination->width,
		(GLfloat)destination->height);
	state->gl.uniform_1ui(state->blit_raster_batch_mode_location, mode);
	state->gl.uniform_1ui(state->blit_raster_batch_depth_location,
		destination->depth);
	state->gl.uniform_1ui(state->blit_raster_batch_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	state->gl.bind_vertex_array(state->blit_batch_vertex_array);
	state->gl.bind_buffer(GL_ARRAY_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(*items)),
		items, GL_STREAM_DRAW);
	if (logic_operation != 0u) {
		state->gl.enable(GL_COLOR_LOGIC_OP);
		state->gl.logic_op(logic_operation);
	}
	state->gl.draw_arrays_instanced(GL_TRIANGLES, 0, 6, (GLsizei)count);
	if (logic_operation != 0u)
		state->gl.disable(GL_COLOR_LOGIC_OP);
	state->gl.memory_barrier(GL_FRAMEBUFFER_BARRIER_BIT |
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "ordered blit raster batch");
	return result;
}

int fb_gfx3_gl_ellipse_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_OPENGL_ELLIPSE_BATCH_ITEM items[
		FB_GFX3_OPENGL_ELLIPSE_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	uint32_t generation;
	uint32_t groups_x;
	uint32_t groups_y;
	uint32_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_OPENGL_ELLIPSE_BATCH_LIMIT) ||
	    (commands[0] == NULL))
		return FB_GFX3_INVALID;
	result = fb_gfx3_gl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0u; index < count; ++index) {
		const FB_GFX3_ELLIPSE_COMMAND *payload;
		FB_GFX3_RECT clip;
		double x1;
		double y1;
		double x2;
		double y2;
		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_ELLIPSE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) != sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_ELLIPSE_COMMAND *)commands[index]->payload;
		if ((payload->filled == 0u) ||
		    ((payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
		    !(payload->radius_x >= 0.0f) ||
		    !(payload->radius_x <= 32767.0f) ||
		    !(payload->radius_y >= 0.0f) ||
		    !(payload->radius_y <= 32767.0f)) {
			result = FB_GFX3_UNSUPPORTED;
			goto done;
		}
		if (!fb_gfx3_gl_clip_rect(destination, &payload->clip, &clip)) {
			items[index].center_x = payload->center_x;
			items[index].center_y = payload->center_y;
			memcpy(&items[index].radius_x_bits, &payload->radius_x,
				sizeof(items[index].radius_x_bits));
			memcpy(&items[index].radius_y_bits, &payload->radius_y,
				sizeof(items[index].radius_y_bits));
			items[index].clip_x1 = 1;
			items[index].clip_y1 = 1;
			items[index].clip_x2 = 0;
			items[index].clip_y2 = 0;
			items[index].color = payload->color;
			items[index].reserved1 = 0u;
			items[index].reserved2 = 0u;
			items[index].reserved3 = 0u;
			continue;
		}
		x1 = (double)payload->center_x - payload->radius_x;
		y1 = (double)payload->center_y - payload->radius_y;
		x2 = (double)payload->center_x + payload->radius_x;
		y2 = (double)payload->center_y + payload->radius_y;
		if ((x1 < INT32_MIN) || (y1 < INT32_MIN) ||
		    (x2 > INT32_MAX) || (y2 > INT32_MAX)) {
			result = FB_GFX3_UNSUPPORTED;
			goto done;
		}
		left = (int32_t)x1;
		top = (int32_t)y1;
		right = (int32_t)x2;
		bottom = (int32_t)y2;
		if (left < clip.x1)
			left = clip.x1;
		if (top < clip.y1)
			top = clip.y1;
		if (right > clip.x2)
			right = clip.x2;
		if (bottom > clip.y2)
			bottom = clip.y2;
		if ((left <= right) && (top <= bottom)) {
			if (left < resolve_x1)
				resolve_x1 = left;
			if (top < resolve_y1)
				resolve_y1 = top;
			if (right > resolve_x2)
				resolve_x2 = right;
			if (bottom > resolve_y2)
				resolve_y2 = bottom;
		}
		items[index].center_x = payload->center_x;
		items[index].center_y = payload->center_y;
		memcpy(&items[index].radius_x_bits, &payload->radius_x,
			sizeof(items[index].radius_x_bits));
		memcpy(&items[index].radius_y_bits, &payload->radius_y,
			sizeof(items[index].radius_y_bits));
		items[index].clip_x1 = clip.x1;
		items[index].clip_y1 = clip.y1;
		items[index].clip_x2 = clip.x2;
		items[index].clip_y2 = clip.y2;
		items[index].color = payload->color;
		items[index].reserved1 = 0u;
		items[index].reserved2 = 0u;
		items[index].reserved3 = 0u;
	}
	if ((resolve_x1 > resolve_x2) || (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	if ((count > state->maximum_compute_groups_z) ||
	    ((uint64_t)count * sizeof(items[0]) >
	     state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)(count * sizeof(items[0])), items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	if (state->blit_batch_generation >= (UINT32_MAX >> 13))
		result = opengl_blit_batch_winner_ensure(state, destination, TRUE);
	else
		result = opengl_blit_batch_winner_ensure(state, destination, FALSE);
	if (result != FB_GFX3_OK)
		goto done;
	generation = ++state->blit_batch_generation;
	state->gl.use_program(state->ellipse_batch_program);
	state->gl.bind_image_texture(0, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_1ui(state->ellipse_batch_key_location,
		(generation << 13) | 1u);
	state->gl.dispatch_compute(1, 1, count);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "ellipse batch selection dispatch");
	if (result != FB_GFX3_OK)
		goto done;
	groups_x = ((uint32_t)(resolve_x2 - resolve_x1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = ((uint32_t)(resolve_y2 - resolve_y1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	state->gl.use_program(state->ellipse_batch_resolve_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->ellipse_batch_resolve_rect_location,
		resolve_x1, resolve_y1, resolve_x2 - resolve_x1 + 1,
		resolve_y2 - resolve_y1 + 1);
	state->gl.uniform_1ui(state->ellipse_batch_resolve_generation_location,
		generation);
	state->gl.uniform_1ui(state->ellipse_batch_resolve_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "ellipse batch resolve dispatch");

done:
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

int fb_gfx3_gl_blit_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	const FB_GFX3_BLIT_COMMAND *payload;
	FB_GFX3_OPENGL_BLIT_BATCH_ITEM items[
		FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_OPENGL_SURFACE *source = NULL;
	FB_GFX3_RECT clip;
	uint32_t item_count = 0;
	uint32_t index;
	GLenum logic_operation = 0;
	int64_t draw_x2;
	int64_t draw_y2;
	int result = FB_GFX3_OK;

	if ((count < 2) || (count > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	if ((commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	for (index = 1u; index < count; index++) {
		if ((commands[index] == NULL) ||
		    (commands[index]->sequence <= commands[index - 1u]->sequence) ||
		    (fb_gfx3_command_payload_size(commands[index]) != sizeof(*payload)))
			return FB_GFX3_INVALID;
	}
	payload = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	/*
		The coalescer accepts only one source and destination handle. Retaining
		them at the batch tail protects every command in this FIFO run while
		avoiding two registry operations for every individual sprite.
	*/
	result = fb_gfx3_gl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_gl_surface_retain(state, payload->source,
		commands[count - 1u]->sequence, &source);
	if (result != FB_GFX3_OK)
		goto done;
	if ((source == destination) || (source->depth != destination->depth)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	for (index = 0; index < count; ++index) {
		if ((commands[index] == NULL) ||
		    ((index > 0u) && (commands[index]->sequence <=
		     commands[index - 1u]->sequence)) ||
		    (fb_gfx3_command_payload_size(commands[index]) != sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_BLIT_COMMAND *)commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (payload->source != ((const FB_GFX3_BLIT_COMMAND *)
			commands[0]->payload)->source) ||
		    ((index > 0u) &&
		     (commands[index]->sequence <= commands[index - 1u]->sequence)) ||
		    (payload->source_rect.x1 < 0) ||
		    (payload->source_rect.y1 < 0) ||
		    (payload->source_rect.x1 > payload->source_rect.x2) ||
		    (payload->source_rect.y1 > payload->source_rect.y2) ||
		    (payload->source_rect.x2 >= (int32_t)source->width) ||
		    (payload->source_rect.y2 >= (int32_t)source->height)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (!fb_gfx3_gl_clip_rect(destination, &payload->clip, &clip))
			continue;
		draw_x2 = (int64_t)payload->destination_x +
			payload->source_rect.x2 - payload->source_rect.x1;
		draw_y2 = (int64_t)payload->destination_y +
			payload->source_rect.y2 - payload->source_rect.y1;
		if ((draw_x2 < clip.x1) || (draw_y2 < clip.y1) ||
		    (payload->destination_x > clip.x2) ||
		    (payload->destination_y > clip.y2))
			continue;
		items[item_count].source_x = payload->source_rect.x1;
		items[item_count].source_y = payload->source_rect.y1;
		items[item_count].width = payload->source_rect.x2 -
			payload->source_rect.x1 + 1;
		items[item_count].height = payload->source_rect.y2 -
			payload->source_rect.y1 + 1;
		items[item_count].clip_x1 = clip.x1;
		items[item_count].clip_y1 = clip.y1;
		items[item_count].clip_x2 = clip.x2;
		items[item_count].clip_y2 = clip.y2;
		items[item_count].destination_x = payload->destination_x;
		items[item_count].destination_y = payload->destination_y;
		items[item_count].reserved1 = 0;
		items[item_count].reserved2 = 0;
		item_count++;
	}
	if (item_count != 0u) {
		payload = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
		switch (payload->mode) {
		case FB_GFX3_BLIT_AND:
			logic_operation = GL_AND;
			break;
		case FB_GFX3_BLIT_OR:
			logic_operation = GL_OR;
			break;
		case FB_GFX3_BLIT_XOR:
			logic_operation = GL_XOR;
			break;
		default:
			break;
		}
		result = opengl_blit_raster_batch(state, destination, source, items,
			item_count, payload->mode, logic_operation);
	}

done:
	if (source != NULL)
		fb_gfx3_resource_release(state->resources,
			((const FB_GFX3_BLIT_COMMAND *)commands[0]->payload)->source);
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

/*
	Execute one adjacent source/mode run from a heterogeneous BLITS packet. The
	packet preserves BASIC order, while each run still uses the existing
	instanced GPU path which binds one source texture.
*/
static int opengl_blits_run(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_COMMAND *command, FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_BLIT_COMMAND *blits, uint32_t count)
{
	FB_GFX3_OPENGL_BLIT_BATCH_ITEM items[
		FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *source = NULL;
	FB_GFX3_HANDLE source_handle;
	uint32_t mode;
	uint32_t alpha;
	uint32_t item_count = 0u;
	uint32_t index;
	GLenum logic_operation = 0;
	int result;

	if ((state == NULL) || (command == NULL) || (destination == NULL) ||
	    (blits == NULL) || (count == 0u) ||
	    (count > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	source_handle = blits[0].source;
	mode = blits[0].mode;
	alpha = blits[0].alpha;
	if ((source_handle == 0) || (source_handle == command->target))
		return FB_GFX3_INVALID;
	result = fb_gfx3_gl_surface_retain(state, source_handle, command->sequence,
		&source);
	if (result != FB_GFX3_OK)
		return result;
	if (source->depth != destination->depth) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	for (index = 0u; index < count; index++) {
		const FB_GFX3_BLIT_COMMAND *blit = &blits[index];

		if ((blit->source != source_handle) || (blit->mode != mode) ||
		    (blit->alpha != alpha) ||
		    (blit->source_rect.x1 < 0) || (blit->source_rect.y1 < 0) ||
		    (blit->source_rect.x1 > blit->source_rect.x2) ||
		    (blit->source_rect.y1 > blit->source_rect.y2) ||
		    (blit->source_rect.x2 >= (int32_t)source->width) ||
		    (blit->source_rect.y2 >= (int32_t)source->height)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		items[item_count].source_x = blit->source_rect.x1;
		items[item_count].source_y = blit->source_rect.y1;
		items[item_count].width = blit->source_rect.x2 -
			blit->source_rect.x1 + 1;
		items[item_count].height = blit->source_rect.y2 -
			blit->source_rect.y1 + 1;
		/*
			The vertex and fragment stages carry the public geometry and clip.
			Framebuffer clipping rejects offscreen triangles before fragment work;
			the fragment comparison applies a smaller VIEW without CPU trimming.
		*/
		items[item_count].clip_x1 = blit->clip.x1;
		items[item_count].clip_y1 = blit->clip.y1;
		items[item_count].clip_x2 = blit->clip.x2;
		items[item_count].clip_y2 = blit->clip.y2;
		items[item_count].destination_x = blit->destination_x;
		items[item_count].destination_y = blit->destination_y;
		items[item_count].reserved1 = 0u;
		items[item_count].reserved2 = 0u;
		item_count++;
	}
	switch (mode) {
	case FB_GFX3_BLIT_AND:
		logic_operation = GL_AND;
		break;
	case FB_GFX3_BLIT_OR:
		logic_operation = GL_OR;
		break;
	case FB_GFX3_BLIT_XOR:
		logic_operation = GL_XOR;
		break;
	default:
		break;
	}
	if (item_count != 0u)
		result = opengl_blit_raster_batch(state, destination, source, items,
			item_count, mode, logic_operation);

done:
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, source_handle);
	return result;
}

/*
	The producer keeps an entire ordered sprite stream in one allocation even
	when adjacent PUTs use different cached images. Split that stream only at
	texture-binding boundaries on the renderer thread. This removes command and
	queue overhead without reordering overlapping sprites.
*/
int fb_gfx3_gl_blits(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_BLITS_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	size_t blit_bytes;
	size_t expected_size;
	uint32_t run_start;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_BLITS_COMMAND, blit)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_BLITS_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (payload->count > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->blit[0]),
	     &blit_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_BLITS_COMMAND, blit), blit_bytes,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)) ||
	    (payload->source != payload->blit[0].source) ||
	    (payload->mode != payload->blit[0].mode) ||
	    (payload->alpha != payload->blit[0].alpha))
		return FB_GFX3_INVALID;
	result = fb_gfx3_gl_surface_retain(state, command->target, command->sequence,
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	run_start = 0u;
	while (run_start < payload->count) {
		uint32_t run_end = run_start + 1u;
		const FB_GFX3_BLIT_COMMAND *first = &payload->blit[run_start];

		while ((run_end < payload->count) &&
		       (payload->blit[run_end].source == first->source) &&
		       (payload->blit[run_end].mode == first->mode) &&
		       (payload->blit[run_end].alpha == first->alpha))
			run_end++;
		result = opengl_blits_run(state, command, destination,
			payload->blit + run_start, run_end - run_start);
		if (result != FB_GFX3_OK)
			break;
		run_start = run_end;
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_rectangle_raster_batch(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM *items, uint32_t count)
{
	int result;

	if ((state == NULL) || (destination == NULL) || (items == NULL) ||
	    (count == 0u) || (state->rectangle_raster_batch_program == 0u))
		return FB_GFX3_INVALID;
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
	result = fb_gfx3_gl_bind_color_framebuffer(state, destination->texture);
	if (result != FB_GFX3_OK)
		return result;
	state->gl.viewport(0, 0, (GLsizei)destination->width,
		(GLsizei)destination->height);
	state->gl.use_program(state->rectangle_raster_batch_program);
	state->gl.uniform_2f(state->rectangle_raster_batch_size_location,
		(GLfloat)destination->width, (GLfloat)destination->height);
	state->gl.uniform_1ui(state->rectangle_raster_batch_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	state->gl.bind_vertex_array(state->rectangle_batch_vertex_array);
	state->gl.bind_buffer(GL_ARRAY_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(*items)),
		items, GL_STREAM_DRAW);
	state->gl.draw_arrays_instanced(GL_TRIANGLES, 0, 6, (GLsizei)count);
	state->gl.memory_barrier(GL_FRAMEBUFFER_BARRIER_BIT |
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "ordered rectangle raster batch");
	return result;
}

int fb_gfx3_gl_alpha_blit_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_OPENGL_BLIT_BATCH_ITEM *items = NULL;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_OPENGL_SURFACE *source = NULL;
	uint32_t *tile_counts = NULL;
	uint32_t *tile_ranges = NULL;
	uint32_t *tile_cursors = NULL;
	uint32_t *tile_indices = NULL;
	const FB_GFX3_BLIT_COMMAND *first;
	uint32_t tiles_x;
	uint32_t tiles_y;
	uint32_t tile_count;
	uint32_t index_count = 0;
	uint32_t index;
	uint32_t item_count = 0;
	uint32_t groups_x;
	uint32_t groups_y;
	int result = FB_GFX3_OK;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	if (((first->mode != FB_GFX3_BLIT_TRANS) &&
	     (first->mode != FB_GFX3_BLIT_PSET) &&
	     (first->mode != FB_GFX3_BLIT_PRESET) &&
	     (first->mode != FB_GFX3_BLIT_AND) &&
	     (first->mode != FB_GFX3_BLIT_OR) &&
	     (first->mode != FB_GFX3_BLIT_XOR) &&
	     (first->mode != FB_GFX3_BLIT_ALPHA) &&
	     (first->mode != FB_GFX3_BLIT_BLEND) &&
	     (first->mode != FB_GFX3_BLIT_ADD)) ||
	    (first->source == commands[0]->target))
		return FB_GFX3_UNSUPPORTED;
	result = fb_gfx3_gl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_gl_surface_retain(state, first->source,
		commands[count - 1u]->sequence, &source);
	if (result != FB_GFX3_OK)
		goto done;
	if ((source == destination) || (source->depth != destination->depth)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	tiles_x = (destination->width + FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) /
		FB_GFX3_OPENGL_LOCAL_SIZE_X;
	tiles_y = (destination->height + FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) /
		FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	if ((tiles_x == 0u) || (tiles_y == 0u) ||
	    (tiles_x > UINT32_MAX / tiles_y) ||
	    ((tile_count = tiles_x * tiles_y) > FB_GFX3_OPENGL_ALPHA_TILE_MAX_TILES)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	items = (FB_GFX3_OPENGL_BLIT_BATCH_ITEM *)calloc(count, sizeof(*items));
	tile_counts = (uint32_t *)calloc(tile_count, sizeof(*tile_counts));
	if ((items == NULL) || (tile_counts == NULL)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (index = 0u; index < count; ++index) {
		const FB_GFX3_BLIT_COMMAND *payload;
		FB_GFX3_RECT clip;
		int64_t right;
		int64_t bottom;
		int32_t left;
		int32_t top;
		int32_t clipped_right;
		int32_t clipped_bottom;
		uint32_t first_tile_x;
		uint32_t first_tile_y;
		uint32_t last_tile_x;
		uint32_t last_tile_y;
		uint32_t tile_y;

		if ((commands[index] == NULL) ||
		    ((index > 0u) && (commands[index]->sequence <=
		     commands[index - 1u]->sequence)) ||
		    (fb_gfx3_command_payload_size(commands[index]) != sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_BLIT_COMMAND *)commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (payload->source != first->source) ||
			(payload->mode != first->mode) ||
			(((first->mode == FB_GFX3_BLIT_BLEND) ||
			  (first->mode == FB_GFX3_BLIT_ADD)) &&
			 (payload->alpha != first->alpha)) ||
		    (payload->source_rect.x1 < 0) || (payload->source_rect.y1 < 0) ||
		    (payload->source_rect.x1 > payload->source_rect.x2) ||
		    (payload->source_rect.y1 > payload->source_rect.y2) ||
		    (payload->source_rect.x2 >= (int32_t)source->width) ||
		    (payload->source_rect.y2 >= (int32_t)source->height)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (!fb_gfx3_gl_clip_rect(destination, &payload->clip, &clip))
			continue;
		right = (int64_t)payload->destination_x +
			payload->source_rect.x2 - payload->source_rect.x1;
		bottom = (int64_t)payload->destination_y +
			payload->source_rect.y2 - payload->source_rect.y1;
		if ((right < clip.x1) || (bottom < clip.y1) ||
		    (payload->destination_x > clip.x2) ||
		    (payload->destination_y > clip.y2))
			continue;
		left = (payload->destination_x > clip.x1) ? payload->destination_x : clip.x1;
		top = (payload->destination_y > clip.y1) ? payload->destination_y : clip.y1;
		clipped_right = (right < clip.x2) ? (int32_t)right : clip.x2;
		clipped_bottom = (bottom < clip.y2) ? (int32_t)bottom : clip.y2;
		items[item_count].source_x = payload->source_rect.x1;
		items[item_count].source_y = payload->source_rect.y1;
		items[item_count].width = payload->source_rect.x2 - payload->source_rect.x1 + 1;
		items[item_count].height = payload->source_rect.y2 - payload->source_rect.y1 + 1;
		items[item_count].clip_x1 = clip.x1;
		items[item_count].clip_y1 = clip.y1;
		items[item_count].clip_x2 = clip.x2;
		items[item_count].clip_y2 = clip.y2;
		items[item_count].destination_x = payload->destination_x;
		items[item_count].destination_y = payload->destination_y;
		first_tile_x = (uint32_t)left / FB_GFX3_OPENGL_LOCAL_SIZE_X;
		first_tile_y = (uint32_t)top / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
		last_tile_x = (uint32_t)clipped_right / FB_GFX3_OPENGL_LOCAL_SIZE_X;
		last_tile_y = (uint32_t)clipped_bottom / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
		for (tile_y = first_tile_y; tile_y <= last_tile_y; ++tile_y) {
			uint32_t tile_x;
			for (tile_x = first_tile_x; tile_x <= last_tile_x; ++tile_x) {
				uint32_t tile = tile_y * tiles_x + tile_x;
				if (tile_counts[tile] == UINT32_MAX) {
					result = FB_GFX3_OUT_OF_MEMORY;
					goto done;
				}
				tile_counts[tile]++;
			}
		}
		item_count++;
	}
	if (item_count == 0u)
		goto done;
	for (index = 0u; index < tile_count; ++index) {
		if (tile_counts[index] > UINT32_MAX - index_count) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto done;
		}
		index_count += tile_counts[index];
	}
	/*
		Every retained item intersects at least one tile.  Keep the
		invariant explicit so zero-sized allocations cannot reach either
		the C allocator or the OpenGL buffer upload path.
	*/
	if (index_count == 0u) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	tile_ranges = (uint32_t *)calloc((size_t)tile_count * 2u, sizeof(*tile_ranges));
	tile_cursors = (uint32_t *)calloc(tile_count, sizeof(*tile_cursors));
	tile_indices = (uint32_t *)malloc((size_t)index_count * sizeof(*tile_indices));
	if ((tile_ranges == NULL) || (tile_cursors == NULL) ||
	    (tile_indices == NULL)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	index_count = 0u;
	for (index = 0u; index < tile_count; ++index) {
		tile_ranges[index * 2u] = index_count;
		tile_ranges[index * 2u + 1u] = tile_counts[index];
		tile_cursors[index] = index_count;
		index_count += tile_counts[index];
	}
	for (index = 0u; index < item_count; ++index) {
		const FB_GFX3_OPENGL_BLIT_BATCH_ITEM *item = &items[index];
		int32_t left = (item->destination_x > item->clip_x1) ? item->destination_x : item->clip_x1;
		int32_t top = (item->destination_y > item->clip_y1) ? item->destination_y : item->clip_y1;
		int32_t right = ((int64_t)item->destination_x + item->width - 1 < item->clip_x2) ?
			(int32_t)((int64_t)item->destination_x + item->width - 1) : item->clip_x2;
		int32_t bottom = ((int64_t)item->destination_y + item->height - 1 < item->clip_y2) ?
			(int32_t)((int64_t)item->destination_y + item->height - 1) : item->clip_y2;
		uint32_t tile_y;
		for (tile_y = (uint32_t)top / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
		     tile_y <= (uint32_t)bottom / FB_GFX3_OPENGL_LOCAL_SIZE_Y; ++tile_y) {
			uint32_t tile_x;
			for (tile_x = (uint32_t)left / FB_GFX3_OPENGL_LOCAL_SIZE_X;
			     tile_x <= (uint32_t)right / FB_GFX3_OPENGL_LOCAL_SIZE_X; ++tile_x) {
				uint32_t tile = tile_y * tiles_x + tile_x;
				tile_indices[tile_cursors[tile]++] = index;
			}
		}
	}
	groups_x = tiles_x;
	groups_y = tiles_y;
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_tile_range_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)tile_count * 2u * sizeof(*tile_ranges)), tile_ranges,
		GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2, state->blit_tile_range_buffer);
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_tile_index_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)index_count * sizeof(*tile_indices)), tile_indices,
		GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 3, state->blit_tile_index_buffer);
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)item_count * sizeof(*items)), items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 4, state->blit_batch_buffer);
	state->gl.use_program(state->blit_alpha_tile_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, source->texture, 0, GL_FALSE, 0,
		GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_1ui(state->blit_alpha_tile_count_location, tiles_x);
	state->gl.uniform_1ui(state->blit_alpha_tile_mode_location, first->mode);
	state->gl.uniform_1ui(state->blit_alpha_tile_alpha_location, first->alpha);
	state->gl.uniform_1ui(state->blit_alpha_tile_depth_location,
		destination->depth);
	state->gl.uniform_1ui(state->blit_alpha_tile_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "ordered tile blit compute dispatch");

done:
	free(tile_indices);
	free(tile_cursors);
	free(tile_ranges);
	free(tile_counts);
	free(items);
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, first->source);
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static int opengl_rectangle_item_append(
	FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM *items, uint32_t *count,
	const FB_GFX3_RECT *clip, int32_t x1, int32_t y1, int32_t x2,
	int32_t y2, uint32_t color)
{
	FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM *item;

	if ((items == NULL) || (count == NULL) || (clip == NULL) ||
	    (x1 > x2) || (y1 > y2))
		return FB_GFX3_INVALID;
	if ((x2 < clip->x1) || (y2 < clip->y1) ||
	    (x1 > clip->x2) || (y1 > clip->y2))
		return FB_GFX3_OK;
	if (*count >= FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT)
		return FB_GFX3_OUT_OF_MEMORY;
	item = &items[(*count)++];
	item->x1 = (x1 < clip->x1) ? clip->x1 : x1;
	item->y1 = (y1 < clip->y1) ? clip->y1 : y1;
	item->x2 = (x2 > clip->x2) ? clip->x2 : x2;
	item->y2 = (y2 > clip->y2) ? clip->y2 : y2;
	item->color = color;
	item->reserved1 = 0u;
	item->reserved2 = 0u;
	item->reserved3 = 0u;
	return FB_GFX3_OK;
}

/*
	Producer-side rectangle packets remove one heap command per ordinary
	LINE ... B/BF. Their payload is already FIFO ordered, and framebuffer
	raster order preserves that same last-writer behavior. A solid outline is
	four non-overlapping quads, so it can share the filled-box instanced draw
	without asking the CPU to rasterize any pixels.
*/
int fb_gfx3_gl_rectangles(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_RECTANGLES_COMMAND *payload;
	FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM items[
		FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	size_t rectangle_bytes;
	size_t expected_size;
	uint32_t item_count = 0u;
	uint32_t index;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_RECTANGLES_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (payload->count > FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->rectangle[0]),
	     &rectangle_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle),
	     rectangle_bytes, &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;
	result = fb_gfx3_gl_surface_retain(state, command->target, command->sequence,
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0u; index < payload->count; ++index) {
		const FB_GFX3_RECTANGLE_COMMAND *rectangle = &payload->rectangle[index];
		FB_GFX3_RECT clip;

		if (((rectangle->filled == 0u) &&
		     ((rectangle->style & 0xFFFFu) != 0xFFFFu)) ||
		    ((rectangle->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
		    (rectangle->x1 > rectangle->x2) ||
		    (rectangle->y1 > rectangle->y2)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (!fb_gfx3_gl_clip_rect(destination, &rectangle->clip, &clip) ||
		    (rectangle->x2 < clip.x1) || (rectangle->y2 < clip.y1) ||
		    (rectangle->x1 > clip.x2) || (rectangle->y1 > clip.y2))
			continue;
		if (rectangle->filled != 0u) {
			result = opengl_rectangle_item_append(items, &item_count, &clip,
				rectangle->x1, rectangle->y1, rectangle->x2,
				rectangle->y2, rectangle->color);
		} else {
			result = opengl_rectangle_item_append(items, &item_count, &clip,
				rectangle->x1, rectangle->y2, rectangle->x2,
				rectangle->y2, rectangle->color);
			if ((result == FB_GFX3_OK) &&
			    (rectangle->y1 != rectangle->y2))
				result = opengl_rectangle_item_append(items, &item_count,
					&clip, rectangle->x1, rectangle->y1,
					rectangle->x2, rectangle->y1, rectangle->color);
			if ((result == FB_GFX3_OK) &&
			    ((int64_t)rectangle->y2 - rectangle->y1 > 1))
				result = opengl_rectangle_item_append(items, &item_count,
					&clip, rectangle->x2, rectangle->y1 + 1,
					rectangle->x2, rectangle->y2 - 1,
					rectangle->color);
			if ((result == FB_GFX3_OK) &&
			    (rectangle->x1 != rectangle->x2) &&
			    ((int64_t)rectangle->y2 - rectangle->y1 > 1))
				result = opengl_rectangle_item_append(items, &item_count,
					&clip, rectangle->x1, rectangle->y1 + 1,
					rectangle->x1, rectangle->y2 - 1,
					rectangle->color);
		}
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (item_count != 0u)
		result = opengl_rectangle_raster_batch(state, destination, items,
			item_count);

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

int fb_gfx3_gl_rectangle_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM items[
		FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	const FB_GFX3_RECTANGLE_COMMAND *payload;
	FB_GFX3_RECT clip;
	uint32_t item_count = 0;
	uint32_t index;
	int result = FB_GFX3_OK;

	if ((count < 2) || (count > FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	/*
		The batch counter accepts one destination handle only. Retain that
		surface through the final sequence once, rather than performing a
		registry retain/release pair for every small filled box.
	*/
	result = fb_gfx3_gl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0; index < count; ++index) {
		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_RECTANGLE) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_RECTANGLE_COMMAND *)commands[index]->payload;
		if ((payload->filled == 0) ||
		    ((payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) ||
		    (payload->x1 > payload->x2) || (payload->y1 > payload->y2)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (commands[index]->target != commands[0]->target) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (!fb_gfx3_gl_clip_rect(destination, &payload->clip, &clip) ||
		    (payload->x2 < clip.x1) || (payload->y2 < clip.y1) ||
		    (payload->x1 > clip.x2) || (payload->y1 > clip.y2))
			continue;
		items[item_count].x1 = (payload->x1 < clip.x1) ? clip.x1 : payload->x1;
		items[item_count].y1 = (payload->y1 < clip.y1) ? clip.y1 : payload->y1;
		items[item_count].x2 = (payload->x2 > clip.x2) ? clip.x2 : payload->x2;
		items[item_count].y2 = (payload->y2 > clip.y2) ? clip.y2 : payload->y2;
		items[item_count].color = payload->color;
		items[item_count].reserved1 = 0;
		items[item_count].reserved2 = 0;
		items[item_count].reserved3 = 0;
		item_count++;
	}
	if (item_count == 0)
		goto done;
	if ((uint64_t)item_count * sizeof(items[0]) >
	    state->maximum_storage_buffer_size) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	result = opengl_rectangle_raster_batch(state, destination, items, item_count);

done:
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

#endif

/* end of gfx3_opengl_batch.c */
