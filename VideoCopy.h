#pragma once
#include "Decoder.h"
#include <thread>
extern "C" {
#include <libswscale/swscale.h>
}
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
	int init_encoder(const std::string& path,const std::string& startPts, const std::string& endPts);
	int makeJpg(const std::string& position,const std::string& path);
	
protected:
	int exec();
	void timebaseConvert(AVPacket& packet, const AVRational& Tsrc, const AVRational& Tdst);
	int screenShot(const std::string& jpgName, AVFrame* frame);
	int findKeyFrame(float position, const std::string& path);
	float timeConvert(const std::string& position);
	AVFrame* pixformatScale(AVFrame* frame);
private:
	bool run;
	std::thread* handle_decode;

private:
	AVFormatContext* outFmtCtx;
	AVStream* videoStream;
	AVStream* audioStream;

private:
	//视频remux的起止时间
	float SPTS;  
	float EPTS;

	int dst_vIndex;
	int dst_aIndex;

};

