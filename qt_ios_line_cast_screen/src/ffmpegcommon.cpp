//#include <Windows.h>
#include "ffmpegcommon.h"

int get_available_size(AVFrame *out, int samples /*= -1*/)
{
	int bytes_per_sample = av_get_bytes_per_sample((AVSampleFormat)out->format);
	int size = (samples != -1 ? samples : out->nb_samples) * bytes_per_sample;

	if (av_sample_fmt_is_planar((AVSampleFormat)out->format)) {
		return size;
	}
	else {
		int channels = av_get_channel_layout_nb_channels(out->channel_layout);
		return size * channels;
	}
}

int get_available_samples(AVFrame *out)
{
	int bytes_per_sample = av_get_bytes_per_sample((AVSampleFormat)out->format);
	int samples = out->linesize[0] / bytes_per_sample;

	if (av_sample_fmt_is_planar((AVSampleFormat)out->format)) {
		return samples;
	}
	else {
		int channels = av_get_channel_layout_nb_channels(out->channel_layout);
		return samples / channels;
	}
}

int get_pix_fmt_planes(enum AVPixelFormat pix_fmt)
{
	int planes = 0;
	for (int i = 0; i < 4; i++)
	{
		if (av_image_get_linesize(pix_fmt, 128, i) > 0)
			planes++;
	}
	return planes;
};

AVPacket* dup_packet(AVPacket* src)
{
	AVPacket *new_pkt = av_packet_alloc();
	av_new_packet(new_pkt, src->size);
	av_packet_copy_props(new_pkt, src);
	memcpy(new_pkt->data, src->data, src->size);

	int side_size = 0;
	uint8_t *side = av_packet_get_side_data(src, AV_PKT_DATA_NEW_EXTRADATA, &side_size);
	if (side && side_size > 0)
	{
		int ret = av_packet_add_side_data(new_pkt, AV_PKT_DATA_NEW_EXTRADATA, side, side_size);
	}
	return new_pkt;
}


AVFrame* dup_vframe(AVFrame* src)
{
	AVFrame * dst = av_frame_alloc();
	dst->format = src->format;
	dst->width = src->width;
	dst->height = src->height;

	if (av_frame_get_buffer(dst, 32) == 0)
		av_frame_copy(dst, src);
	return dst;
#if 0
	dst->channels = src->channels;
	dst->channel_layout = src->channel_layout;
	dst->nb_samples = src->nb_samples;

	int ret = av_frame_copy_props(dst, src);
	if (ret < 0)
	{
		av_frame_free(&dst);
		return NULL;
	}
	ret = av_frame_get_buffer(dst, 32);
	if (ret < 0)
	{
		av_frame_free(&dst);
		return NULL;
	}

	ret = av_frame_copy(dst, src);
	if (ret < 0)
	{
		av_frame_free(&dst);
		return NULL;
	}
	return dst;
#endif
}

AVFrame* dup_vframe_cache(AVFrame* cache, AVFrame* src)
{
	if (cache && (cache->width != src->width || cache->height != src->height || cache->format != src->format))
		av_frame_free(&cache);

	if (!cache)
		cache = new_vframe(src->width, src->height, src->format, 32);

	av_frame_copy(cache, src);
	return cache;
}

AVFrame* dup_aframe(AVFrame* src)
{
	AVFrame * newframeX = new_aframe2(src->channel_layout, src->sample_rate, src->nb_samples, src->format);
	av_frame_copy(newframeX, src);
	return newframeX;
}

AVFrame* dup_aframe_cache(AVFrame* cache, AVFrame* src)
{
	if (cache && (cache->channels != src->channels || cache->sample_rate != src->sample_rate || cache->nb_samples != src->nb_samples || cache->format != src->format))
		av_frame_free(&cache);

	if (!cache)
		cache = dup_aframe(src);
	else
		av_frame_copy(cache, src);

	return cache;
}

AVPacket* new_packet(int size)
{
	AVPacket *new_pkt = av_packet_alloc();
	av_new_packet(new_pkt, size);
	return new_pkt;
}

AVFrame* new_vframe(int w, int h, int format, int align/* = 32*/)
{
	AVFrame * newframeX = av_frame_alloc();
	newframeX->width = w;
	newframeX->height = h;
	newframeX->format = format;
	av_frame_get_buffer(newframeX, align);
	return newframeX;
}

AVFrame* new_aframe(int channels, int samplerate, int nsamples, int format)
{
	AVFrame* frame = av_frame_alloc();
	frame->nb_samples = nsamples;
	frame->format = format;
	frame->channels = channels;
	frame->channel_layout = av_get_default_channel_layout(channels);;
	frame->sample_rate = samplerate;

	/* allocate the data buffers */
	int ret = av_frame_get_buffer(frame, 1);
	return frame;
}

AVFrame* new_aframe2(int64_t channel_layout, int samplerate, int nsamples, int format)
{
	AVFrame* frame = av_frame_alloc();
	frame->nb_samples = nsamples;
	frame->format = format;
	frame->channel_layout = channel_layout;
	frame->channels = av_get_channel_layout_nb_channels(channel_layout);
	frame->sample_rate = samplerate;

	/* allocate the data buffers */
	int ret = av_frame_get_buffer(frame, 1);
	return frame;
}

AVFrame* new_vframe_cache(AVFrame* cache, int w, int h, int format, int align/* = 32*/)
{
	if (cache && (cache->width != w || cache->height != h || cache->format != format))
		av_frame_free(&cache);

	if (!cache)
		cache = new_vframe(w, h, format, align);

	return cache;
}

AVFrame* new_aframe_cache(AVFrame* cache, int channels, int sample_rate, int nb_samples, int format)
{
	if (cache && (cache->channels != channels || cache->sample_rate != sample_rate || cache->nb_samples != nb_samples || cache->format != format))
		av_frame_free(&cache);

	if (!cache)
		cache = new_aframe(channels, sample_rate, nb_samples, format);

	return cache;
}

void frame_free(AVFrame **frame)
{
	av_frame_free(frame);
}

SwrContext* new_swr_ctx(int channels, int samplerate, int format, int channels2, int samplerate2, int format2)
{
	struct SwrContext *swr_ctx = swr_alloc();
	if (!swr_ctx) {
		return NULL;
	}

	/* set options */
	av_opt_set_int(swr_ctx, "in_channel_count", channels, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", samplerate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", (AVSampleFormat)format, 0);
	av_opt_set_int(swr_ctx, "out_channel_count", channels2, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", samplerate2, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", (AVSampleFormat)format2, 0);

	if (swr_init(swr_ctx) < 0)
	{
		swr_free(&swr_ctx);
		return NULL;
	}
	return swr_ctx;
}

const char* _err2str(int errnum)
{
	static char str[AV_ERROR_MAX_STRING_SIZE];
	av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
	return str;
}

std::string serialize_opt(AVClass* cls, const char key_val_sep /*= '='*/, const char pairs_sep /*= ':'*/)
{
	char *buf = NULL;
	av_opt_serialize(cls, 0, 0, &buf, key_val_sep, pairs_sep);

	std::string  str(buf);

	av_free(buf);
	return str;
}

void set_options_by_string(AVClass* cls, const char* buff, const char* key_val_sep /*= "="*/, const char* pairs_sep /*= ":"*/)
{
	av_set_options_string(cls, buff, key_val_sep, pairs_sep);
}
