#pragma once
#include <string>
#include <iostream>
#include <array>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/channel_layout.h>

};


class Decoder
{
public:

	Decoder();
	virtual ~Decoder();
	virtual int init_decoder(const std::string url);
	void uninit_decoder();
	
	virtual int  decoderStream() = 0;
	virtual int	 stopStream() = 0;
	static int64_t getDecodeTime() {
		static int64_t time = av_gettime();
		return time;
	}
protected:
	virtual void setOpenOption(void);			
	virtual	void setInterruptCallback() {}   // ffmpeg interrupt callback
	

	virtual int init_videoDec();
	virtual int init_audioDec();
	
	
protected:
	AVFormatContext*	formatContext;
	AVCodecContext*		video_codecCtx;
	AVCodecContext*		audio_codecCtx;
	AVDictionary*		dict;

	int						video_index;
	int						audio_index;
	int64_t					start_time;		// record ffmpeg IO require time for caculate timeout
	int64_t					dec_time;		// start decode time
	std::string				inputUrl;
	
private:
	
};

