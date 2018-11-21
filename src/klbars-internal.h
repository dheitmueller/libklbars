#include <stdint.h>

void compute_colorbar_10bit_array(const uint32_t uyvy, uint8_t *bar10);

void compute_colorbar_10bit_array2(uint16_t y0, uint16_t cb, uint16_t cr,
				   uint8_t *bar10);

int kl_colorbar_render_moveto(struct kl_colorbar_context *ctx, int x, int y);

void kl_colorbar_fill_rp219_1(struct kl_colorbar_context *ctx);

void kl_colorbar_fill_rp198(struct kl_colorbar_context *ctx);
