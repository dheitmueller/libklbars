#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "libklbars/klbars.h"
#include "klbars-internal.h"

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
