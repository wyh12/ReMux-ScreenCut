#pragma once
#include "Decoder.h"
#include <thread>
enum ERRORCODE{
	ERROR_NONE = 0,
	ERROR_PARAM = -1,   //输入参数错误
	ERROR_DECODE = -2,  //文件解码失败
	ERROR_NOTEXIST = -3,//文件不存在或无法创建
	ERROR_RUNING = -4,  //运行产生错误
};

class VideoCopy:public Decoder
{
public:
	VideoCopy();
	virtual ~VideoCopy();

	virtual int decoderStream() override;
	virtual int stopStream() override;
	int init_encoder(const std::string& path,int64_t startPts,int64_t endPts = -1);
	int show_I_FreamTime();
protected:
	int exec();
	void timebaseConvert(AVPacket& packet, const AVRational& Tsrc, const AVRational& Tdst);

private:
	std::thread* handle_decode;
	bool run;

private:
	AVFormatContext* outFmtCtx;
	AVStream* videoStream;
	AVStream* audioStream;

private:
	//视频remux的起止时间
	int64_t SPTS;  
	int64_t EPTS;  

	int dst_vIndex;
	int dst_aIndex;
};

