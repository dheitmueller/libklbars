/**
 * @file        klbars.h
 * @author      Devin Heitmueller <dheitmueller@kernellabs.com>
 * @copyright   Copyright (c) 2016-2017 Kernel Labs Inc. All Rights Reserved.
 * @brief       A mechanism to draw standardized colorbars and audio tones,\n
 *              useful when creating audio/video generators.\n
 *              A single colorbar pattern is supported (EIA-189-A), in both 8
 *              and 10 bit pixel formats.\n
 *              In all cases, the output colorspace format is YUV422.\n
 */

#ifndef klbars_h
#define klbars_h

#ifdef __cplusplus
extern "C" {
#endif

#define KL_COLORBAR_8BIT  0
#define KL_COLORBAR_10BIT 1

struct kl_colorbar_context
{
    unsigned char *frame, *ptr; /* top left of render image and a working ptr */
    unsigned int width, height; /* width/height in pixels */
    unsigned int stride;

    unsigned int pic_count; /* Increments with every finalize */

    int plotwidth, plotheight, plotctrl;
    int colorspace;

    int currx, curry;

    /* Rendered font fg and bg colors */
    unsigned char bg[2], fg[2];
};

struct kl_colorbar_audio_context
{
	unsigned char *audio_data;
	size_t audio_data_size;
	size_t currentLocation;
};

/**
 * @brief       Initialize a previously allocated context, for a pixel width and height, and a final output stride.
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 * @param[in]   unsigned int width - in pixels.
 * @param[in]   unsigned int height - in pixels.
 * @param[in]   unsigned int bitDepth - A value of KL_COLORBAR_8BIT or KL_COLORBAR_10BIT is supported.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int kl_colorbar_init(struct kl_colorbar_context *ctx, unsigned int width,
		     unsigned int height, int bitDepth);

/**
 * @brief       Put the fully compositied colorbar frame into a final user allocated buffer in the requested
 *              colorspace (TODO) and stride.
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int kl_colorbar_finalize(struct kl_colorbar_context *ctx, unsigned char *buf,
			 int targetColorspace, unsigned int byteStride);
/**
 * @brief       Free any internal allocations containined within the context, but note that this DOES NOT
 *              free the context itself. The context is user allocated and user destroyed. The context is no longer
 *              valid for use once this call returns.
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 */
void kl_colorbar_free(struct kl_colorbar_context *ctx);

/**
 * @brief       Reset / re-initialize any internal position mechanisms related to string compositing.
 *              Generally you should do this at the beginning of every frame, before you render strings.
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 */
int kl_colorbar_render_reset(struct kl_colorbar_context *ctx);

enum kl_colorbar_pattern {
	/** Colorbars conforming to SMPTE RP219-1 **/
	KL_COLORBAR_SMPTE_RP_219_1,
	/** Completely black video frame **/
	KL_COLORBAR_BLACK,
	/** Colorbars conforming to EIA-189A **/
	KL_COLORBAR_EIA_189A,
	/* SMPTE RP 198 Checkfield for HD Interfaces (i.e. "half pathological") */
	KL_COLORBAR_SMPTE_RP_198,
};
/**
 * @brief       Composite the string 's' of length into the colorbar at position x, y, where 0,0 is top left.
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 * @param[in]   char *s - ASCII string.
 * @param[in]   unsigned int len - length of string in bytes, maximum 128 bytes.
 * @param[in]   unsigned int x - Horizontal position
 * @param[in]   unsigned int y - Veritical position
 * @return      0 - Success
 * @return      < 0 - Error
 */
int kl_colorbar_render_string(struct kl_colorbar_context *ctx, char *s, unsigned int len, unsigned int x, unsigned int y);

/**
 * @brief       Fill colorbar with a pattern (e.g. EIA-189 colorbars, black video, etc)
 * @param[in]   kl_colorbar_context *ctx - Context.
 * @param[in]   kl_colorbar_pattern pattern - The pattern to use.
 */
int kl_colorbar_fill_pattern(struct kl_colorbar_context *ctx, enum kl_colorbar_pattern pattern);

/**
 * @brief       Generate a colorbar frame, which was previously configured via KL_COLORBAR_xxx.
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 */
void kl_colorbar_fill_colorbars(struct kl_colorbar_context *ctx);

/**
 * @brief       Generate a fixed black frame.
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 */
void kl_colorbar_fill_black(struct kl_colorbar_context *ctx);

/**
 * @brief       Retrieve name of named pattern.
 *              This allows an application to get a textual representation
 *              for a named pattern.  Typically the application would use a
 *              loop to get the names of all the available patterns (i.e.
 *              increment pattern until NULL is returned).
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 */
const char *kl_colorbar_get_pattern_name(struct kl_colorbar_context *ctx,
					 enum kl_colorbar_pattern pattern);


/**
 * @brief       TODO: Document..... Generate an audio tone which can be pushed out on a PCM channel.
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int kl_colorbar_tonegenerator(struct kl_colorbar_audio_context *audio_ctx,
			      int toneFreqHz, int sampleSize,
			      int channelCount, int durationUs,
			      int sampleRate, int signedSample);

/**
 * @brief       TODO: Document.....
 * @param[in]   struct kl_colorbar_context *ctx - Context.
 * @param[in]   struct kl_colorbar_audio_context *audio_ctx - Context
 */
void kl_colorbar_tonegenerator_extract(struct kl_colorbar_audio_context *audio_ctx,
				       unsigned char *buf, size_t bufSize);

/**
 * @brief       Free any internal allocations containined within the context, but note that this DOES NOT
 *              free the context itself. The context is user allocated and user destroyed. The context is no longer
 *              valid for use once this call returns.
 * @param[in]   struct kl_colorbar_audio_context *ctx - Context.
 */
void kl_colorbar_tonegenerator_free(struct kl_colorbar_audio_context *ctx);

#ifdef __cplusplus
};
#endif

#endif
