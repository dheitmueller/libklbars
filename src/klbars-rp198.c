#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "libklbars/klbars.h"
#include "klbars-internal.h"

/* See SMPTE RP-219-1998 for details of how these bars
   are arranged */

static int draw_bar10(struct kl_colorbar_context *ctx, uint32_t row_num,
		      uint32_t bar_width, uint32_t pixel_offset, 
		      uint16_t y0, uint16_t pb, uint16_t pr)
{
	uint8_t *rowPtr;
	int bar_width_pixels;
	uint8_t bar10[16];

	rowPtr = ctx->frame + (ctx->stride * row_num);
	compute_colorbar_10bit_array2(y0, pb, pr, &bar10[0]);

	bar_width_pixels = bar_width * 16 / 6;
	pixel_offset = pixel_offset - (pixel_offset % 16);
#if 0
	printf("bar width=%d pixels=%d offset=%d\n", bar_width,
	       bar_width_pixels, pixel_offset);
#endif
	for (uint32_t x = 0; x < bar_width_pixels; x+= 16) {
		for (int n = 0; n < 16; n++)
			rowPtr[pixel_offset + x + n] = bar10[n];
	}
	return bar_width_pixels;
}

static int draw_bar8(struct kl_colorbar_context *ctx, uint32_t row_num,
		     uint32_t bar_width, uint32_t pixel_offset, 
		     uint16_t y0, uint16_t pb, uint16_t pr)
{
	uint8_t *rowPtr;
	int bar_width_pixels;

	rowPtr = ctx->frame + (ctx->stride * row_num);
	bar_width_pixels = bar_width * 2;
	pixel_offset = pixel_offset - (pixel_offset % 4);

	for (uint32_t x = 0; x < bar_width_pixels; x+= 4) {
		rowPtr[pixel_offset + x] = pb >> 2;
		rowPtr[pixel_offset + x + 1] = y0 >> 2;
		rowPtr[pixel_offset + x + 2] = pr >> 2;
		rowPtr[pixel_offset + x + 3] = y0 >> 2;
	}

	return bar_width_pixels;
}

static int draw_bar(struct kl_colorbar_context *ctx, uint32_t row_num,
		     uint32_t bar_width, uint32_t pixel_offset, 
		     uint16_t y0, uint16_t pb, uint16_t pr)
{
	if (ctx->colorspace == KL_COLORBAR_8BIT)
		return draw_bar8(ctx, row_num, bar_width, pixel_offset,
				 y0, pb, pr);
	else
		return draw_bar10(ctx, row_num, bar_width, pixel_offset,
				  y0, pb, pr);
}

/* See SMPTE RP 198-1998 Sec 4 */
static void gen_pattern_1(struct kl_colorbar_context *ctx, uint32_t row_num)
{
	draw_bar(ctx, row_num, ctx->width, 0, 0x198, 0x300, 0x300);
}

/* See SMPTE RP 198-1998 Sec 4 */
static void gen_pattern_2(struct kl_colorbar_context *ctx, uint32_t row_num)
{
	draw_bar(ctx, row_num, ctx->width, 0, 0x110, 0x200, 0x200);
}

void kl_colorbar_fill_rp198(struct kl_colorbar_context *ctx)
{
	if (!ctx)
		return;

	uint32_t y = 0;
	uint32_t rowStride = ctx->stride;
	uint8_t *rowPtr;
	uint8_t *start;
	int endline;

	/* Pattern 1 - Equalizer testing */
	gen_pattern_1(ctx, y);
	y++;

	/* Clone the first line to the rest of the lines which
	   make up the top 75% of the frame */
	rowPtr = ctx->frame + rowStride * y;
	for (y = 1; y < (ctx->height / 2); y++) {
		memcpy(rowPtr, ctx->frame, rowStride);
		rowPtr += rowStride;
	}

	/* Pattern 2 - Phase Locked Loop testing */
	gen_pattern_2(ctx, y);
	start = ctx->frame + rowStride * y;
	rowPtr += rowStride;
	y++;

	endline = ctx->height;
	while (y < endline) {
		memcpy(rowPtr, start, rowStride);
		rowPtr += rowStride;
		y++;
	}

	/* Polarity Control Word */
	if (ctx->pic_count % 2 == 0) {
		/* Change the first Y value from 0x198 to 0x190, but
		   do it in the V210 colorspace */
		ctx->frame[1] &= ~0x20;
	}
}
