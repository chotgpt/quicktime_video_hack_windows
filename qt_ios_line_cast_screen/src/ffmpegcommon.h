#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libavutil/mem.h>
}

#include <string>

int get_available_size(AVFrame *out, int samples = -1);
int get_available_samples(AVFrame *out);

int get_pix_fmt_planes(enum AVPixelFormat pix_fmt);

AVPacket* dup_packet(AVPacket* src);
AVFrame* dup_vframe(AVFrame* src);
AVFrame* dup_vframe_cache(AVFrame* cache, AVFrame* src);
AVFrame* dup_aframe(AVFrame* src);
AVFrame* dup_aframe_cache(AVFrame* cache, AVFrame* src);

AVPacket* new_packet(int size);
AVFrame* new_vframe(int w, int h, int format, int align = 32);
AVFrame* new_aframe(int channels, int samplerate, int nsamples, int format);
AVFrame* new_aframe2(int64_t channel_layout, int samplerate, int nsamples, int format);

AVFrame* new_vframe_cache(AVFrame* cache, int w, int h, int format, int align = 32);
AVFrame* new_aframe_cache(AVFrame* cache, int channels, int samplerate, int nsamples, int format);
void frame_free(AVFrame **frame);
//void     vertical_flip_vframe(AVFrame* src);

struct SwrContext* new_swr_ctx(int channels, int samplerate, int format, int channels2, int samplerate2, int format2);
const char*  _err2str(int errnum);

std::string serialize_opt(AVClass* cls, const char key_val_sep = '=', const char pairs_sep = ':');
void set_options_by_string(AVClass* cls, const char* buff, const char* key_val_sep = "=", const char* pairs_sep = ":");

