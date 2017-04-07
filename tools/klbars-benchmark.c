#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <libklbars/klbars.h>
#include <sys/time.h>

#define NUM_ITERATIONS 7500

int run_iteration(int width, int height, int indepth, int bitdepth)
{
	struct kl_colorbar_context osd_ctx;
	unsigned char *buf;
	struct timeval start_time, end_time, delta_time;

	/* Use the custom V210 stride required by the Decklink stack */
	int rowWidth = ((width + 47) / 48) * 128;

	buf = malloc(rowWidth * height);
	memset(buf, 0, rowWidth * height);
	kl_colorbar_init(&osd_ctx, width, height, indepth);

	printf("Generating %dx%d %d-bit colorbars (%d-bit internal) %d times...\n",
	       width, height, (bitdepth == KL_COLORBAR_10BIT ? 10 : 8),
	       (indepth == KL_COLORBAR_10BIT ? 10 : 8), NUM_ITERATIONS);
	gettimeofday(&start_time, NULL);
	printf("Start time\t%ld.%06d\n", start_time.tv_sec, start_time.tv_usec);
	for (int i = 0; i < NUM_ITERATIONS; i++) {
		kl_colorbar_fill_colorbars(&osd_ctx);
		char text[64];
		snprintf(text, sizeof(text), "Hello World!\n");
		kl_colorbar_render_string(&osd_ctx, text, strlen(text), 0, 2);
		kl_colorbar_finalize(&osd_ctx, buf, bitdepth, rowWidth);
	}
	gettimeofday(&end_time, NULL);
	printf("End time\t%ld.%06d\n", end_time.tv_sec, end_time.tv_usec);
	timersub(&end_time, &start_time, &delta_time);
	printf("Delta time\t%ld.%06d\n", delta_time.tv_sec, delta_time.tv_usec);

	/* Compute FPS */
	float fps = (float)NUM_ITERATIONS /
	  ((float)delta_time.tv_sec * 1000 + (float)delta_time.tv_usec / 1000) * 1000;
	printf("FPS=%f\n", fps);

	kl_colorbar_free(&osd_ctx);
	free(buf);
	return 0;
}

int main()
{
	/* 8-bit internal buffers */
	run_iteration(640, 480, KL_COLORBAR_8BIT, KL_COLORBAR_8BIT);
	run_iteration(640, 480, KL_COLORBAR_8BIT, KL_COLORBAR_10BIT);

	run_iteration(1280, 720, KL_COLORBAR_8BIT, KL_COLORBAR_8BIT);
	run_iteration(1280, 720, KL_COLORBAR_8BIT, KL_COLORBAR_10BIT);

	run_iteration(1920, 1080, KL_COLORBAR_8BIT, KL_COLORBAR_8BIT);
	run_iteration(1920, 1080, KL_COLORBAR_8BIT, KL_COLORBAR_10BIT);

	/* 10-bit internal buffers */
	run_iteration(640, 480, KL_COLORBAR_10BIT, KL_COLORBAR_8BIT);
	run_iteration(640, 480, KL_COLORBAR_10BIT, KL_COLORBAR_10BIT);

	run_iteration(1280, 720, KL_COLORBAR_10BIT, KL_COLORBAR_8BIT);
	run_iteration(1280, 720, KL_COLORBAR_10BIT, KL_COLORBAR_10BIT);

	run_iteration(1920, 1080, KL_COLORBAR_10BIT, KL_COLORBAR_8BIT);
	run_iteration(1920, 1080, KL_COLORBAR_10BIT, KL_COLORBAR_10BIT);
	return 0;
}
