/**
 * @file        klbars.c
 * @author      Devin Heitmueller <dheitmueller@kernellabs.com>
 * @copyright   Copyright (c) 2016-2017 Kernel Labs Inc. All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "libklbars/klbars.h"
#include "font8x8_basic.h"

#define ALPHA_BACKGROUND 0

int kl_colorbar_init(struct kl_colorbar_context *ctx, unsigned int width,
		     unsigned int height, int bitDepth)
{
	if (!ctx)
		return -1;

	memset(ctx, 0, sizeof(*ctx));

	ctx->width = width;
	ctx->height = height;
	ctx->colorspace = bitDepth;
	if (bitDepth == KL_COLORBAR_10BIT) {
		/* V210 stride required by Blackmagic Decklink */
		ctx->stride = ((width + 47) / 48) * 128;
	} else {
		ctx->stride = width * 2;
	}
	ctx->frame = malloc(height * ctx->stride);
	if (ctx->frame == NULL)
		return -1;

	kl_colorbar_render_reset(ctx);

	return 0;
}

int kl_colorbar_finalize(struct kl_colorbar_context *ctx, unsigned char *buf,
			 int targetColorspace, unsigned int byteStride)
{
	if ((!ctx) || (!buf) || (byteStride == 0))
		return -1;

	if (ctx->colorspace == KL_COLORBAR_8BIT) {
		if (targetColorspace == KL_COLORBAR_8BIT){
			/* Just a straight memcpy() */
			for (int y = 0; y < ctx->height; y++) {
				memcpy(buf, ctx->frame + (y * ctx->stride),
				       ctx->width * 2);
				buf += byteStride;
			}
		} else {
			/* Convert 8-bit to 10-bit */
			for (int y = 0; y < ctx->height; y++) {
				/* Note, we're simultaneously converting 8-bit to 10-bit
				 *AND* repacking to 10-bit in the same operation, which
				 is why this is pretty convoluted */

				/* FIXME:  this just begs for some SSE optimization */
				unsigned char *line = ctx->frame + (y * ctx->stride);
				int n = 0;
				int x = 0;
				for (x = 0; x < (ctx->width - 2) * 2; x+= 3) {
					buf[n] = line[x] << 2;
					buf[n+1] = (line[x] >> 6) | (line[x+1] << 4);
					buf[n+2] = (line[x+1] >> 4) | (line [x+2] << 6);
					buf[n+3] = (line[x+2] >> 2);
					n += 4;
				}

				/* Each increment of the above loop processes 1.5
				   pixels on the input buffer, so we need deal with the
				   remainder */
				buf[n] = line[x] << 2;
				buf[n+1] = (line[x] >> 6) | (line[x+1] << 4);
				buf[n+2] = (line[x+1] >> 4);

				buf += byteStride;
			}

		}
	} else {
		/* For now just handle the colorspace in 8-bit, and colorspace convert
		   it to 10-bit on finalize */
		if (targetColorspace == KL_COLORBAR_10BIT){
			/* Just a straight memcpy() */
			for (int y = 0; y < ctx->height; y++) {
				memcpy(buf, ctx->frame + (y * ctx->stride),
				       ctx->width * 16 / 6);
				buf += byteStride;
			}
		} else {
			/* Convert 10-bit to 8-bit */
			int line_width = (ctx->width) * 16 / 6;
			for (int y = 0; y < ctx->height; y++) {
				/* FIXME:  this just begs for some SSE optimization */
				unsigned char *line = ctx->frame + (y * ctx->stride);
				int n = 0;
				int x = 0;
                                for (x = 0; x < (line_width - 4); x+= 4) {
					uint32_t *valptr = (uint32_t *)&line[x];
					uint32_t val = *valptr;
					buf[n++] = val >> 2;
					buf[n++] = val >> 12;
					buf[n++] = val >> 22;
				}
				buf += byteStride;
			}
		}
	}
	return 0;
}

