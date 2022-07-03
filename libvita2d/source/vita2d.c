#include <vitasdk.h>
#include <vitaGL.h>
#include "../include/vita2d_vgl.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_W 960
#define SCREEN_H 544

static GLfloat v2d_clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
static unsigned int v2d_clear_color_u32 = 0xFF000000;
static GLboolean has_common_dialog = GL_FALSE;
static GLboolean has_clipping = GL_FALSE;
static GLint v2d_scissor_region[4] = {0, 0, 960, 544};
static GLuint v2d_curr_fbo = 0;
static const int v2d_num_circle_segments = 100;
static vita2d_clear_vertex *v2d_circle_vertices;
static uint16_t *v2d_circle_indices;
static GLboolean has_additive_blending = GL_FALSE;
static GLboolean v2d_inited = GL_FALSE;

static void _reset_blending() {
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	if (has_additive_blending) {
		glBlendFunc(GL_ONE, GL_ONE);
	} else {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void vita2d_set_blend_mode_add(int enable) {
	has_additive_blending = enable;
	_reset_blending();
}

int vita2d_init() {
	if (v2d_inited)
		return 0;
	v2d_circle_vertices = (vita2d_clear_vertex *)vglMalloc((v2d_num_circle_segments + 1) * sizeof(vita2d_clear_vertex));
	v2d_circle_indices = (uint16_t *)vglMalloc((v2d_num_circle_segments + 2) * sizeof(uint16_t));
	
	sceSysmoduleLoadModule(SCE_SYSMODULE_PGF);

	v2d_inited = GL_TRUE;
	return 0;
}

int vita2d_fini() {
	if (v2d_inited) {
		vglFree(v2d_circle_vertices);
		vglFree(v2d_circle_indices);
		v2d_inited = GL_FALSE;
	}
	return 0;
}

void vita2d_wait_rendering_done() {
	glFinish();
}

void vita2d_clear_screen() {
	glClearColor(v2d_clear_color[0], v2d_clear_color[1], v2d_clear_color[2], v2d_clear_color[3]);
	glClear(GL_COLOR_BUFFER_BIT);
}

void vita2d_swap_buffers() {
	vglSwapBuffers(has_common_dialog);
	has_common_dialog = GL_FALSE;
}

void vita2d_start_drawing() {
	glUseProgram(0);
	_reset_blending();
	glBindFramebuffer(GL_FRAMEBUFFER, v2d_curr_fbo);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0, SCREEN_W, SCREEN_H, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (has_clipping) {
		glScissor(v2d_scissor_region[0], v2d_scissor_region[1], v2d_scissor_region[2], v2d_scissor_region[3]);
		glEnable(GL_SCISSOR_TEST);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
}

void vita2d_start_drawing_advanced(vita2d_texture *target, unsigned int flags) {
	v2d_curr_fbo = target->fbo;
	vita2d_start_drawing();
	v2d_curr_fbo = 0;
}

void vita2d_end_drawing() {
}

int vita2d_common_dialog_update() {
	has_common_dialog = GL_TRUE;
	return 0;
}

void vita2d_set_clear_color(unsigned int color) {
	v2d_clear_color[0] = ((color >> 8*0) & 0xFF)/255.0f;
	v2d_clear_color[1] = ((color >> 8*1) & 0xFF)/255.0f;
	v2d_clear_color[2] = ((color >> 8*2) & 0xFF)/255.0f;
	v2d_clear_color[3] = ((color >> 8*3) & 0xFF)/255.0f;
	v2d_clear_color_u32 = color;
}

unsigned int vita2d_get_clear_color() {
	return v2d_clear_color_u32;
}

void vita2d_set_vblank_wait(int enable) {
	vglWaitVblankStart(enable);
}

void vita2d_set_clip_rectangle(int x_min, int y_min, int x_max, int y_max) {
	// FIXME: Since we use scissoring, we ignore 'mode'
	GLint y = SCREEN_H - y_min;
	GLint w = x_max - x_min;
	GLint h = y - (SCREEN_H - y_max);
	glScissor(x_min, y, x_max, h);
	v2d_scissor_region[0] = x_min;
	v2d_scissor_region[1] = y;
	v2d_scissor_region[2] = w;
	v2d_scissor_region[3] = h;
}

void vita2d_get_clip_rectangle(int *x_min, int *y_min, int *x_max, int *y_max) {
	*x_min = v2d_scissor_region[0];
	*y_min = SCREEN_H - v2d_scissor_region[1];
	*x_max = v2d_scissor_region[2] + *x_min;
	*y_max = v2d_scissor_region[3] - v2d_scissor_region[1] + SCREEN_H;
}

void vita2d_enable_clipping() {
	has_clipping = GL_TRUE;
	glEnable(GL_SCISSOR_TEST);
}

void vita2d_disable_clipping() {
	has_clipping = GL_FALSE;
	glDisable(GL_SCISSOR_TEST);
}

int vita2d_get_clipping_enabled() {
	return has_clipping;
}

void vita2d_draw_pixel(float x, float y, unsigned int color) {
	GLfloat vtx[2] = {
		x, y
	};
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glDrawArrays(GL_POINTS, 0, 1);
}

void vita2d_draw_line(float x0, float y0, float x1, float y1, unsigned int color) {
	GLfloat vtx[4] = {
		x0, y0,
		x1, y1
	};
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glDrawArrays(GL_LINES, 0, 2);
}

void vita2d_draw_rectangle(float x, float y, float w, float h, unsigned int color) {
	GLfloat vtx[8] = {
		x, y,
		x + w, y,
		x, y + h,
		x + w, y + h
	};
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void vita2d_draw_fill_circle(float x, float y, float radius, unsigned int color) {
	v2d_circle_vertices[0].x = x;
	v2d_circle_vertices[0].y = y;
	v2d_circle_indices[0] = 0;

	float theta = 2 * M_PI / (float)v2d_num_circle_segments;
	float c = cosf(theta);
	float s = sinf(theta);
	float t;

	float xx = radius;
	float yy = 0;
	int i;

	for (i = 1; i <= v2d_num_circle_segments; i++) {
		v2d_circle_vertices[i].x = x + xx;
		v2d_circle_vertices[i].y = y + yy;
		v2d_circle_indices[i] = i;

		t = xx;
		xx = c * xx - s * yy;
		yy = s * t + c * yy;
	}

	v2d_circle_indices[v2d_num_circle_segments + 1] = 1;
	
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, v2d_circle_vertices);
	glDrawElements(GL_TRIANGLE_FAN, v2d_num_circle_segments + 2, GL_UNSIGNED_SHORT, v2d_circle_indices);
}

uint32_t bpp_from_format(SceGxmTextureFormat format) {
	switch (format) {
	case SCE_GXM_TEXTURE_FORMAT_U8_R:
		return 1;
	case SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB:
	case SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_RGBA:
	case SCE_GXM_TEXTURE_FORMAT_U5U5U5U1_RGBA:
		return 2;
	case SCE_GXM_TEXTURE_FORMAT_U8U8U8_RGB:
		return 3;
	default:
		return 4;
	}
}

vita2d_texture *vita2d_create_empty_texture_format(unsigned int w, unsigned int h, SceGxmTextureFormat format) {
	vita2d_texture *r = (vita2d_texture *)vglMalloc(sizeof(vita2d_texture));
	glGenTextures(1, &r->tex_id);
	glBindTexture(GL_TEXTURE_2D, r->tex_id);
	switch (format) {
	case SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
		break;
	case SCE_GXM_TEXTURE_FORMAT_U8U8U8_RGB:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGR, w, h, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
		break;
	case SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		break;
	case SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_RGBA:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ABGR_EXT, w, h, 0, GL_ABGR_EXT, GL_UNSIGNED_BYTE, NULL);
		break;
	case SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_RGBA:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, NULL);
		break;
	case SCE_GXM_TEXTURE_FORMAT_U5U5U5U1_RGBA:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, NULL);
		break;
	case SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		break;
	case SCE_GXM_TEXTURE_FORMAT_U8_R:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
		SceGxmTexture *gxm_tex = vglGetGxmTexture(GL_TEXTURE_2D);
		sceGxmTextureSetFormat(gxm_tex, SCE_GXM_TEXTURE_FORMAT_U8_R111);
		break;
	default:
		printf("Invalid format on vita2d_create_empty_texture_format: 0x%08X\n", format);
		break;
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	r->filters[0] = r->filters[1] = SCE_GXM_TEXTURE_FILTER_POINT;
	r->w = w;
	r->h = h;
	r->format = format;
	
	return r;
}

vita2d_texture *vita2d_create_empty_texture(unsigned int w, unsigned int h) {
	return vita2d_create_empty_texture_format(w, h, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR);
}

vita2d_texture *vita2d_create_empty_texture_rendertarget(unsigned int w, unsigned int h, SceGxmTextureFormat format) {
	vita2d_texture *r = vita2d_create_empty_texture_format(w, h, format);
	glGenFramebuffers(1, &r->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, r->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->tex_id, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, v2d_curr_fbo);
	return r;
}

void vita2d_free_texture(vita2d_texture *texture) {
	glDeleteFramebuffers(1, &texture->fbo);
	glDeleteTextures(1, &texture->tex_id);
	vglFree(texture);
}

unsigned int vita2d_texture_get_width(const vita2d_texture *texture) {
	return texture->w;
}

unsigned int vita2d_texture_get_height(const vita2d_texture *texture) {
	return texture->h;
}

unsigned int vita2d_texture_get_stride(const vita2d_texture *texture) {
	return ALIGN(texture->w, 8) * bpp_from_format(texture->format);
}

SceGxmTextureFormat vita2d_texture_get_format(const vita2d_texture *texture) {
	return texture->format;
}

void *vita2d_texture_get_datap(const vita2d_texture *texture) {
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	return vglGetTexDataPointer(GL_TEXTURE_2D);
}

SceGxmTextureFilter vita2d_texture_get_min_filter(const vita2d_texture *texture) {
	return texture->filters[0];
}

SceGxmTextureFilter vita2d_texture_get_mag_filter(const vita2d_texture *texture) {
	return texture->filters[1];
}

void vita2d_texture_set_filters(vita2d_texture *texture, SceGxmTextureFilter min_filter, SceGxmTextureFilter mag_filter) {
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter == SCE_GXM_TEXTURE_FILTER_POINT ? GL_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter == SCE_GXM_TEXTURE_FILTER_POINT ? GL_NEAREST : GL_LINEAR);
}

