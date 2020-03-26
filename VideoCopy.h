#pragma once
#include "Decoder.h"
#include <thread>
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
	//��Ƶremux����ֹʱ��
	int64_t SPTS;  
	int64_t EPTS;  

	int dst_vIndex;
	int dst_aIndex;
};

