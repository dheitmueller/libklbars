#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "libklbars/klbars.h"
#include "klbars-internal.h"

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