void vita2d_draw_texture_tint(const vita2d_texture *texture, float x, float y, unsigned int color) {
	GLfloat vtx[8] = {
		             x,              y,
		x + texture->w,              y,
		             x, y + texture->h,
		x + texture->w, y + texture->h
	};
	GLfloat tcoord[8] = {
		0, 0,
		1, 0,
		0, 1,
		1, 1
	};
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void vita2d_draw_texture(const vita2d_texture *texture, float x, float y) {
	vita2d_draw_texture_tint(texture, x, y, 0xFFFFFFFF);
}

void vita2d_draw_texture_tint_scale(const vita2d_texture *texture, float x, float y, float x_scale, float y_scale, unsigned int color) {
	GLfloat w = x + (texture->w * x_scale);
	GLfloat h = y + (texture->h * y_scale);
	GLfloat vtx[8] = {
		x, y,
		w, y,
		x, h,
		w, h
	};
	GLfloat tcoord[8] = {
		0, 0,
		1, 0,
		0, 1,
		1, 1
	};
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void vita2d_draw_texture_scale(const vita2d_texture *texture, float x, float y, float x_scale, float y_scale) {
	vita2d_draw_texture_tint_scale(texture, x, y, x_scale, y_scale, 0xFFFFFFFF);
}

void vita2d_draw_texture_tint_rotate_hotspot(const vita2d_texture *texture, float x, float y, float rad, float center_x, float center_y, unsigned int color) {
	float c = cosf(rad);
	float s = sinf(rad);
	
	GLfloat vtx[8] = {
		             - center_x,              - center_y,
		- center_x + texture->w,              - center_y,
		             - center_x, - center_y + texture->h,
		- center_x + texture->w, - center_y + texture->h,
	};
	
	for (int i = 0; i < 4; i++) { // Rotate and translate
		float _x = vtx[i*2];
		float _y = vtx[i*2+1];
		vtx[i*2] = _x*c - _y*s + x;
		vtx[i*2+1] = _x*s + _y*c + y;
	}
	
	GLfloat tcoord[8] = {
		0, 0,
		1, 0,
		0, 1,
		1, 1
	};
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void vita2d_draw_texture_rotate_hotspot(const vita2d_texture *texture, float x, float y, float rad, float center_x, float center_y) {
	vita2d_draw_texture_tint_rotate_hotspot(texture, x, y, rad, center_x, center_y, 0xFFFFFFFF);
}

void vita2d_draw_texture_tint_rotate(const vita2d_texture *texture, float x, float y, float rad, unsigned int color) {
	vita2d_draw_texture_tint_rotate_hotspot(texture, x, y, rad, texture->w / 2, texture->h / 2, color);
}

void vita2d_draw_texture_rotate(const vita2d_texture *texture, float x, float y, float rad) {
	vita2d_draw_texture_tint_rotate(texture, x, y, rad, 0xFFFFFFFF);
}

void vita2d_draw_texture_tint_part(const vita2d_texture *texture, float x, float y, float tex_x, float tex_y, float tex_w, float tex_h, unsigned int color) {
	GLfloat w = x + tex_w;
	GLfloat h = y + tex_h;
	GLfloat vtx[8] = {
		x, y,
		w, y,
		x, h,
		w, h
	};
	GLfloat tx = tex_x / (float)texture->w;
	GLfloat ty = tex_y / (float)texture->h;
	GLfloat tw = (tex_x + tex_w) / (float)texture->w;
	GLfloat th = (tex_y + tex_h) / (float)texture->h;
	GLfloat tcoord[8] = {
		tx, ty,
		tx, ty,
		tx, th,
		tw, th
	};
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void vita2d_draw_texture_part(const vita2d_texture *texture, float x, float y, float tex_x, float tex_y, float tex_w, float tex_h) {
	vita2d_draw_texture_tint_part(texture, x, y, tex_x, tex_y, tex_w, tex_h, 0xFFFFFFFF);
}

void vita2d_draw_texture_tint_part_scale(const vita2d_texture *texture, float x, float y, float tex_x, float tex_y, float tex_w, float tex_h, float x_scale, float y_scale, unsigned int color) {
	GLfloat w = x + (tex_w * x_scale);
	GLfloat h = y + (tex_h * y_scale);
	GLfloat vtx[8] = {
		x, y,
		w, y,
		x, h,
		w, h
	};
	GLfloat tx = tex_x / (float)texture->w;
	GLfloat ty = tex_y / (float)texture->h;
	GLfloat tw = (tex_x + tex_w) / (float)texture->w;
	GLfloat th = (tex_y + tex_h) / (float)texture->h;
	GLfloat tcoord[8] = {
		tx, ty,
		tw, ty,
		tx, th,
		tw, th
	};
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void vita2d_draw_texture_part_scale(const vita2d_texture *texture, float x, float y, float tex_x, float tex_y, float tex_w, float tex_h, float x_scale, float y_scale) {
	vita2d_draw_texture_tint_part_scale(texture, x, y, tex_x, tex_y, tex_w, tex_h, x_scale, y_scale, 0xFFFFFFFF);
}

void vita2d_draw_texture_tint_scale_rotate_hotspot(const vita2d_texture *texture, float x, float y, float x_scale, float y_scale, float rad, float center_x, float center_y, unsigned int color) {
	GLfloat w = (texture->w * x_scale);
	GLfloat h = (texture->h * y_scale);
	center_x *= x_scale;
	center_y *= y_scale;
	
	GLfloat vtx[8] = {
		    - center_x,     - center_y,
		- center_x + w,     - center_y,
		    - center_x, - center_y + h,
		- center_x + w, - center_y + h,
	};
	GLfloat tcoord[8] = {
		0, 0,
		1, 0,
		0, 1,
		1, 1
	};
	
	float c = cosf(rad);
	float s = sinf(rad);
	for (int i = 0; i < 4; i++) { // Rotate and translate
		float _x = vtx[i*2];
		float _y = vtx[i*2+1];
		vtx[i*2] = _x*c - _y*s + x;
		vtx[i*2+1] = _x*s + _y*c + y;
	}
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void vita2d_draw_texture_scale_rotate_hotspot(const vita2d_texture *texture, float x, float y, float x_scale, float y_scale, float rad, float center_x, float center_y) {
	vita2d_draw_texture_tint_scale_rotate_hotspot(texture, x, y, x_scale, y_scale, rad, center_x, center_y, 0xFFFFFFFF);
}

void vita2d_draw_texture_tint_scale_rotate(const vita2d_texture *texture, float x, float y, float x_scale, float y_scale, float rad, unsigned int color) {
	vita2d_draw_texture_tint_scale_rotate_hotspot(texture, x, y, x_scale, y_scale, rad, texture->w / 2, texture->h / 2, color);
}

void vita2d_draw_texture_scale_rotate(const vita2d_texture *texture, float x, float y, float x_scale, float y_scale, float rad) {
	vita2d_draw_texture_tint_scale_rotate_hotspot(texture, x, y, x_scale, y_scale, rad, texture->w / 2, texture->h / 2, 0xFFFFFFFF);
}

void vita2d_draw_texture_part_tint_scale_rotate(const vita2d_texture *texture, float x, float y, float tex_x, float tex_y, float tex_w, float tex_h, float x_scale, float y_scale, float rad, unsigned int color) {
	GLfloat center_x = (tex_w * x_scale) / 2;
	GLfloat center_y = (tex_h * y_scale) / 2;
	
	GLfloat vtx[8] = {
		    - center_x, - center_y,
		      center_x, - center_y,
		    - center_x,   center_y,
		      center_x,   center_y,
	};
	GLfloat tx = tex_x / (float)texture->w;
	GLfloat ty = tex_y / (float)texture->h;
	GLfloat tw = (tex_x + tex_w) / (float)texture->w;
	GLfloat th = (tex_y + tex_h) / (float)texture->h;
	GLfloat tcoord[8] = {
		tx, ty,
		tx, ty,
		tx, th,
		tw, th
	};
	
	float c = cosf(rad);
	float s = sinf(rad);
	for (int i = 0; i < 4; i++) { // Rotate and translate
		float _x = vtx[i*2];
		float _y = vtx[i*2+1];
		vtx[i*2] = _x*c - _y*s + x;
		vtx[i*2+1] = _x*s + _y*c + y;
	}
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	glColor4ubv((GLubyte *)&color);
	glVertexPointer(2, GL_FLOAT, 0, vtx);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tcoord);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void vita2d_draw_texture_part_scale_rotate(const vita2d_texture *texture, float x, float y, float tex_x, float tex_y, float tex_w, float tex_h, float x_scale, float y_scale, float rad) {
	vita2d_draw_texture_part_tint_scale_rotate(texture, x, y, tex_x, tex_y, tex_w, tex_h, x_scale, y_scale, rad, 0xFFFFFFFF);
}

vita2d_texture *vita2d_load_PNG_file(const char *filename) {
	int w, h;
	uint32_t *data = (uint32_t *)stbi_load(filename, &w, &h, NULL, 4);
	if (!data)
		return NULL;

	vita2d_texture *r = (vita2d_texture *)vglMalloc(sizeof(vita2d_texture));
	glGenTextures(1, &r->tex_id);
	glBindTexture(GL_TEXTURE_2D, r->tex_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	vglFree(data);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	r->filters[0] = r->filters[1] = SCE_GXM_TEXTURE_FILTER_POINT;
	r->w = w;
	r->h = h;
	r->format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
	
	return r;
}

vita2d_texture *vita2d_load_JPEG_file(const char *filename) {
	return vita2d_load_PNG_file(filename);
}

vita2d_texture *vita2d_load_BMP_file(const char *filename) {
	return vita2d_load_PNG_file(filename);
}

vita2d_texture *vita2d_load_PNG_buffer(const void *buffer, unsigned long buffer_size) {
	int w, h;
	uint32_t *data = (uint32_t *)stbi_load_from_memory(buffer, buffer_size, &w, &h, NULL, 4);
	if (!data)
		return NULL;

	vita2d_texture *r = (vita2d_texture *)vglMalloc(sizeof(vita2d_texture));
	glGenTextures(1, &r->tex_id);
	glBindTexture(GL_TEXTURE_2D, r->tex_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	vglFree(data);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	r->filters[0] = r->filters[1] = SCE_GXM_TEXTURE_FILTER_POINT;
	r->w = w;
	r->h = h;
	r->format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
	
	return r;
}

vita2d_texture *vita2d_load_JPEG_buffer(const void *buffer, unsigned long buffer_size) {
	return vita2d_load_PNG_buffer(buffer, buffer_size);
}

vita2d_texture *vita2d_load_BMP_buffer(const void *buffer, unsigned long buffer_size) {
	return vita2d_load_PNG_buffer(buffer, buffer_size);
}