void kl_colorbar_free(struct kl_colorbar_context *ctx)
{
	if (!ctx)
		return;

	free(ctx->frame);
}

// SD 75% Colour Bars
static uint32_t gSD75pcColourBars[7] =
{
	0xb480b480, 0xa28ea22c, 0x832c839c, 0x703a7048,
	0x54c654b8, 0x41d44164, 0x237223d4
};

// HD 75% Colour Bars
static uint32_t gHD75pcColourBars[7] =
{
	0xb480b480, 0xa888a82c, 0x912c9193, 0x8634863f,
	0x3fcc3fc1, 0x33d4336d, 0x1c781cd4
};

/* Intended to conform to EIA-189-A */
static void kl_colorbar_fill_colorbars_8bit(struct kl_colorbar_context *ctx)
{
	if (!ctx)
		return;

	uint32_t *nextWord = (uint32_t *) ctx->frame;
	uint32_t *bars;
	uint32_t y = 0;
	uint32_t rowStride = ctx->width * 2;
	uint8_t *rowPtr;

	if (ctx->width > 720)
		bars = gHD75pcColourBars;
	else
		bars = gSD75pcColourBars;

	/* Vertical color bars for top 75% of field */
	for (uint32_t x = 0; x < ctx->width; x+=2)
		*(nextWord++) = bars[(x * 7) / ctx->width];
	y++;

	rowPtr = ctx->frame + rowStride * y;
	for (y = 1; y < (ctx->height * 3 / 4); y++) {
		memcpy(rowPtr, ctx->frame, rowStride);
		rowPtr += rowStride;
	}

	/* Generate the first row for the last 25% */
	uint32_t *bottom = (uint32_t *)(ctx->frame + rowStride * y);
	nextWord = bottom;
	uint32_t x = 0;
	int b_width = ((ctx->width / 7) * 5 / 4);
	/* -I */
	while (x < b_width) {
		*(nextWord++) = 0x105f109e;
		x += 2;
	}

	/* White */
	while (x < b_width * 2) {
		*(nextWord++) = 0xeb80eb80;
		x += 2;
	}

	/* -Q */
	while (x < b_width * 3) {
		*(nextWord++) = 0x109410ad;
		x += 2;
	}

	/* Black */
	while (x < ctx->width) {
		*(nextWord++) = 0x10801080;
		x += 2;
	}
	y++;

	/* Now fill the rest of the rows for the last 25% */
	rowPtr = ctx->frame + rowStride * y;
	while (y < ctx->height) {
		memcpy(rowPtr, bottom, rowStride);
		rowPtr += rowStride;
		y++;
	}
}

static void compute_colorbar_10bit_array(const uint32_t uyvy, uint8_t *bar10)
{
	uint8_t *bar8 = (uint8_t *)&uyvy;
	uint16_t cb, y0, cr;

	cb = bar8[0] << 2;
	y0 = bar8[1] << 2;
	cr = bar8[2] << 2;

	bar10[0] = cb & 0xff;
	bar10[1] = (cb >> 8) | ((y0 & 0x3f) << 2);
	bar10[2] = (y0 >> 6) | ((cr & 0x0f) << 4);
	bar10[3] = (cr >> 4);

	bar10[4] = y0 & 0xff;
	bar10[5] = (y0 >> 8) | ((cb & 0x3f) << 2);
	bar10[6] = (cb >> 6) | ((y0 & 0x0f) << 4);
	bar10[7] = (y0 >> 4);

	bar10[8] = cr & 0xff;
	bar10[9] = (cr >> 8) | ((y0 & 0x3f) << 2);
	bar10[10] = (y0 >> 6) | ((cb & 0x0f) << 4);
	bar10[11] = (cb >> 4);

	bar10[12] = y0 & 0xff;
	bar10[13] = (y0 >> 8) | ((cr & 0x3f) << 2);
	bar10[14] = (cr >> 6) | ((y0 & 0x0f) << 4);
	bar10[15] = (y0 >> 4);
}

