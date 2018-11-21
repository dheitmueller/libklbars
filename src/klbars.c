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
#include "klbars-internal.h"

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

	ctx->pic_count++;

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

void compute_colorbar_10bit_array(const uint32_t uyvy, uint8_t *bar10)
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

void compute_colorbar_10bit_array2(uint16_t y0, uint16_t cb, uint16_t cr,
				   uint8_t *bar10)
{
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

int kl_colorbar_fill_pattern (struct kl_colorbar_context *ctx, enum kl_colorbar_pattern pattern)
{
	switch (pattern) {
	case KL_COLORBAR_BLACK:
		kl_colorbar_fill_black(ctx);
		break;
	case KL_COLORBAR_EIA_189A:
		kl_colorbar_fill_colorbars(ctx);
		break;
	case KL_COLORBAR_SMPTE_RP_219_1:
		kl_colorbar_fill_rp219_1(ctx);
		break;
	case KL_COLORBAR_SMPTE_RP_198:
		kl_colorbar_fill_rp198(ctx);
		break;
	default:
		return -1;
	}
	return 0;
}

const char *kl_colorbar_get_pattern_name (struct kl_colorbar_context *ctx, enum kl_colorbar_pattern pattern)
{
	switch (pattern) {
	case KL_COLORBAR_BLACK:
		return "Black field";
	case KL_COLORBAR_EIA_189A:
		return "EIA-189A Colorbars";
	case KL_COLORBAR_SMPTE_RP_219_1:
		return "SMPTE RP 219-1 Colorbars";
	case KL_COLORBAR_SMPTE_RP_198:
		return "SMPTE RP 198 Checkfield";
	default:
		return NULL;
	}
}
