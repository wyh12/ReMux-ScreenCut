// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "Decoder.h"
#include <libavformat/avio.h>
#include <libavformat/avformat.h>

static const struct {
	const char* name;
	int         nb_channels;
	uint64_t     layout;
} channel_layout_map[] = {
	{ "mono",        1,  AV_CH_LAYOUT_MONO },
	{ "stereo",      2,  AV_CH_LAYOUT_STEREO },
	{ "2.1",         3,  AV_CH_LAYOUT_2POINT1 },
	{ "3.0",         3,  AV_CH_LAYOUT_SURROUND },
	{ "3.0(back)",   3,  AV_CH_LAYOUT_2_1 },
	{ "4.0",         4,  AV_CH_LAYOUT_4POINT0 },
	{ "quad",        4,  AV_CH_LAYOUT_QUAD },
	{ "quad(side)",  4,  AV_CH_LAYOUT_2_2 },
	{ "3.1",         4,  AV_CH_LAYOUT_3POINT1 },
	{ "5.0",         5,  AV_CH_LAYOUT_5POINT0_BACK },
	{ "5.0(side)",   5,  AV_CH_LAYOUT_5POINT0 },
	{ "4.1",         5,  AV_CH_LAYOUT_4POINT1 },
	{ "5.1",         6,  AV_CH_LAYOUT_5POINT1_BACK },
	{ "5.1(side)",   6,  AV_CH_LAYOUT_5POINT1 },
	{ "6.0",         6,  AV_CH_LAYOUT_6POINT0 },
	{ "6.0(front)",  6,  AV_CH_LAYOUT_6POINT0_FRONT },
	{ "hexagonal",   6,  AV_CH_LAYOUT_HEXAGONAL },
	{ "6.1",         7,  AV_CH_LAYOUT_6POINT1 },
	{ "6.1(back)",   7,  AV_CH_LAYOUT_6POINT1_BACK },
	{ "6.1(front)",  7,  AV_CH_LAYOUT_6POINT1_FRONT },
	{ "7.0",         7,  AV_CH_LAYOUT_7POINT0 },
	{ "7.0(front)",  7,  AV_CH_LAYOUT_7POINT0_FRONT },
	{ "7.1",         8,  AV_CH_LAYOUT_7POINT1 },
	{ "7.1(wide)",   8,  AV_CH_LAYOUT_7POINT1_WIDE_BACK },
	{ "7.1(wide-side)",   8,  AV_CH_LAYOUT_7POINT1_WIDE },
	{ "octagonal",   8,  AV_CH_LAYOUT_OCTAGONAL },
	{ "hexadecagonal", 16, AV_CH_LAYOUT_HEXADECAGONAL },
	{ "downmix",     2,  AV_CH_LAYOUT_STEREO_DOWNMIX, },
};

Decoder::Decoder():
	formatContext(NULL), video_codecCtx(NULL), audio_codecCtx(NULL), dict(nullptr), 
	video_index(-1), audio_index(-1),
	start_time(0),
	dec_time(0),
	inputUrl()
{
	getDecodeTime();
}


Decoder::~Decoder()
{
	av_dict_free(&dict);
	uninit_decoder();
}

int Decoder::init_decoder(const std::string url)
{
	int ret = 0;
	if (url.empty()) {
		return -1;
	}

	formatContext = avformat_alloc_context();

//	setOpenOption();		// rtmp can't set timeout option   
//	setInterruptCallback();
	
	start_time = av_gettime();
	if ((ret = avformat_open_input(&formatContext, url.c_str(), NULL, &dict)) < 0) {
		std::cout << "avformat_open_input err " << AVERROR(ret) << std::endl;		
		return ret;
	}
//	formatContext->probesize = 50 * 1024 * 1024;
	if ((ret = avformat_find_stream_info(formatContext, NULL)) < 0) {
		std::cout << "avformat_find_stream_info err " << ret << std::endl;
		return ret;
	}
	
	if (formatContext->duration == AV_NOPTS_VALUE)
		std::cout << "this is a stream live\n";
/*	if(formatContext->nb_streams >= 3){
		std::cout << "Decode stream's number >=3_________________\n";
		return -1;
	}*/	
	ret = 2;	
	if (init_videoDec() < 0) {
		ret--;
	}

	if (init_audioDec() < 0) {
		ret--;
	}
	inputUrl = url;

//	std::cout << "finshed "<< (((HLSContext*)((AVIOInternal*)formatContext->pb->opaque)->h->priv_data)->finished) << std::endl;
	return (ret - (formatContext->nb_streams > 2)?2: formatContext->nb_streams);
}

void Decoder::setOpenOption(void)
{
	// set time out
	av_dict_set(&dict, "timeout", "10", 0);
//	int ret = av_dict_set(&dict, "movflags", "faststart", 0);
}



int Decoder::init_videoDec()
{
	
	AVCodec* codec_v = NULL;
	start_time = av_gettime();
	int ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &codec_v, 0);
	if (ret < 0) {
		std::cout << "av_find_best_stream err " << AVERROR(ret) << std::endl;
		if (AVERROR_DECODER_NOT_FOUND == ret) {
			std::cout << "video AVERROR_DECODER_NOT_FOUND \n";
		}
	}
	else
		video_index = ret;


	if (video_index >= 0) {
		video_codecCtx = avcodec_alloc_context3(codec_v);
		if (!video_codecCtx) {
			std::cout << "video avcodec_alloc_context3 err\n";
		}

	
		avcodec_parameters_to_context(video_codecCtx, formatContext->streams[video_index]->codecpar);
//		video_codecCtx->time_base = formatContext->streams[video_index]->codec->time_base;
		ret = avcodec_open2(video_codecCtx, codec_v, NULL);
		if (ret < 0) {
			std::cout << "video avcodec_open2 err " << ret << std::endl;
			return ret;
		}
	}
	return ret;
}

int Decoder::init_audioDec()
{	
	AVCodec* codec_a = NULL;
	int ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &codec_a, 0);
	if (ret < 0) {
		std::cout << "av_find_best_stream err " << ret << std::endl;
	}
	else
		audio_index = ret;

	if (audio_index >= 0) {
		audio_codecCtx = avcodec_alloc_context3(codec_a);
		if (!audio_codecCtx) {
			std::cout << "audio avcodec_alloc_context3 err\n";
		}
		
		
		avcodec_parameters_to_context(audio_codecCtx, formatContext->streams[audio_index]->codecpar);

		ret = avcodec_open2(audio_codecCtx, codec_a, NULL);
		if (ret < 0) {
			std::cout << "audio avcodec_open2 err " << ret << std::endl;
			return ret;
		}
	}
	return ret;
}


void Decoder::uninit_decoder()
{
//
//	stopStream();
	if (video_codecCtx) {
		avcodec_free_context(&video_codecCtx);
		video_codecCtx = NULL;
	}

	if (audio_codecCtx) {
		avcodec_free_context(&audio_codecCtx);
		audio_codecCtx = NULL;
	}

	if (formatContext) {
		avformat_free_context(formatContext);
		formatContext = NULL;
	}
	video_index = -1;
	audio_index = -1;
}