static void kl_colorbar_fill_colorbars_10bit(struct kl_colorbar_context *ctx)
{
	if (!ctx)
		return;

	uint32_t *bars;
	uint32_t y = 0;
	uint32_t rowStride = ctx->stride;
	uint8_t *rowPtr;
	int bar_width;
	int bar_width_pixels;
	int pixel_offset;

	if (ctx->width > 720)
		bars = gHD75pcColourBars;
	else
		bars = gSD75pcColourBars;

	/* Colorspace convert the actual bar values and repack */
	rowPtr = ctx->frame;
	uint8_t bar10[16];

	/* Generate first line of colorbars, which will be copied
	   to all the other lines */
	for (int i = 0; i < 7; i++) {
		compute_colorbar_10bit_array(bars[i], &bar10[0]);
		bar_width = (ctx->width / 7);
		bar_width_pixels = bar_width * 16 / 6;
		pixel_offset = (bar_width * i) * 16 / 6;
		pixel_offset = pixel_offset - (pixel_offset % 16);
		for (uint32_t x = 0; x < bar_width_pixels; x+= 16) {
			for (int n = 0; n < 16; n++)
				rowPtr[pixel_offset + x + n] = bar10[n];
		}
	}
	y++;

	/* Clone the first line to the rest of the lines which
	   make up the top 75% of the frame */
	rowPtr = ctx->frame + rowStride * y;
	for (y = 1; y < (ctx->height * 3 / 4); y++) {
		memcpy(rowPtr, ctx->frame, rowStride);
		rowPtr += rowStride;
	}


	/* Generate the first row for the last 25% */
	uint32_t *bottom = (uint32_t *)(ctx->frame + rowStride * y);
	bar_width = ((ctx->width / 7) * 5 / 4);
	bar_width_pixels = bar_width * 16 / 6;

	/* -I */
	compute_colorbar_10bit_array(0x105f109e, &bar10[0]);
	pixel_offset = 0;
	for (uint32_t x = 0; x < bar_width_pixels; x+= 16) {
		for (int n = 0; n < 16; n++)
			rowPtr[pixel_offset + x + n] = bar10[n];
	}

	/* White */
	compute_colorbar_10bit_array(0xeb80eb80, &bar10[0]);
	pixel_offset = (bar_width * 1) * 16 / 6;
	pixel_offset = pixel_offset - (pixel_offset % 16);
	for (uint32_t x = 0; x < bar_width_pixels; x+= 16) {
		for (int n = 0; n < 16; n++)
			rowPtr[pixel_offset + x + n] = bar10[n];
	}

	/* -Q */
	compute_colorbar_10bit_array(0x109410ad, &bar10[0]);
	pixel_offset = (bar_width * 2) * 16 / 6;
	pixel_offset = pixel_offset - (pixel_offset % 16);
	for (uint32_t x = 0; x < bar_width_pixels; x+= 16) {
		for (int n = 0; n < 16; n++)
			rowPtr[pixel_offset + x + n] = bar10[n];
	}

	/* Black */
	compute_colorbar_10bit_array(0x10801080, &bar10[0]);
	pixel_offset = (bar_width * 3) * 16 / 6;
	pixel_offset = pixel_offset - (pixel_offset % 16);
	for (uint32_t x = pixel_offset; x < ctx->width * 8 / 3; x+= 16) {
		for (int n = 0; n < 16; n++)
			rowPtr[x + n] = bar10[n];
	}

	/* Now fill the rest of the rows for the last 25% */
	rowPtr = ctx->frame + rowStride * y;
	while (y < ctx->height) {
		memcpy(rowPtr, bottom, rowStride);
		rowPtr += rowStride;
		y++;
	}
}

