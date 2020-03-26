#define _CRT_SECURE_NO_WARNINGS
#include "VideoCopy.h"
#include <iostream>

int FindKeyFrame(const char *srcFile,const char *dstFile,const int64_t startPts,const int64_t endPts) {
	int ret = ERROR_NONE;
	VideoCopy vc;
	ret = vc.init_decoder(std::string(srcFile));
	if (ret < 0) {
		std::cout << "Decoder File error ret " << ret << std::endl;
		return ret;
	}
	ret = vc.init_encoder(std::string(dstFile), startPts, endPts);
	if(ret < 0)
		return ret;

	ret = vc.decoderStream();	
	return ret; 
}


//"D:\CloudMusic\MV\J.Fla-Shape_Of_You.mp4" "D:\CloudMusic\MV\remux.mp4" 145647 293128
int main(int argc,char ** argv){
	if (argc < 4) {
		std::cout << "params not enough \n";
		std::cout << "PathIn PathOut StartPts EndPts\n";
		return ERROR_PARAM;
	}

	for (int index = 1; index < argc; index++) {
		std::cout << argv[index] << " ";
	}
	std::cout << std::endl;
	int ret = FindKeyFrame(argv[1], argv[2], atoi(argv[3]), atoi(argv[4]));
	return ret;
}


