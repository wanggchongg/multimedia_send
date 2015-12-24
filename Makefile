.SUFFIXES : .c .cpp .o#变量"SUFFIXE"用来定义默认的后缀列表

OBJECTS = ./main.o						\
		  ./Common/performance.o		\
		  ./Common/LogMsg.o				\
		  ./FrameExtractor/FileRead.o 		\
		  ./FrameExtractor/FrameExtractor.o 	\
		  ./FrameExtractor/H263Frames.o		\
		  ./FrameExtractor/H264Frames.o		\
		  ./FrameExtractor/MPEG4Frames.o	\
		  ./FrameExtractor/VC1Frames.o		\
		  ./JPEG_API/JPGApi.o			\
		  ./MFC_API/SsbSipH264Decode.o	\
		  ./MFC_API/SsbSipH264Encode.o	\
		  ./MFC_API/SsbSipMfcDecode.o	\
		  ./MFC_API/SsbSipMpeg4Decode.o	\
		  ./MFC_API/SsbSipMpeg4Encode.o	\
		  ./MFC_API/SsbSipVC1Decode.o \
		  ./CharSet.o            \
		  ./YUVOSDMixer.o        \
		  ./Raptor/raptorcode.o		\
		  ./Raptor/matrix.o     

SRCS =    ./main.cpp   				\
		  ./Common/performance.c		\
		  ./Common/LogMsg.c			\
		  ./FrameExtractor/FileRead.c 		\
	  	  ./FrameExtractor/FrameExtractor.c 	\
		  ./FrameExtractor/H263Frames.c		\
		  ./FrameExtractor/H264Frames.c		\
		  ./FrameExtractor/MPEG4Frames.c	\
		  ./FrameExtractor/VC1Frames.c		\
		  ./JPEG_API/JPGApi.c			\
		  ./MFC_API/SsbSipH264Decode.c	\
		  ./MFC_API/SsbSipH264Encode.c	\
		  ./MFC_API/SsbSipMfcDecode.c	\
		  ./MFC_API/SsbSipMpeg4Decode.c	\
		  ./MFC_API/SsbSipMpeg4Encode.c	\
		  ./MFC_API/SsbSipVC1Decode.c   \
		  ./CharSet.cpp            \
		  ./YUVOSDMixer.cpp        \
		  ./Raptor/raptorcode.c          \
		  ./Raptor/matrix.c          
		  
DEPENDENCY = ./Common/lcd.h 			\
		     ./Common/LogMsg.h 			\
		     ./Common/mfc.h 			\
		     ./Common/MfcDriver.h 		\
		     ./Common/MfcDrvParams.h 	\
		     ./Common/performance.h 	\
			 ./Common/post.h			\
			 ./Common/videodev2.h		\
			 ./Common/videodev2_s3c.h	\
			 ./Common/videodev2_s3c_tv.h	\
			 ./FrameExtractor/FileRead.h 		\
		     ./FrameExtractor/FrameExtractor.h 	\
		     ./FrameExtractor/H263Frames.h 		\
		     ./FrameExtractor/H264Frames.h 		\
		     ./FrameExtractor/MPEG4Frames.h 	\
		     ./FrameExtractor/VC1Frames.h 		\
			 ./JPEG_APIJPGApi.h					\
			 ./MFC_API/SsbSipH264Decode.h		\
		     ./MFC_API/SsbSipH264Encode.h		\
		     ./MFC_API/SsbSipMfcDecode.h		\
		     ./MFC_API/SsbSipMpeg4Decode.h		\
		     ./MFC_API/SsbSipMpeg4Encode.h		\
		     ./MFC_API/SsbSipVC1Decode.h		\
		     ./CharSet.h             \
		     ./YUVOSDMixer.h         \
		     ./myHead.h              \
		     ./YUVOSDMixerT.h        \
			 ./Raptor/raptorcode.h   \
			 ./Raptor/matrix.h       \
	  		 ./main.h							

CC = arm-linux-g++ 

CFLAGS = -g -c -Os -Wall -DLCD_SIZE_43 

INC = -I./Common -I./FrameExtractor -I./MFC_API -I./JPEG_API -I./Raptor 

TARGET = multimedia_test



all: common frame_extractor jpeg_api mfc_api raptor $(TARGET)

common: 
	cd Common; $(MAKE)

frame_extractor: 
	cd FrameExtractor; $(MAKE)

jpeg_api: 
	cd JPEG_API; $(MAKE)

mfc_api: 
	cd MFC_API; $(MAKE)

raptor:
	cd Raptor; $(MAKE)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -lpthread -o $@

#规则表示所有的 .o文件都是依赖与相应的.c文件的
.c.o:
	$(CC) $(INC) $(CFLAGS) $<
.cpp.o:
	$(CC) $(INC) $(CFLAGS) $<

clean:
	-rm -rf $(OBJECTS) $(TARGET) 

