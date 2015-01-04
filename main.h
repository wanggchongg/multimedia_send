#ifndef __SAMSUNG_SYSLSI_APDEV_CAM_ENCODING_TEST_H__
#define __SAMSUNG_SYSLSI_APDEV_CAM_ENCODING_TEST_H__
// #include "alsa/asoundlib.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
//#include <g729a/typedef.h>
//#include <g729a/basic_op.h>
//#include <g729a/ld8a.h>
#include <limits.h>
#include <linux/fb.h>
#include <linux/videodev2.h>
#include <linux/vt.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "FileRead.h"
#include "FrameExtractor.h"
#include "lcd.h"
//#include "lt_enc.h"
#include "LogMsg.h"
#include "H263Frames.h"
#include "H264Frames.h"
#include "MfcDriver.h"
#include "MPEG4Frames.h"
#include "performance.h"
#include "post.h"
#include "SsbSipH264Encode.h"
#include "SsbSipH264Decode.h"
#include "SsbSipMpeg4Decode.h"
#include "SsbSipVC1Decode.h"

extern "C"{
	#include "raptorcode.h"
}


#include "CharSet.h"
#include "YUVOSDMixer.h"
#include "YUVOSDMixerT.h"
/***************** MAIN ARGUMENTS *******************/
#define IP_DEFAULT "10.0.0.87"
#define GOP_DEFAULT 5
#define QP_DEFAULT 28
#define RES_DEFAULT 4  // 480*272
#define WITH_VEDIO_DEFAULT 1  //WITH_VEDIO
#define WITH_AUDIO_DEFAULT 0  //WITH_AUDIO
#define WITH_PREVIEW_DEFAULT 0  //NO_PREVIEW
#define WITH_MULTI_DESCRIPTION 0 //NO_MULTI_DESCRIPTION
#define FRAGMENT_SIZE_DEFAULT 5000 
#define WITH_LOCAL_DEFAULT 0
#define WITH_FB_DEFAULT 0
#define WITH_FEC_DEFAULT 1
#define T_INIT_DEFAULT 128


#define PORT_V 8888
#define PORT_A 5555
#define PORT_GPS 6666

/******************* CAMERA ********************/
#define SAMSUNG_UXGA_S5K3BA
#ifdef RGB24BPP
	#define LCD_24BIT		/* For s3c2443/6400 24bit LCD interface */
	#define LCD_BPP_V4L2		V4L2_PIX_FMT_RGB24
#else
	#define LCD_BPP_V4L2		V4L2_PIX_FMT_RGB565	/* 16 BPP - RGB565 */
#endif
#define PREVIEW_NODE  "/dev/video1"
#define CODEC_NODE  "/dev/video0"

/************* VEDIO FRAME BUFFER ***************/
#ifdef RGB24BPP
	#define LCD_BPP	24	/* 24 BPP - RGB888 */
#else
	#define LCD_BPP	16	/* 16 BPP - RGB565 */
#endif


/*function declaration*/
static int cam_p_init();
static int cam_c_init();
static int read_data(int, char *, int, int, int);
//static int fb_init(int, int, int, int, int, int, unsigned int *);
static void draw(char *, char *, int, int, int);
static void * mfc_encoder_init(int, int, int, int, int);
static void * mfc_encoder_exe(void *, unsigned char *, int, int, long *);
static void mfc_encoder_free(void *);
//static int extract_nal(char *, int, void *);
//static void audio_thread();
static void* video_thread(void*);
static void* video_send_thread(void*);
static void* keep_linked_thread(void*);
//static void get_fb_thread();
//static void main_msg_thread();
static void exit_from_app();
//static void open_alsa_device();
//static void set_alsa_params();
static void ctrl_c(int);

//将C++代码以标准C形式输出（即以C的形式被调用）
#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_CAM_ENCODING_TEST_H__ */
