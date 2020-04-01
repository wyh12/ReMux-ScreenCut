#include "VideoCopy.h"
#include <iostream>
#include <string.h>

int FindKeyFrame(const char *srcFile,const char *dstFile,const char * startPts,const char * endPts) {
	int ret = ERROR_NONE;
	VideoCopy vc;
	ret = vc.init_decoder(std::string(srcFile));
	if (ret < 0) {
		std::cout << "Decoder File error ret " << ret << std::endl;
		return ret;
	}

	// I帧拆条
	ret = vc.init_encoder(std::string(dstFile), std::string(startPts), std::string(endPts));
	if(ret < 0)
		return ret;

	ret = vc.decoderStream();	
	return ret; 
}

int ScreenShot(const char *srcFile,const char *dstFile,const char * startPts){
	int ret = ERROR_NONE;
	VideoCopy vc;
	ret = vc.init_decoder(std::string(srcFile));
	if (ret < 0) {
		std::cout << "Decoder File error ret " << ret << std::endl;
		return ret;
	}
	// 截图
	ret = vc.makeJpg(std::string(startPts),std::string(dstFile));
	return ret;
}

int main(int argc,char ** argv){
	if (argc < 4) {
		std::cout << "params not enough \n";
		std::cout << "cmd PathIn PathOut StartPts EndPts 拆条\n";
		std::cout << "cmd PathIn PathOut StartPts 截图\n";
		return ERROR_PARAM;
	}
	
	int ret = 0;
	if(strcmp(argv[1],"ScreenShot") == 0){
	
	}else if (strcmp(argv[1],"SplitVideo") == 0){
	  ret = FindKeyFrame(argv[2], argv[3], argv[4], argv[5]);
	}
	return ret;
}
