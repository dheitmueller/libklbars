#include <stdint.h>

void compute_colorbar_10bit_array(const uint32_t uyvy, uint8_t *bar10);

int kl_colorbar_render_moveto(struct kl_colorbar_context *ctx, int x, int y);
