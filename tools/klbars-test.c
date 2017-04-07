#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <libklbars/klbars.h>

int main()
{
	struct kl_colorbar_context osd_ctx;
	int width=640, height=480;
	unsigned char *buf;
	int ret;

	/* Use the custom V210 stride required by the Decklink stack */
	int rowWidth = ((width + 47) / 48) * 128;
	fprintf(stderr, "row stride is %d\n", rowWidth);

	buf = malloc(rowWidth * height);
	memset(buf, 0, rowWidth * height);
	kl_colorbar_init(&osd_ctx, width, height, KL_COLORBAR_10BIT);

	/* List all available patterns */
	for (int i = 0; i < 255; i++) {
		const char *name = kl_colorbar_get_pattern_name(&osd_ctx, i);
		if (name == NULL)
			break;
		printf("Pattern %d: %s\n", i, name);
	}

	kl_colorbar_fill_pattern(&osd_ctx, KL_COLORBAR_EIA_189A);
	kl_colorbar_finalize(&osd_ctx, buf, KL_COLORBAR_10BIT, rowWidth);
	for (int i = 0; i < 32; i++) {
		printf("%02x ", buf[i]);
	}
	printf("\n");

	int fd = open("foo.yuv", O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (fd < 0) {
		fprintf(stderr, "Failed to open output file\n");
		return 1;
	}
	ret = write(fd, buf, rowWidth * height);
	if (ret < 0)
		fprintf(stderr, "Error writing to file: %d\n", ret);
	else
		fprintf(stderr, "%d bytes written to file\n", ret);
	close(fd);

	kl_colorbar_free(&osd_ctx);
	free(buf);
	return 0;
}
