#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "libklbars/klbars.h"
#include "klbars-internal.h"

/* See SMPTE RP-219-1-2014 for details of how these bars
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

/* Draws a gradient from y0 to y1 */
static int draw_grad10(struct kl_colorbar_context *ctx, uint32_t row_num,
		       uint32_t bar_width, uint32_t pixel_offset, 
		       uint16_t y0, uint16_t y1, uint16_t cb, uint16_t cr)
{
	uint8_t *rowPtr;
	int bar_width_pixels;
	int range = y1 - y0;

	rowPtr = ctx->frame + (ctx->stride * row_num);
	bar_width_pixels = bar_width * 16 / 6;
	pixel_offset = pixel_offset - (pixel_offset % 16);

	float step = (float) range / (float)bar_width;
	float y0_f = y0;

	for (int i = 0; i < bar_width_pixels; i += 16) {
		uint8_t *bar10 = &rowPtr[pixel_offset + i];
		bar10[0] = cb & 0xff;
		bar10[1] = (cb >> 8) | ((y0 & 0x3f) << 2);
		bar10[2] = (y0 >> 6) | ((cr & 0x0f) << 4);
		bar10[3] = (cr >> 4);
		y0_f += step; y0 = y0_f;

		bar10[4] = y0 & 0xff;
		bar10[5] = (y0 >> 8) | ((cb & 0x3f) << 2);
		y0_f += step; y0 = y0_f;
		bar10[6] = (cb >> 6) | ((y0 & 0x0f) << 4);
		bar10[7] = (y0 >> 4);
		y0_f += step; y0 = y0_f;

		bar10[8] = cr & 0xff;
		bar10[9] = (cr >> 8) | ((y0 & 0x3f) << 2);
		bar10[10] = (y0 >> 6) | ((cb & 0x0f) << 4);
		bar10[11] = (cb >> 4);
		y0_f += step; y0 = y0_f;

		bar10[12] = y0 & 0xff;
		bar10[13] = (y0 >> 8) | ((cr & 0x3f) << 2);
		y0_f += step; y0 = y0_f;
		bar10[14] = (cr >> 6) | ((y0 & 0x0f) << 4);
		bar10[15] = (y0 >> 4);
		y0_f += step; y0 = y0_f;
	}
#if 0
	printf("y0_f at end was %f.  y0 was %d\n", y0_f, y0);
#endif
	return bar_width_pixels;
}

static int draw_grad8(struct kl_colorbar_context *ctx, uint32_t row_num,
		      uint32_t bar_width, uint32_t pixel_offset, 
		      uint16_t y0, uint16_t y1, uint16_t cb, uint16_t cr)
{
	uint8_t *rowPtr;
	int bar_width_pixels;
	int range;
	
	y0 >>= 2;
	y1 >>= 2;
	cb >>= 2;
	cr >>= 2;

	range = y1 - y0;
	rowPtr = ctx->frame + (ctx->stride * row_num);
	bar_width_pixels = bar_width * 2;
	pixel_offset = pixel_offset - (pixel_offset % 4);

	float step = (float) range / (float)bar_width;
	float y0_f = y0;
	for (int i = 0; i < bar_width_pixels; i += 4) {
		rowPtr[pixel_offset + i] = cb;
		rowPtr[pixel_offset + i + 1] = y0;
		y0_f += step; y0 = y0_f;
		rowPtr[pixel_offset + i + 2] = cr;
		rowPtr[pixel_offset + i + 3] = y0;
		y0_f += step; y0 = y0_f;
	}
#if 0
	printf("y0_f at end was %f.  y0 was %d\n", y0_f, y0);
#endif
	return bar_width_pixels;
}

static int draw_grad(struct kl_colorbar_context *ctx, uint32_t row_num,
		     uint32_t bar_width, uint32_t pixel_offset, 
		     uint16_t y0, uint16_t y1, uint16_t cb, uint16_t cr)
{
	if (ctx->colorspace == KL_COLORBAR_8BIT)
		return draw_grad8(ctx, row_num, bar_width, pixel_offset,
				  y0, y1, cb, cr);
	else
		return draw_grad10(ctx, row_num, bar_width, pixel_offset,
				   y0, y1, cb, cr);
}

