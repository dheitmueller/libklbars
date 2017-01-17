//
//  klbars.c
//  Kernel Labs Colorbar Generator
//
//  Created by Devin Heitmueller
//  Copyright (c) 2016 Kernel Labs Inc. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "libklbars/klbars.h"
#include "font8x8_basic.h"

#define ALPHA_BACKGROUND 0

typedef unsigned char u8;

int kl_colorbar_init(struct kl_colorbar_context *ctx, unsigned int width,
		     unsigned int height, int bitDepth)
{
	memset(ctx, 0, sizeof(*ctx));
    
	ctx->plotwidth = 8 * 4; /* The 8bit char is rendered as 32x32 */
	ctx->plotheight = ctx->plotwidth;
	ctx->plotctrl = 8;

	ctx->colorspace = bitDepth;
	if (bitDepth == KL_COLORBAR_8BIT)
	  ctx->frame = malloc(height * width * 2);
	else if (bitDepth == KL_COLORBAR_10BIT) {
	  /* We internally use 16-bit integers for internal frame
	     representation (makes blending easier) */
	  ctx->frame = malloc(height * width * sizeof(uint16_t) * 2);
	} else {
	  // Unknown color depth
	  return -1;
	}

	ctx->width = width;
	ctx->height = height;
	ctx->stride = width * 2;

	ctx->currx = 0;
	ctx->curry = 0;
    
	return 0;
}

int kl_colorbar_finalize(struct kl_colorbar_context *ctx, unsigned char *buf,
			 unsigned int byteStride)
{
	int y;
	if (ctx->colorspace == KL_COLORBAR_8BIT) {
		for (y = 0; y < ctx->height; y++) {
			memcpy(buf, ctx->frame + (y * ctx->width * 2), ctx->width * 2);
			buf += byteStride;
		}
	} else {
		/* For now just handle the colorspace in 8-bit, and colorspace convert
		   it to 10-bit on finalize */
		for (y = 0; y < ctx->height; y++) {
			/* Note, we're simultaneously converting 8-bit to 10-bit *AND*
			   repacking to 10-bit in the same operation, which is why this
			   is pretty convoluted */

			/* FIXME:  this just begs for some SSE optimization */
			unsigned char *line = ctx->frame + (y * ctx->width * 2);
			int n = 0;
			for (int x = 0; x < ctx->width * 2; x+= 3) {
				buf[n] = line[x] << 2;
				buf[n+1] = (line[x] >> 6) | (line[x+1] << 4);
				buf[n+2] = (line[x+1] >> 4) | (line [x+2] << 6);
				buf[n+3] = (line[x+2] >> 2);
				n += 4;
			}
			buf += byteStride;
		}
	}
	return 0;
}

void kl_colorbar_free(struct kl_colorbar_context *ctx)
{
	free(ctx->frame);
}

// SD 75% Colour Bars
static uint32_t gSD75pcColourBars[8] =
{
	0xeb80eb80, 0xa28ea22c, 0x832c839c, 0x703a7048,
	0x54c654b8, 0x41d44164, 0x237223d4, 0x10801080
};

// HD 75% Colour Bars
static uint32_t gHD75pcColourBars[8] =
{
	0xeb80eb80, 0xa888a82c, 0x912c9193, 0x8534853f,
	0x3fcc3fc1, 0x33d4336d, 0x1c781cd4, 0x10801080
};

/* Intended to conform to EIA-189-A */
void kl_colorbar_fill_colorbars(struct kl_colorbar_context *ctx)
{
	uint32_t *nextWord = (uint32_t *) ctx->frame;
	uint32_t *bars;
	uint32_t y = 0;

	if (ctx->width > 720)
		bars = gHD75pcColourBars;
	else
		bars = gSD75pcColourBars;

	/* Vertical color bars for top 75% of field */
	for (y = 0; y < (ctx->height * 3 / 4); y++)
	{
		for (uint32_t x = 0; x < ctx->width; x+=2)
			*(nextWord++) = bars[(x * 8) / ctx->width];
	}

	while (y < ctx->height) {
		uint32_t x = 0;
		int b_width = ((ctx->width / 8) * 5 / 4);
		/* -I */
		while (x < b_width) {
			*(nextWord++) = 0x109410af;
			x += 2;
		}

		/* White */
		while (x < b_width * 2) {
			*(nextWord++) = 0xff80ff80;
			x += 2;
		}

		/* -Q */
		while (x < b_width * 3) {
			*(nextWord++) = 0x105f109e;
			x += 2;
		}

		/* Black */
		while (x < ctx->width) {
			*(nextWord++) = 0x00800080;
			x += 2;
		}
		y++;
	}
}

void kl_colorbar_fill_black(struct kl_colorbar_context *ctx)
{
	uint32_t *nextWord = (uint32_t *) ctx->frame;

	long wordsRemaining = (ctx->width * 2 * ctx->height) / 4;

	while (wordsRemaining-- > 0)
		*(nextWord++) = 0x10801080;
}

static int kl_colorbar_render_moveto(struct kl_colorbar_context *ctx, int x, int y)
{
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

int kl_colorbar_render_character(struct kl_colorbar_context *ctx, u8 letter)
{
	int i, j, k;
	u8 line;
    
	if (letter > 0x9f)
		return -1;
    
	for (i = 0; i < 8; i++) {
		k = 0;
		while (k++ < 4) {
			line = font8x8_basic[letter][ i ];
			for (j = 0; j < 8; j++) {
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

static int kl_colorbar_render_ascii(struct kl_colorbar_context *ctx, u8 letter, int x, int y)
{
	if (letter > 0x9f)
		return -1;
    
	kl_colorbar_render_moveto(ctx, x, y);
	kl_colorbar_render_character(ctx, letter);
    
	return 0;
}

int kl_colorbar_render_string(struct kl_colorbar_context *ctx, u8 *s, int len, int x, int y)
{
	int i;
    
	for (i = 0; i < len; i++)
		kl_colorbar_render_ascii(ctx, *(s + i), x + i, y);
    
	return 0;
}

int kl_colorbar_render_reset(struct kl_colorbar_context *ctx, u8 *ptr, long width)
{
	if (!ptr)
		return -1;
    
	switch (ctx->width) {
        case 640:
        case 800:
        case 720:
        case 768:
        case 1024:
        case 1280:
        case 1400:
        case 1440:
        case 1600:
        case 1680:
        case 1920:
        case 2048: /* Black Magic Extreme 3D 2K formats */
		break;
        default:
		printf("%s() illegal stride %d\n", __func__, ctx->width);
		return -1;
	}
    
	ctx->ptr = ptr;
	ctx->frame = ptr;
	ctx->width = width;
	ctx->stride = ctx->width * 2;

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

