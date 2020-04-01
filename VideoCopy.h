#pragma once
#include "Decoder.h"
#include <thread>
extern "C" {
#include <libswscale/swscale.h>
}
enum ERRORCODE{
	ERROR_NONE = 0,
	ERROR_PARAM = -1,   //�����������
	ERROR_DECODE = -2,  //�ļ�����ʧ��
	ERROR_NOTEXIST = -3,//�ļ������ڻ��޷�����
	ERROR_RUNING = -4,  //���в�������
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
	//��Ƶremux����ֹʱ��
	float SPTS;  
	float EPTS;

	int dst_vIndex;
	int dst_aIndex;

};