void kl_colorbar_fill_colorbars(struct kl_colorbar_context *ctx)
{
	if (ctx->colorspace == KL_COLORBAR_8BIT)
		kl_colorbar_fill_colorbars_8bit(ctx);
	else
		kl_colorbar_fill_colorbars_10bit(ctx);
}

void kl_colorbar_fill_black_8bit(struct kl_colorbar_context *ctx)
{
	if (!ctx)
		return;

	uint32_t *nextWord = (uint32_t *) ctx->frame;

	long wordsRemaining = (ctx->width * 2 * ctx->height) / 4;

	while (wordsRemaining-- > 0)
		*(nextWord++) = 0x10801080;
}

void kl_colorbar_fill_black_10bit(struct kl_colorbar_context *ctx)
{
	uint32_t y = 0;
	uint32_t rowStride = ctx->stride;
	uint8_t *rowPtr;
	uint8_t bar10[16];

	if (!ctx)
		return;

	rowPtr = ctx->frame;

	/* Black */
	compute_colorbar_10bit_array(0x10801080, &bar10[0]);
	for (uint32_t x = 0; x < ctx->width * 8 / 3; x+= 16) {
		for (int n = 0; n < 16; n++)
			rowPtr[x + n] = bar10[n];
	}
	y++;

	rowPtr = ctx->frame + rowStride * y;
	for (y = 1; y < ctx->height; y++) {
		memcpy(rowPtr, ctx->frame, rowStride);
		rowPtr += rowStride;
	}
}

void kl_colorbar_fill_black(struct kl_colorbar_context *ctx)
{
	if (ctx->colorspace == KL_COLORBAR_8BIT)
		kl_colorbar_fill_black_8bit(ctx);
	else
		kl_colorbar_fill_black_10bit(ctx);
}

static int kl_colorbar_render_moveto(struct kl_colorbar_context *ctx, int x, int y)
{
	if (ctx->colorspace == KL_COLORBAR_10BIT)
		ctx->ptr = ctx->frame + (x * (ctx->plotwidth * 2 * 2));
	else
		ctx->ptr = ctx->frame + (x * (ctx->plotwidth * 2));
	ctx->ptr += ((y * (ctx->stride)) * ctx->plotheight);
    
	ctx->currx = x;
	ctx->curry = y;
    
	/* Black */
	ctx->bg[0] = 0x80;
	ctx->bg[1] = 0x00;
    
	/* Slight off white, closer to grey */
	ctx->fg[0] = 0x80;
	ctx->fg[1] = 0x90;
    
	return 0;
}

static int kl_colorbar_render_character_8bit(struct kl_colorbar_context *ctx, uint8_t letter)
{
	uint8_t line;
    
	if (letter > 0x9f)
		return -1;
    
	for (int i = 0; i < 8; i++) {
		int k = 0;
		while (k++ < 4) {
			line = font8x8_basic[letter][ i ];
			for (int j = 0; j < 8; j++) {
				if (line & 0x01) {
					/* font color */
					*(ctx->ptr + 0) = ctx->fg[0];
					*(ctx->ptr + 1) = ctx->fg[1];
					*(ctx->ptr + 2) = ctx->fg[0];
					*(ctx->ptr + 3) = ctx->fg[1];
					if (ctx->plotctrl == 8) {
						*(ctx->ptr + 4) = ctx->fg[0];
						*(ctx->ptr + 5) = ctx->fg[1];
						*(ctx->ptr + 6) = ctx->fg[0];
						*(ctx->ptr + 7) = ctx->fg[1];
					}
				} else {
					/* background color */
#if ALPHA_BACKGROUND
					/* Minor alpha */
					*(ctx->ptr + 0) >>= 1;
					*(ctx->ptr + 2) >>= 1;
					*(ctx->ptr + 4) >>= 1;
					*(ctx->ptr + 6) >>= 1;
#else
					/* Complete black background */
					*(ctx->ptr + 0) = ctx->bg[0];
					*(ctx->ptr + 1) = ctx->bg[1];
					*(ctx->ptr + 2) = ctx->bg[0];
					*(ctx->ptr + 3) = ctx->bg[1];
					if (ctx->plotctrl == 8) {
						*(ctx->ptr + 4) = ctx->bg[0];
						*(ctx->ptr + 5) = ctx->bg[1];
						*(ctx->ptr + 6) = ctx->bg[0];
						*(ctx->ptr + 7) = ctx->bg[1];
					}
#endif
				}
                
				ctx->ptr += ctx->plotctrl;
				line >>= 1;
			}
			ctx->ptr += ((ctx->stride) - (ctx->plotctrl * 2 * 4));
		}
	}
    
	return 0;
}

