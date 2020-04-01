#include "VideoCopy.h"

VideoCopy::VideoCopy() :
	run(false),
	handle_decode(nullptr),
	outFmtCtx(nullptr),
	videoStream(nullptr), audioStream(nullptr),	
	SPTS(0.0), EPTS(0.0),
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

int VideoCopy::init_encoder(const std::string& path, const std::string& startPts, const std::string& endPts)
{
	
	if (endPts <= startPts) {
		std::cout << "endPts <= startPts, this maybe err\n";
		return ERRORCODE::ERROR_PARAM;
	}
	SPTS = timeConvert(startPts);
	EPTS = timeConvert(endPts);

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

int VideoCopy::makeJpg(const std::string& position, const std::string& path)
{	
	float time = timeConvert(position);
	findKeyFrame(time, path);
	return 0;
}



int VideoCopy::exec()
{
	int ret = ERRORCODE::ERROR_NONE;
	int64_t endTime = EPTS / av_q2d(formatContext->streams[video_index]->time_base);
	AVPacket packet;
	ret = av_seek_frame(formatContext, -1, SPTS*1000*1000, AVSEEK_FLAG_BACKWARD);
	while (run) {
		ret = av_read_frame(formatContext, &packet);
		if( packet.stream_index == video_index && packet.pts >= endTime)
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
				if(packet.pts * av_q2d(outFmtCtx->streams[dst_aIndex]->time_base) > SPTS){
					timebaseConvert(packet, formatContext->streams[audio_index]->time_base, outFmtCtx->streams[dst_aIndex]->time_base);
					packet.stream_index = dst_aIndex;
					av_write_frame(outFmtCtx, &packet);
				}
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


int VideoCopy::screenShot(const std::string& jpgName,AVFrame* frame)
{
	//todo 对数据格式进行转换，避免因数据格式问题导致生成截图失败
	AVFrame *Shotjpg = pixformatScale(frame);
	if (Shotjpg == nullptr)
		return ERROR_RUNING;


	AVFormatContext* jpgContext = avformat_alloc_context();
	if (jpgContext == nullptr && (jpgContext = avformat_alloc_context()) == nullptr) {		
		return ERROR_RUNING;
	}
	jpgContext->oformat = av_guess_format("mjpeg", jpgName.c_str(), NULL);


	AVStream* stream = avformat_new_stream(jpgContext, NULL);
	AVCodecParameters* codecpar = stream->codecpar;
	codecpar->codec_id = jpgContext->oformat->video_codec;
	codecpar->width = Shotjpg->width;
	codecpar->height = Shotjpg->height;
	codecpar->format = AV_PIX_FMT_YUVJ420P;
	codecpar->codec_type = AVMEDIA_TYPE_VIDEO;

	AVCodec* codec = avcodec_find_encoder(codecpar->codec_id);
	if (codec == nullptr)
		return ERROR_RUNING;

	AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codecCtx,codecpar);
	codecCtx->time_base = AVRational{ 1,25 };

	int ret = avcodec_open2(codecCtx, codec, NULL);
	if (ret < 0)
		return ERROR_RUNING;
	ret = avio_open(&jpgContext->pb, jpgName.c_str(), AVIO_FLAG_WRITE);
	if (ret < 0)
		return ERROR_RUNING;

	ret = avformat_write_header(jpgContext, nullptr);
	if (ret < 0)
		return ERROR_RUNING;

	AVPacket* packet = av_packet_alloc();
	
	Shotjpg->pts = 1;
	ret = avcodec_send_frame(codecCtx, Shotjpg);
	if (ret < 0)
		return ERROR_RUNING;

	ret = avcodec_receive_packet(codecCtx, packet);
	if (ret < 0)
		return ERROR_RUNING;

	ret = av_write_frame(jpgContext, packet);
	if (ret < 0)
		return ERROR_RUNING;

	av_packet_unref(packet);
	av_write_trailer(jpgContext);
	avcodec_close(codecCtx);
	avio_close(jpgContext->pb);

	av_frame_free(&Shotjpg);
	avcodec_free_context(&codecCtx);
	avformat_free_context(jpgContext);

	return 0;
}

AVFrame* VideoCopy::pixformatScale(AVFrame* frame)
{
	AVFrame* jpgframe = av_frame_alloc();
	jpgframe->width = frame->width;
	jpgframe->height = frame->height;
	jpgframe->format = AV_PIX_FMT_YUVJ420P;
	av_frame_get_buffer(jpgframe, 4);
	int scaleH = -1;
	SwsContext* swsCtx = sws_alloc_context();
	swsCtx = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
		frame->width, frame->height, AV_PIX_FMT_YUVJ420P, SWS_BICUBIC, NULL, NULL,NULL);
	if(nullptr == swsCtx){
		goto err;
	}
	
	scaleH = sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, jpgframe->data, jpgframe->linesize);
	if (scaleH != frame->height) {
		goto err;
	}
	return jpgframe;

err:
	av_frame_free(&jpgframe);
	sws_freeContext(swsCtx);
	return nullptr;
}




int VideoCopy::findKeyFrame(float position, const std::string& path)
{
	bool run = true;
	int ret = 0;
	AVPacket packet;
	av_seek_frame(formatContext, -1, position * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD);
	while (run) {
		ret = av_read_frame(formatContext, &packet);
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
		if (packet.stream_index == video_index) {

			ret = avcodec_send_packet(video_codecCtx, &packet);
			if (ret < 0) {
				av_packet_unref(&packet);
				if (AVERROR(EINVAL) == ret) {
					break;
				}
				else {
					continue;
				}
			}

			while (1) {
				AVFrame* vframe = av_frame_alloc();
				ret = avcodec_receive_frame(video_codecCtx, vframe);
				if (ret < 0) {
					av_frame_free(&vframe);
					if (AVERROR(EINVAL) == ret) {
						return -1;
					}
					else {
						break;
					}
				}
				std::string tstr = std::to_string(vframe->pts * av_q2d(formatContext->streams[video_index]->time_base));
				std::string fname = path + tstr + ".jpg";
				ret = screenShot(fname, vframe);
				std::cout << "screen shot name: " << fname << std::endl;

				av_frame_free(&vframe);
				run = false;
				break;
			}
		}
		av_packet_unref(&packet);
	}

	return ret;
}

float VideoCopy::timeConvert(const std::string& position)
{
	size_t npos = std::string::npos;
	size_t offset = 0;
	int step = 3600;
	float sec_screenshot = 0.0;
	for (int index(3); index > 0; index--) {
		npos = position.find(':', offset);
		if (std::string::npos == npos)
			return ERROR_PARAM;
		sec_screenshot += step * std::stoi(position.substr(npos - 2, 2));
		step /= 60;
		offset = npos + 1;
	}
	sec_screenshot += std::stoi(position.substr(offset, offset + 2)) * 0.04;
	return sec_screenshot;
}