/* See SMPTE RP 219-1-2014 Sec 4.3.1 */
static void gen_pattern_1(struct kl_colorbar_context *ctx, uint32_t row_num)
{
	int pixel_offset;

	/* Pattern 1:
	   1/8: 40% Gray
	   3/4 x 1/7 75% White
	   3/4 x 1/7 75% Yellow
	   3/4 x 1/7 75% Cyan
	   3/4 x 1/7 75% Green
	   3/4 x 1/7 75% Magenta
	   3/4 x 1/7 75% Red
	   3/4 x 1/7 75% Blue
	   1/8: 40% Gray
	*/

	pixel_offset = 0;

	/* 40% Grey */
	pixel_offset += draw_bar(ctx, row_num, ctx->width / 8,
				 pixel_offset, 414, 512, 512);

	/* 75% white */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 721, 512, 512);

	/* 75% Yellow */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 674, 176, 543);

	/* 75% Cyan */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 581, 589, 176);

	/* 75% Green */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 534, 253, 207);

	/* 75% Magenta */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 251, 771, 817);

	/* 75% Red */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 204, 435, 848);

	/* 75% Blue */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 111, 848, 481);

	/* 40% Grey */
	draw_bar(ctx, row_num, ctx->width / 8,
				 pixel_offset, 414, 512, 512);
}

/* See SMPTE RP 219-1-2014 Sec 4.3.2.
   Note: There are multiple choices for this pattern.  We implement
   "Option B" as defined in the spec: "Pattern 2 waveforms with 100%
   white signal (in *2 sub-pattern)". */
static void gen_pattern_2(struct kl_colorbar_context *ctx, uint32_t row_num)
{
	int pixel_offset;

	/* Pattern 2 (option B):
	   1/8: 100% Cyan
	   3/4 x 1/7 100% White
	   3/4 x 6/7 75% White
	   1/8: 100% Blue
	*/

	pixel_offset = 0;

	/* 100% Cyan */
	pixel_offset += draw_bar(ctx, row_num, ctx->width / 8,
				 pixel_offset, 754, 615, 64);

	/* 100% white */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 940, 512, 512);

	/* 75% White */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 * 6/7,
				 pixel_offset, 721, 512, 512);
	/* 100% Blue */
	draw_bar(ctx, row_num, ctx->width / 8,
				 pixel_offset, 127, 960, 471);
}

/* See SMPTE RP 219-1-2014 Sec 4.3.3.
   Note: There are multiple choices for this pattern.  We implement
   "Option A" as defined in the spec: "Sub-pattern *3 set to black
   signal". */
static void gen_pattern_3(struct kl_colorbar_context *ctx, uint32_t row_num)
{
	int pixel_offset;

	/* Pattern 3 (option A):
	   1/8: 100% Yellow
	   3/4 x 1/7 0% Black
	   3/4 x 5/7 Y-Ramp
	   3/4 x 1/7 100% White
	   1/8: 100% Red
	*/

	pixel_offset = 0;

	/* 100% Yellow */
	pixel_offset += draw_bar(ctx, row_num, ctx->width / 8,
				 pixel_offset, 877, 64, 553);

	/* 0% Black */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 64, 512, 512);

	/* Y Ramp */
	pixel_offset += draw_grad(ctx, row_num, ctx->width * 3/4 * 5/7,
				  pixel_offset, 64, 940, 512, 512);

	/* 100% White */
	pixel_offset += draw_bar(ctx, row_num, ctx->width * 3/4 / 7,
				 pixel_offset, 940, 512, 512);

	/* 100% Red */
	draw_bar(ctx, row_num, ctx->width / 8,
				 pixel_offset, 250, 409, 960);
}

