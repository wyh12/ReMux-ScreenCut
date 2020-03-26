#include "VideoCopy.h"

VideoCopy::VideoCopy():
	handle_decode(nullptr),
	run(false),
	outFmtCtx(nullptr),
	videoStream(nullptr), audioStream(nullptr),	
	SPTS(0), EPTS(0),
	dst_vIndex(-1), dst_aIndex(-1)
{

}

VideoCopy::~VideoCopy()
{
	
	stopStream();
	if (outFmtCtx) {
		avformat_free_context(outFmtCtx);
		outFmtCtx = nullptr;
	}
}

int VideoCopy::decoderStream()
{
	run = true;
	handle_decode = new std::thread([&]() {	this->exec(); });
	return 0;
}

int VideoCopy::stopStream()
{
//	run = false;
	if (handle_decode) {

		std::cout << "wait decode thread quit..."<<std::flush;
		if (handle_decode->joinable()) {
			handle_decode->join();
		}
		std::cout << " thread is stop" << std::endl;

		delete handle_decode;
		handle_decode = nullptr;
	}
	return 0;
}

int VideoCopy::init_encoder(const std::string& path, const int64_t startPts, const int64_t endPts)
{
	
	if (startPts * av_q2d(formatContext->streams[video_index]->time_base) > formatContext->duration) {
		std::cout << "start time is biger than duration\n";
		return ERRORCODE::ERROR_PARAM;
	}
	SPTS = startPts;
	EPTS = endPts;

	outFmtCtx = avformat_alloc_context();
	if (!outFmtCtx) {
		std::cout << "avformat_alloc_context err\n";
		return ERRORCODE::ERROR_RUNING;
	}

	int ret = avformat_alloc_output_context2(&outFmtCtx, NULL, "mp4", path.c_str());
	if (ret < 0) {
		std::cout << "err open out file\n";
		return ERRORCODE::ERROR_RUNING;
	}
	
	//有视频流
	if (video_index != -1) {
		videoStream = avformat_new_stream(outFmtCtx, NULL);
		avcodec_parameters_copy(videoStream->codecpar, formatContext->streams[video_index]->codecpar);
		videoStream->codecpar->codec_tag = 0;
		dst_vIndex = 0;
	}

	//有音频流
	if (audio_index != -1) {
		audioStream = avformat_new_stream(outFmtCtx, nullptr);
		avcodec_parameters_copy(audioStream->codecpar, formatContext->streams[audio_index]->codecpar);
		audioStream->codecpar->codec_tag = 0;
		dst_aIndex = dst_vIndex + 1;
	}

	ret = avio_open(&(outFmtCtx->pb), path.c_str(), AVIO_FLAG_WRITE);
	if (ret < 0) {
		std::cout << "err avio_open " << ret<<std::endl;
		return ERRORCODE::ERROR_RUNING;
	}
	
	ret = avformat_write_header(outFmtCtx, nullptr);
	if (ret < 0) {
		std::cout << "err avformat_write_header " << ret << std::endl;
		return ERRORCODE::ERROR_RUNING;
	}

	return 0;
}

int VideoCopy::show_I_FreamTime()
{
	int ret = 0;
	AVPacket packet;

	av_seek_frame(formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);

	while (true) {
		ret = av_read_frame(formatContext, &packet);
		if (ret < 0){
			if (ret == AVERROR_EOF) {
				std::cout << "End Of File\n";
			}
			else {
				std::cout << "Exec Error\n";
				ret = ERRORCODE::ERROR_DECODE;
			}
			break;
		}

		if (packet.stream_index == video_index && packet.flags == AV_PKT_FLAG_KEY) {
			std::cout << "key frame pts " << packet.pts
				<< " act time " << packet.pts * av_q2d(formatContext->streams[video_index]->time_base) << std::endl;
		}
	}

	return ret;
}

int VideoCopy::screenShot(const std::string& path, const std::string& timeStamp)
{
	//path format 11:22:33::4  前面3位为时间，最后一位为当前帧的序列号
	int miliionSec = 0;
	
	int setp = 3600;
	int offset = 0;

	for(int loop(3);loop >0;loop--){
		size_t npos = timeStamp.find(':', offset);
		miliionSec += setp* std::stoi(timeStamp.substr(offset, npos));
		offset = npos + 1;
		setp /= 60;
	}

	size_t npos = timeStamp.find(':', offset);
	int frameIdx = std::stoi(timeStamp.substr(offset, npos));

	std::cout << "screen time " << miliionSec << " frame index " << frameIdx << std::endl;
	return 0;
}

int VideoCopy::exec()
{
	int ret = ERRORCODE::ERROR_NONE;
	AVPacket packet;
	ret = av_seek_frame(formatContext, video_index, SPTS, AVSEEK_FLAG_BACKWARD);
	while (run) {
		ret = av_read_frame(formatContext, &packet);
		if( -1 != EPTS && packet.stream_index == video_index && packet.pts >= EPTS)
			break;
		if (ret < 0) {
			if (ret == AVERROR_EOF) {
				std::cout << "End Of File\n";
			}
			else {
				std::cout << "Exec Error\n";
				ret = ERRORCODE::ERROR_DECODE;
			}
			break;
		}
		else {
			if (packet.stream_index == video_index && dst_vIndex > -1) {
				timebaseConvert(packet, formatContext->streams[video_index]->time_base, outFmtCtx->streams[dst_vIndex]->time_base);
				packet.stream_index = dst_vIndex;
				av_write_frame(outFmtCtx, &packet);
			}
			
			if (packet.stream_index == audio_index && dst_aIndex > -1) {
				timebaseConvert(packet, formatContext->streams[audio_index]->time_base, outFmtCtx->streams[dst_aIndex]->time_base);
				packet.stream_index = dst_aIndex;
				av_write_frame(outFmtCtx, &packet);
			}		
		}
		av_packet_unref(&packet);
	}
	
	av_write_trailer(outFmtCtx);
	avio_close(outFmtCtx->pb);
	return ret;
}


void VideoCopy::timebaseConvert(AVPacket& packet,const AVRational& Tsrc, const AVRational& Tdst)
{
	int64_t pts = av_rescale_q(packet.pts, Tsrc, Tdst);
	int64_t dts = av_rescale_q(packet.dts, Tsrc, Tdst);
	packet.pts = pts;
	packet.dts = dts;
}
