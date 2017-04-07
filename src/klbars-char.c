#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "libklbars/klbars.h"
#include "klbars-internal.h"
#include "font8x8_basic.h"

int kl_colorbar_render_moveto(struct kl_colorbar_context *ctx, int x, int y)
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