/* See SMPTE RP 219-1-2014 Sec 4.3.4.
   Note: There are multiple choices for this pattern.  We implement
   "Option A" as defined in the spec: "Sub-pattern *5 set to black
   signal and Sub-pattern *6 set to white signal". */
static void gen_pattern_4(struct kl_colorbar_context *ctx, uint32_t row_num)
{
	int pixel_offset;

	/* Pattern 4 (option A):
	   c = 3/4a * 1/7
	   1/8a: 15% Gray
	   3/2c: 0% Black
	   2c: 100% white
	   5/6c: 0% Black
	   1/3c: -2% Black
	   1/3c: 0% Black
	   1/3c: 2% Black
	   1/3c: 0% Black
	   1/3c: 4% Black
	   1/8a: 15% Gray
	*/

	pixel_offset = 0;

	int c = ctx->width * 3/4 / 7;

	/* 15% Gray */
	pixel_offset += draw_bar(ctx, row_num, ctx->width / 8,
				 pixel_offset, 195, 512, 512);

	/* 0% Black */
	pixel_offset += draw_bar(ctx, row_num, c * 3 / 2,
				 pixel_offset, 64, 512, 512);

	/* 100% White */
	pixel_offset += draw_bar(ctx, row_num, c * 2,
				 pixel_offset, 940, 512, 512);

	/* 0% Black */
	pixel_offset += draw_bar(ctx, row_num, c * 5 / 6,
				 pixel_offset, 64, 512, 512);

	/* Pluge */
	pixel_offset += draw_bar(ctx, row_num, c / 3,
				 pixel_offset, 46, 512, 512);
	pixel_offset += draw_bar(ctx, row_num, c / 3,
				 pixel_offset, 64, 512, 512);
	pixel_offset += draw_bar(ctx, row_num, c / 3,
				 pixel_offset, 82, 512, 512);
	pixel_offset += draw_bar(ctx, row_num, c / 3,
				 pixel_offset, 64, 512, 512);
	pixel_offset += draw_bar(ctx, row_num, c / 3,
				 pixel_offset, 99, 512, 512);

	/* 0% Black */
	pixel_offset += draw_bar(ctx, row_num, c,
				 pixel_offset, 64, 512, 512);

	/* 15% Gray */
	draw_bar(ctx, row_num, ctx->width / 8,
				 pixel_offset, 195, 512, 512);
}

void kl_colorbar_fill_rp219_1(struct kl_colorbar_context *ctx)
{
	if (!ctx)
		return;

	uint32_t y = 0;
	uint32_t rowStride = ctx->stride;
	uint8_t *rowPtr;
	uint8_t *start;
	int endline;

	/* Pattern 1 */
	gen_pattern_1(ctx, y);
	y++;

	/* Clone the first line to the rest of the lines which
	   make up the top 75% of the frame */
	rowPtr = ctx->frame + rowStride * y;
	for (y = 1; y < (ctx->height * 7 / 12); y++) {
		memcpy(rowPtr, ctx->frame, rowStride);
		rowPtr += rowStride;
	}

	/* Pattern 2 */
	gen_pattern_2(ctx, y);
	start = ctx->frame + rowStride * y;
	rowPtr += rowStride;
	y++;

	endline = y + (ctx->height * 1 / 12);
	while (y < endline) {
		memcpy(rowPtr, start, rowStride);
		rowPtr += rowStride;
		y++;
	}

	/* Pattern 3 */
	gen_pattern_3(ctx, y);
	start = ctx->frame + rowStride * y;
	rowPtr += rowStride;
	y++;

	endline = y + (ctx->height * 1 / 12);
	while (y < endline) {
		memcpy(rowPtr, start, rowStride);
		rowPtr += rowStride;
		y++;
	}

	/* Pattern 4 */
	gen_pattern_4(ctx, y);
	start = ctx->frame + rowStride * y;
	rowPtr += rowStride;
	y++;

	endline = ctx->height;
	while (y < endline) {
		memcpy(rowPtr, start, rowStride);
		rowPtr += rowStride;
		y++;
	}
}