static int kl_colorbar_render_character_10bit(struct kl_colorbar_context *ctx, uint8_t letter)
{
	uint8_t line;
	uint8_t bar10_fg[16];
	uint8_t bar10_bg[16];

	if (letter > 0x9f)
		return -1;

	compute_colorbar_10bit_array(ctx->fg[0] | (ctx->fg[1] << 8) |
				     (ctx->fg[0] << 16) | (ctx->fg[1] << 24),
				     &bar10_fg[0]);

	compute_colorbar_10bit_array(ctx->bg[0] | (ctx->bg[1] << 8) |
				     (ctx->bg[0] << 16) | (ctx->bg[1] << 24),
				     &bar10_bg[0]);

	for (int i = 0; i < 8; i++) {
		int k = 0;
		while (k++ < 4) {
			line = font8x8_basic[letter][ i ];
			for (int j = 0; j < 4; j++) {
				/* Hack which takes advantage of the fact that
				   both the FG and BG have the same chroma */
				for (int c=0; c < 2; c++) {
					if (line & 0x01) {
						for (int n = 0; n < 8; n++)
							*(ctx->ptr + n) = bar10_fg[n];
						if (ctx->plotctrl == 8) {
							for (int n = 0; n < 8; n++)
								*(ctx->ptr + 8 + n) = bar10_fg[n];
						}
					} else {
						for (int n = 0; n < 8; n++)
							*(ctx->ptr + n) = bar10_bg[n];
						if (ctx->plotctrl == 8) {
							for (int n = 0; n < 8; n++)
								*(ctx->ptr + 8 + n) = bar10_bg[n];
						}
					}
					line >>= 1;
					ctx->ptr += ctx->plotctrl * 2;
				}
			}
			ctx->ptr += ((ctx->stride) - (ctx->plotctrl * 4 * 4));
		}
	}
	return 0;
}

static int kl_colorbar_render_ascii(struct kl_colorbar_context *ctx, uint8_t letter, int x, int y)
{
	if (letter > 0x9f)
		return -1;
    
	kl_colorbar_render_moveto(ctx, x, y);

	if (ctx->colorspace == KL_COLORBAR_8BIT)
		kl_colorbar_render_character_8bit(ctx, letter);
	else
		kl_colorbar_render_character_10bit(ctx, letter);
    
	return 0;
}

int kl_colorbar_render_string(struct kl_colorbar_context *ctx, char *s, unsigned int len, unsigned int x, unsigned int y)
{
	if ((!ctx) || (!s) || (len == 0) || (len > 128))
		return -1;
    
	for (unsigned int i = 0; i < len; i++)
		kl_colorbar_render_ascii(ctx, *(s + i), x + i, y);
    
	return 0;
}

int kl_colorbar_render_reset(struct kl_colorbar_context *ctx)
{
	if (!ctx)
		return -1;

	ctx->ptr = ctx->frame;

	if (ctx->width < 1280) {
		ctx->plotwidth = 8 * 2;
		ctx->plotheight = 8 * 4;
		ctx->plotctrl = 4;
	} else {
		ctx->plotwidth = 8 * 4;
		ctx->plotheight = 8 * 4;
		ctx->plotctrl = 8;
	}

	kl_colorbar_render_moveto(ctx, 0, 0);

	return 0;
}
