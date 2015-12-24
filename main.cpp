/*========================================================================
#   FileName: main.c
#     Author: Ruchee
#      Email: my@ruchee.com
#   HomePage: http://www.ruchee.com
# LastChange: 2013-03-13 14:23:34
========================================================================*/
/*Time: 2012/04/10*/
/*Version: 1.7*/
/*Author: Tae*/
/*Description: 1)I-Frame进行LT编码，P-Frame仍只切片发送*/
#include "main.h"
//#include "raptorcode.h"
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <semaphore.h>
#include <unistd.h>
#define K1_MAX 5000
#define T_MAX 2048
#define INDEX_SIZE 40
#define PORT_FB 3333
#define FB_DATA_SIZE 5000
//#define YUV_NO_MAX 50000



/*global variable*/
static int cam_p_fp = -1;
static int cam_c_fp = -1;
static char * win0_fb_addr = NULL;
//static int pre_fb_fd = -1;
//static unsigned char delimiter_h264[4] = {0x00, 0x00, 0x00, 0x01};
//static void * dec_handle;
static char ip[80];
static int gop, qp, res, lcd_width=400, lcd_height=272, with_audio, with_video, with_preview, with_multi_description,
		   with_local, with_fb, with_fec, t_init , k1_max_init=3000;
static int camera_no = 1;
static void *video_handle, *video_handle1;
//static snd_pcm_t *audio_handle;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//结构常量,静态初始化互斥锁

//static int finished = 0;

//int lt_frame = 1; // 用于计算凑齐可以进行lt的帧个数
int LT_R =  20;
int NEW_D = 1;
int M_SLICE = 1;
int time_dely = 30;
int time_dely_save = 30;
int locally_store_264 = 0;
float lose_q = 0.3;
sem_t sem_id,sem_quene,sem_room;//semaphore

typedef struct{
	int frame_no;
	long slice_no;
	int frame_type;
	long F;
	int T;
	int K;
	int R;
	int esi;
	int camera_no;
} Frame_header;

struct frame_info{
	int type;
	int frameNo;
	int count;
	int fragmentLen;
	int fragmentNo;
	int offset;
	int iCnt;
	int pCnt;
};
/***************************************g_yuv处理**************************************/
enum
{
	WIDTH = 480,
	HEIGHT= 272,
};

enum
{
	YUV420Size = WIDTH * HEIGHT * 3 >> 1,
};

BYTE byBuffer[YUV420Size] = { 0 };

enum
{
	STEP = 12,
};

/***************************************g_yuv处理**************************************/

struct quene_node{
	int size;
	struct quene_node* next;
};

struct quene{
	int size;
	quene_node* first;
	quene_node* last;
};

struct yuv_info{
	int length;
	int frameNo;
};

typedef struct Feedback
{
	int type;
	float value;
}feedback;

struct linkmsg
{
	int flag;
	int type;
	double x;
	double y;
	double speed;
	struct timeval collect_time;
};

//udp connection with video_dispaly end
int sockfd;
struct sockaddr_in saddr;
int fds[1], maxfd;
fd_set inset;


//void add_to_quene ( quene *, void*, size_t);
//void get_data(quene * ,char *);

char * sendq;
int savedata, getdata;
FILE* file_v;

/* Main Process */
int main(int argc, char **argv){
	pthread_t /*a_pth, */v_pth, v_s_pth;//, m_c_pth;
	pthread_t k_l_pth;
	int ret, start, found = 0;
	int /*a_id,*/ v_id, v_s_id;//, m_c_msg;
	int k_l_id;
	unsigned int addr = 0;
	char * rgb_for_preview = (char *)malloc(lcd_width*lcd_height*4*sizeof(char));

	struct v4l2_capability cap;
	struct v4l2_input chan;
	struct v4l2_framebuffer preview;
	struct v4l2_pix_format preview_fmt;
	struct v4l2_format codec_fmt;

	int i;


	if(argc%2 == 0){
		printf(">>>>> Wrong number of arguments!\n");
		return 1;
	}
	else{
		strcpy(ip, IP_DEFAULT);
		gop = GOP_DEFAULT;
		qp = QP_DEFAULT;
		res = RES_DEFAULT;
		with_video = WITH_VEDIO_DEFAULT;
		with_audio = WITH_AUDIO_DEFAULT;
		with_preview = WITH_PREVIEW_DEFAULT;
		with_multi_description = WITH_MULTI_DESCRIPTION;
		with_local = WITH_LOCAL_DEFAULT;
		with_fb = WITH_FB_DEFAULT;
		with_fec = WITH_FEC_DEFAULT;
		t_init = T_INIT_DEFAULT;
		k1_max_init = K1_MAX;

		for(i = 1; i < argc; i += 2){
			switch(argv[i][1]){
				case 'i':
					strcpy(ip, argv[i+1]);
					break;
				case 'g':
					gop = atoi(argv[i+1]);
					break;
				case 'q':
					qp = atoi(argv[i+1]);
					break;
				case 'r':
					res = atoi(argv[i+1]);
					break;
				case 'v':
					with_video = atoi(argv[i+1]);
					break;
				case 'a':
					with_audio = atoi(argv[i+1]);
					break;
				case 'p':
					with_preview = atoi(argv[i+1]);
					break;
				//case 'm':
					//with_multi_description = atoi(argv[i+1]);
				//	break;
				case 'l':
					with_local = atoi(argv[i+1]);
					break;
				case 'f':
					with_fb = atoi(argv[i+1]);
					break;
				case 'z':
					with_fec = atoi(argv[i+1]);
					break;
				case 't':
					t_init = atoi(argv[i+1]);
					break;
				case 'k':
					k1_max_init = atoi(argv[i+1]);
					break;
//				case 'b':
//					lt_frame = atoi(argv[i+1]);
//					break;
				case 'R':
					LT_R = atoi(argv[i+1]);
					break;
				case 'D':
					NEW_D = atoi(argv[i+1]);
					break;
				case 's':
					M_SLICE = atoi(argv[i+1]);
					break;
				case 'S':
					locally_store_264 = atoi(argv[i+1]);
				    break;
				case 'd':
					time_dely = atoi(argv[i+1]);
					break;
				case 'Q':
					time_dely_save = atoi(argv[i+1]);
					break;
				case 'c':
				    camera_no = atoi(argv[i+1]);
				    break;

				default:
					printf(">>>>> Wrong arguments!\n");
					return 1;
			}
		}

		switch(res){
			case 4:
				lcd_width = 480;
				lcd_height = 272;
				break;
			case 6:
				lcd_width = 640;
				lcd_height = 480;
				break;
			case 8:
				lcd_width = 800;
				lcd_height = 600;
				break;
			default:
				printf(">>>>> Wrong resolution!\n");
				return 1;
		}
	}//else

	printf("\n");
	printf("\t***********************************\n");
	printf("\t*                                 *\n");
	printf("\t*       IP %15s    -i  *\n", ip);
	printf("\t*                                 *\n");
	printf("\t*       GOP %14d    -g  *\n", gop);
	printf("\t*                                 *\n");
	printf("\t*       QP %15d    -q  *\n", qp);
	printf("\t*                                 *\n");
	printf("\t*       RES %10d*%3d    -r  *\n", lcd_width, lcd_height);
	printf("\t*                                 *\n");
	printf("\t*       VEDIO %12d    -v  *\n", with_video);
	printf("\t*                                 *\n");
	printf("\t*       AUDIO %12d    -a  *\n", with_audio);
	printf("\t*                                 *\n");
	printf("\t*       PREVIEW %10d    -p  *\n", with_preview);
	printf("\t*                                 *\n");
	printf("\t*       LOCAL %12d    -l  *\n", with_local);
	printf("\t*                                 *\n");
	printf("\t*       FEED BACK %8d    -f  *\n", with_fb);
	printf("\t*                                 *\n");
	printf("\t*       TIME DELY %8d    -d  *\n", time_dely);
	printf("\t*                                 *\n");
	printf("\t*	      CAMERA NO %8d    -c  *\n", camera_no);
	printf("\t*                                 *\n");
	printf("\t*       FEC %14d    -z  *\n", with_fec);
	printf("\t*                                 *\n");
	printf("\t*       T_INIT %11d    -t  *\n", t_init);
	printf("\t*                                 *\n");
	printf("\t*       K1_MAX %11d    -k  *\n", k1_max_init);
	printf("\t*                                 *\n");
	printf("\t*       LT_R %13d    -R  *\n", LT_R);
	printf("\t*                                 *\n");
//	printf("\t*       LT_FRAME %9d    -b  *\n", lt_frame);
//	printf("\t*                                 *\n");
	printf("\t*       NEW_D %12d    -D  *\n", NEW_D);
	printf("\t*                                 *\n");
	printf("\t*       M_SLICE %10d    -s  *\n", M_SLICE);
	printf("\t*                                 *\n");
	printf("\t*       L_STORE %10d    -S  *\n", locally_store_264);
	printf("\t*                                 *\n");
	printf("\t*	      TIME_DELY %8d   -d  *\n",time_dely);
	printf("\t*                                 *\n");
	printf("\t***********************************\n");
	printf("\n");

	sleep(3);


	sendq = (char*)malloc((T_MAX+sizeof(Frame_header))*20);
	savedata = getdata = 0;

	bzero(&saddr,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT_V);
	if(inet_pton(AF_INET, ip, &saddr.sin_addr) <= 0){
		printf("[%s] is not a valid IP address\n", ip);
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	connect(sockfd,(struct sockaddr *)&saddr, sizeof(saddr));
	fds[0] = sockfd;
	maxfd = sockfd;

	sem_init(&sem_id,0,1);
	sem_init(&sem_quene,0,0);
	sem_init(&sem_room,0,20);


	//signal(SIGINT, signal_ctrl_c);
	//signal(SIGINT, exit_from_app);


	if(with_preview){
		// Camera preview initialization
		if((cam_p_fp = cam_p_init()) < 0)
		    exit_from_app();

		win0_fb_addr = (char *)addr;

	    // Get capability
	    if((ret = ioctl(cam_p_fp , VIDIOC_QUERYCAP, &cap)) < 0){
		    printf("V4L2 : ioctl on VIDIOC_QUERYCAP failled\n");
		    exit(1);
	    }

	    // Check the type - preview(OVERLAY)
	    if(!(cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)){
		    printf("V4L2 : Can not capture(V4L2_CAP_VIDEO_OVERLAY is false)\n");
		    exit(1);
	    }

	    chan.index = 0;
	    found = 0;

	    while(1){
		    if((ret = ioctl(cam_p_fp, VIDIOC_ENUMINPUT, &chan)) < 0){//视频捕获的应用首先要通过VIDIOC_ENUMINPUT
                                                                //命令来枚举所有可用的输入
			    printf("V4L2 : ioctl on VIDIOC_ENUMINPUT failled !!!!\n");
			    fflush(stdout);
			    break;
		    }

		    // Test channel.type
		    if(chan.type & V4L2_INPUT_TYPE_CAMERA){
			    found = 1;
			    break;
		    }
		    chan.index++;
	    }

	    if(!found)
		    exit_from_app();

	    // Settings for input channel 0 which is channel of webcam
	    chan.type = V4L2_INPUT_TYPE_CAMERA;
        //一个video设备节点可能对应多个视频源，上层调用S_INPUT ioctl在多个cvbs视频输入间切换
	    if((ret = ioctl(cam_p_fp, VIDIOC_S_INPUT, &chan)) < 0){
		    printf("V4L2 : ioctl on VIDIOC_S_INPUT failed\n");
		    fflush(stdout);
		    exit(1);
	    }

	    preview_fmt.width = lcd_width;
	    preview_fmt.height = lcd_height;
	    preview_fmt.pixelformat = LCD_BPP_V4L2;

	    preview.capability = 0;
	    preview.flags = 0;
	    preview.fmt = preview_fmt;

	    // Set up for preview
	    if((ret = ioctl(cam_p_fp, VIDIOC_S_FBUF, &preview)) < 0){
		    printf("V4L2 : ioctl on VIDIOC_S_BUF failed\n");
		    exit(1);
	    }

	    // Preview start
	    start = 1;
	    if((ret = ioctl(cam_p_fp, VIDIOC_OVERLAY, &start)) < 0){
		    printf("V4L2 : ioctl on VIDIOC_OVERLAY failed\n");
		    exit(1);
	    }
	}


	if(with_local == 0){
		// Camera codec initialization
		if((cam_c_fp = cam_c_init()) < 0)
		    exit_from_app();

	    // Get capability
	    if((ret = ioctl(cam_c_fp , VIDIOC_QUERYCAP, &cap)) < 0){
		    printf("V4L2 : ioctl on VIDIOC_QUERYCAP failled\n");
		    exit(1);
	    }

	    // Check the type - preview(OVERLAY)
	    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
		    printf("V4L2 : Can not capture(V4L2_CAP_VIDEO_CAPTURE is false)\n");
		    exit(1);
	    }

	    // Set format
	    codec_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    codec_fmt.fmt.pix.width = lcd_width;
	    codec_fmt.fmt.pix.height = lcd_height;
	    codec_fmt.fmt.pix.pixelformat= V4L2_PIX_FMT_YUV420;
	    if((ret = ioctl(cam_c_fp , VIDIOC_S_FMT, &codec_fmt)) < 0){
		    printf("V4L2 : ioctl on VIDIOC_S_FMT failled\n");
		    exit(1);
	    }
	}



	// Encoding threads creation
	if(with_video){
		v_id = pthread_create(&v_pth, 0, video_thread, 0);

		if(with_local == 0 && locally_store_264 == 1){
			if(!(file_v = fopen("origin_video", "wb+"))){
			    perror("origin_video file open error");
			    exit(1);
		    }
		}
	}

	v_s_id = pthread_create(&v_s_pth, 0, video_send_thread, 0);
	k_l_id = pthread_create(&k_l_pth, 0, keep_linked_thread, 0);

//	if(with_audio)
//		a_id = pthread_create(&a_pth, 0, audio_thread, 0);

	while(with_preview){ // preview
		// Get RGB frame from camera preview
		if(!read_data(cam_p_fp, &rgb_for_preview[0], lcd_width, lcd_height, LCD_BPP)){
			printf("V4L2 : read_data() failed\n");
			break;
		}
		// Write RGB frame to LCD frame buf
		draw(win0_fb_addr, &rgb_for_preview[0], lcd_width, lcd_height, LCD_BPP);
	}


	// Start encoding thread
	if(with_video){
		pthread_join(v_pth, NULL);//使一个线程等待另一个线程结束,linux使用此函数对创建的线程进行资源回收
	}

    pthread_join(v_s_pth, NULL);
	pthread_join(k_l_pth, NULL);

//	if(with_audio)
//		pthread_join(a_pth, NULL);

	exit_from_app();

	return 0;
}


/***************** Vedio Thread *****************/
static void* video_thread(void*){

	/************************g_yuv处理**************************/
	YUVImage yuvImage = { 0 };
	MixerConfig mixerConfig = { 0 };

	yuvImage.dwPitch = WIDTH;
	yuvImage.dwHeight= HEIGHT;
	yuvImage.lpYUVImage= byBuffer;

	//CHAR szDate[] = " ";
	//CHAR szTime[] = " ";
	USHORT dayForamt[]	= { TIME_FMT_YEAR4, '-', TIME_FMT_MONTH2, '-', TIME_FMT_DAY, ' ',TIME_FMT_CWEEK1,' '};
	USHORT timeForamt[]	= { TIME_FMT_HOUR24, ':', TIME_FMT_MINUTE, ':', TIME_FMT_SECOND, };

	//mixerConfig.timeConfig.bEnable = true;
	mixerConfig.timeConfig.bEnable = 1;
	mixerConfig.timeConfig.dwFontSize= FONT_SIZE_16;
	mixerConfig.timeConfig.x = 90;
	mixerConfig.timeConfig.y = 250;

	//mixerConfig.timeConfig.bAdjustFontLuma = false;
	mixerConfig.timeConfig.bAdjustFontLuma = 0;
	mixerConfig.timeConfig.byFontLuma = 0xFF;

	UINT nLength = 0;
	// CopyMemory(&mixerConfig.timeConfig.tFormat[nLength], &szDate, sizeof(szDate));
	// nLength += sizeof(szDate) ;
	CopyMemory(&mixerConfig.timeConfig.tFormat[nLength], dayForamt, sizeof(dayForamt));
	nLength += sizeof(dayForamt);

	// CopyMemory(&mixerConfig.timeConfig.tFormat[nLength], &szTime, sizeof(szTime));
	// nLength += sizeof(szTime) ;
	CopyMemory(&mixerConfig.timeConfig.tFormat[nLength], timeForamt, sizeof(timeForamt));

	nLength += sizeof(timeForamt);

	yuvImage.dwYUVFmt= YUV_FMT_YV12;
/*******************************************************************************/

	char file_name[100];
	struct timeval t_start,t_end;
	long cost_time_sec,cost_time_usec;
	long yuv_no = 1;
	int start, ret;
	int yuv_frame_buf_size = (lcd_width*lcd_height)+(lcd_width*lcd_height)/2;
	unsigned char g_yuv[yuv_frame_buf_size];
	unsigned char * encoded_buf;
	long encoded_buf_size;


	uint32 T = t_init; //default: 128

	long F = 0;
	uint32 K = 0, R = 0;
	unsigned int i;
//	int i, j, k;
	unsigned int symbol_no;

	uint8* input_buf = (uint8*)malloc(K1_MAX*T_MAX);
	uint8* output = (uint8*)malloc(K1_MAX*T_MAX);
	char * output_buf = (char *)malloc(sizeof(Frame_header)+T_MAX);  //output_buf存储生成的每一个encoding symbol
	char * sps_pps = (char *)malloc(30);
	Frame_header * frame_header = (Frame_header *)malloc(sizeof(Frame_header));
	uint8* intermediate = (uint8*)malloc(K1_MAX*T_MAX+100);
	long long loop_times;
	long slice_no = 1;


	RParam para;
	para = (RParam)malloc(sizeof(RaptorParam));

	raptor_init(k1_max_init, para);
	//float recieve;
	//char * fb_data = (char *)malloc(FB_DATA_SIZE*sizeof(char));
	//struct timeval tv;

	FILE * local_video_source_fp = NULL;

	if(with_local){
		if((local_video_source_fp = fopen("origin_video", "rb")) == 0){
			perror("origin_video");
			exit(1);
		}

		loop_times = 3076;  //特定大小
	}
	else loop_times = LONG_MAX;


	// pthread_mutex_lock(&mutex);
	video_handle = mfc_encoder_init(lcd_width, lcd_height, 30, 1000, 30);
	printf(">>>>>> video_handle init\n");


	sprintf(&file_name[0], "Cam_encoding_%dx%d.264", lcd_width, lcd_height);
	fflush(stdout);

	if(with_local == 0){
		// Codec start
	    start = 1;
	    if((ret = ioctl(cam_c_fp, VIDIOC_STREAMON, &start)) < 0){
		    printf("V4L2 : ioctl on VIDIOC_STREAMON failed(start)\n");
		    exit(1);
	    }
	}


	//*** set GOP & QP & slice_num***
	H264_ENC_CONF conf_type = H264_ENC_SETCONF_PARAM_CHANGE;
	int value[2];
	value[0] = H264_ENC_PARAM_GOP_NUM;
	value[1] = gop;
	SsbSipH264EncodeSetConfig(video_handle, conf_type, value);

	value[0] = H264_ENC_PARAM_INTRA_QP;
	value[1] = qp;
	SsbSipH264EncodeSetConfig(video_handle, conf_type, value);

	value[0] = H264_ENC_PARAM_SLICE_MODE;
	value[1] = 4;
	SsbSipH264EncodeSetConfig(video_handle, conf_type, value);


	while(yuv_no < loop_times || with_local == 0){
		signal(SIGINT, (void (*)(int))ctrl_c);
		//read from camera device
		if(with_local == 1){
			if(feof(local_video_source_fp) == 0){
				if(fread(g_yuv, 1, yuv_frame_buf_size, local_video_source_fp) < 0){
					perror("read()");
				}
			}
			else{
					printf("The end!!!!!!!!\n");

					SsbSipH264EncodeDeInit(video_handle);


					exit(1);
			}

			//printf("origin_video reading...\n");
		}
		else{

			if((read(cam_c_fp, g_yuv, yuv_frame_buf_size)) < 0)
				perror("read()");
			else if(locally_store_264 == 1){
				//locally restore
			    fwrite((const unsigned char*)g_yuv, 1, yuv_frame_buf_size, file_v);
			    fflush(file_v);
			}
		}

		/*****************************g_yuv处理*********************************/
		    memcpy(yuvImage.lpYUVImage,g_yuv,YUV420Size);

		    HANDLE hMixer = YOM_Initialize();
		    YOM_MixOSD(hMixer, &mixerConfig, &yuvImage);
		    YOM_Uninitialize(hMixer);

		    memcpy(g_yuv,yuvImage.lpYUVImage,YUV420Size);

		    //存在big.yuv里面，测试到底是不是编码所致的乱码

		    //iOffset = count_F * YUV420Size;
		    //fseek(fp,iOffset,SEEK_SET);
		    //fwrite(g_yuv,1,YUV420Size,fp);
		    //count_F++;

		    /*****************************g_yuv处理*********************************/

		// gettimeofday(&t_start, NULL);
		if(yuv_no % gop == 1){

			printf("\n264 i\n");

			conf_type = H264_ENC_SETCONF_CUR_PIC_OPT;
			value[0] = H264_ENC_PIC_OPT_IDR;
			value[1] = 1;
			SsbSipH264EncodeSetConfig(video_handle, conf_type, value);

			encoded_buf = (unsigned char*)mfc_encoder_exe(video_handle, g_yuv, yuv_frame_buf_size, 1, &encoded_buf_size);

			if(yuv_no == 1){
				memcpy(sps_pps,encoded_buf,21);
			}

			F = encoded_buf_size+21;
			K = (uint32)ceil((double)F/T);

			if(with_fec){
	            			R = (uint32)ceil(K/2);
			        	printf("K = %d  R = %d  LT_R = 50%%\n",(int)K,(int)R);
           			 }
            			else{
	            			memset(input_buf, 0, K*T);
				memcpy(input_buf,sps_pps,21);
				memcpy(input_buf+21,encoded_buf,encoded_buf_size);
           			 }
		}
		else{
			printf("\n264 p\n");

			encoded_buf = (unsigned char*)mfc_encoder_exe(video_handle, g_yuv, yuv_frame_buf_size, 0, &encoded_buf_size);

			F = encoded_buf_size;
			K = (uint32)ceil((double)F/T);
			if(K<5){
				K = 5;
			}

			if(with_fec){
            				R = (uint32)ceil((LT_R*K)/(100-LT_R));
		        		printf("K = %d, R = %d  LT_R = %d%%\n",(int)K,(int)R, LT_R);
           			 }
            			else{
            				memset(input_buf, 0, K*T);
			  	memcpy(input_buf, encoded_buf, encoded_buf_size);
           			 }
		}

		// tv.tv_sec = 0;
		// tv.tv_usec = 0;

		// FD_ZERO(&inset);
		// FD_SET(sockfd, &inset);

		// maxfd = sockfd;

		// if( select(maxfd+1, &inset, NULL, NULL, &tv) > 0){
		// 	if(FD_ISSET(sockfd, &inset))
		// 	{
		// 		read(sockfd, fb_data, sizeof(float));
		// 	    memcpy(&recieve, fb_data, sizeof(float));
		// 	    lose_q = recieve;
		// 	}
		// }


		memset(output_buf,0,sizeof(Frame_header)+T_MAX);//2012
		if(with_fec){

			gettimeofday(&t_start, NULL);
		//	raptor_init(K,para);
		//	R = (uint32)ceil((K+3)/(1-lose_q))-K;
		//	R = 20;
		//	R = (uint32)ceil((LT_R*K)/(100-LT_R));
		//	printf("\nK = %d,cauculate R = %d  lose_q = %2f\n",(int)K,(int)R,lose_q);
		//	printf("\nK = %d, R = %d  LT_R = %d%\n",(int)K,(int)R,LT_R);

			raptor_reset(K,para);

			memset(input_buf, 0, para->L*T);
			if(yuv_no%gop == 1){
				memcpy(input_buf+(para->S+para->H)*T,sps_pps,21);
				memcpy(input_buf+(para->S+para->H)*T+21, encoded_buf, encoded_buf_size);
			}else{
				memcpy(input_buf+(para->S+para->H)*T, encoded_buf, encoded_buf_size);
			}


			int result = raptor_encode(para,R,input_buf,intermediate,output,T);
			if(result == 0){
				printf("encode error!\n");
				//raptor_parameterfree(para);
				if(yuv_no == INT_MAX)
					yuv_no = 0;
				else
				    yuv_no++;

				continue;
			}

			gettimeofday(&t_end, NULL);
			cost_time_sec=t_end.tv_sec-t_start.tv_sec;
			cost_time_usec=t_end.tv_usec-t_start.tv_usec;
			if(cost_time_usec<0){
				cost_time_usec+=1000000;
				cost_time_sec--;
			}
			printf("\n frame :%ld raptor encoder cost time %ld.%06ld s\n",yuv_no, cost_time_sec, cost_time_usec);

			for(symbol_no = 0; symbol_no < K+R; symbol_no++){
				frame_header->frame_no = htonl(yuv_no);
				frame_header->slice_no = htonl(slice_no);
				if(slice_no == LONG_MAX)
					slice_no = 0;
				else
					slice_no++;
				frame_header->frame_type = htonl(1);
				frame_header->F = htonl(F);
				frame_header->T = htonl(T);
				frame_header->K = htonl(K);
				frame_header->R = htonl(R);
				frame_header->esi = htonl(symbol_no);
				frame_header->camera_no = htonl(camera_no);

				memcpy(output_buf, frame_header, sizeof(Frame_header));
				memcpy(output_buf+sizeof(Frame_header), output+symbol_no*T, T);


				sem_wait(&sem_room);
				sem_wait(&sem_id);

				memcpy(sendq+(T+sizeof(Frame_header))*savedata, output_buf, sizeof(Frame_header)+T);
				savedata = (savedata+1)%20;
				sem_post(&sem_quene);
				sem_post(&sem_id);
			}


			//raptor_parameterfree(para);
			if(yuv_no == INT_MAX)
				yuv_no = 0;
			else
				yuv_no++;
		}
		else{

			for(i = 0; i < K; i++){
				frame_header->frame_no = htonl(yuv_no);
				frame_header->slice_no = htonl(slice_no);
				if(slice_no == LONG_MAX)
					slice_no = 0;
				else
					slice_no++;
				frame_header->frame_type = htonl(2);
				frame_header->F = htonl(F);
				frame_header->T = htonl(T);
				frame_header->K = htonl(K);
				frame_header->R = htonl(0);
				frame_header->esi = htonl(i);
                			frame_header->camera_no = htonl(camera_no);

				memcpy(output_buf, frame_header, sizeof(Frame_header));
				memcpy(output_buf+sizeof(Frame_header), input_buf+i*T, T);


				sem_wait(&sem_room);
				sem_wait(&sem_id);

				memcpy(sendq+(T+sizeof(Frame_header))*savedata, output_buf, sizeof(Frame_header)+T);
				savedata = (savedata+1)%20;

				sem_post(&sem_quene);
				sem_post(&sem_id);
			}

		    if(yuv_no == INT_MAX)
				yuv_no = 0;
			else
				yuv_no++;
		}

	}

	if(with_local == 0){
		// Codec stop
	    start = 0;
	    ioctl(cam_c_fp, VIDIOC_STREAMOFF, &start);
	}

	// pthread_mutex_unlock(&mutex);

	return 0;
}


// static long total_data = 0;

static void* video_send_thread(void*){
	int T_INIT = t_init;
	int pLen;

	char output_buf_s[1024];
	char temp_frame[1024];

	int w_n;
	int i;
	int header_size = sizeof(Frame_header);
	unsigned int esi_temp = 0;
	unsigned int frame_no_temp = 0;
	int temp_frame_flag = 0;
	int data_len = sizeof(int);

	double total_dt;

	Frame_header *frame_header = (Frame_header *)malloc(sizeof(Frame_header));

	while(1){

		i = 0;
		pLen = 0;
		memset(output_buf_s, '\0', sizeof(output_buf_s));

		if(temp_frame_flag == 1){
        	memcpy(frame_header, temp_frame, header_size);

        	frame_no_temp = ntohl(frame_header->frame_no);
	        esi_temp = ntohl(frame_header->esi);

//	        printf("frame_no : %d esi : %d size : %d\n", frame_no_temp, esi_temp, header_size+T_INIT);

        	memcpy(output_buf_s, temp_frame, header_size+T_INIT);

            i++;
        	getdata = (getdata+1)%20;
		    pLen = T_INIT + header_size;
		    pLen += data_len;

		    temp_frame_flag = 0;
        }

		while(pLen+T_INIT <= 1024){
			sem_wait(&sem_quene);
		    sem_wait(&sem_id);


		    memcpy(frame_header,sendq+getdata*(T_INIT+header_size),header_size);

	        if(pLen == 0){

	        	frame_no_temp = ntohl(frame_header->frame_no);
	        	esi_temp = ntohl(frame_header->esi);

//	        	printf("frame_no : %d esi : %d size : %d\n", frame_no_temp, esi_temp, header_size+T_INIT);

	        	memcpy(output_buf_s, sendq+getdata*(T_INIT+header_size), header_size+T_INIT);


	        	pLen = T_INIT + header_size;
	        	pLen += data_len;
	        }
	        else{
	        	if(frame_no_temp == ntohl(frame_header->frame_no)){
	        		esi_temp ++;

//	        		printf("frame_no : %d esi : %d size : %d\n", frame_no_temp, esi_temp, header_size+T_INIT);

//	        		memcpy(output_buf_s+header_size+i*T_INIT+data_len, sendq+getdata*(T_INIT+header_size)+header_size, T_INIT);
	        		memcpy(output_buf_s+pLen, sendq+getdata*(T_INIT+header_size)+header_size, T_INIT);


	        	    pLen += T_INIT;
	        	}
	        	else{
	        		temp_frame_flag = 1;
	        		memcpy(temp_frame, sendq+getdata*(T_INIT+header_size), header_size+T_INIT);

                    sem_post(&sem_room);
		            sem_post(&sem_id);

	        		break;
	        	}
	        }

		    sem_post(&sem_room);
		    sem_post(&sem_id);

            i++;
		    getdata = (getdata+1)%20;

		}

		i--;
		i = htonl(i);

        memcpy(output_buf_s+T_INIT+header_size, &i, sizeof(i));

		w_n = write(sockfd, output_buf_s, pLen);

		// total_data += w_n;

		// if(total_data > 1048576)
		// {
		// 	total_dt = (double)total_data/1048576;
		// 	fprintf(stderr, "total send: %ldB = %.3fMB\n",  total_data, total_dt);
		// }
		// else if(total_data > 1024)
		// {
		// 	total_dt = (double)total_data/1024;
		// 	fprintf(stderr, "total send: %ldB = %.3fKB\n",  total_data, total_dt);
		// }
		// else
		// {
		// 	fprintf(stderr, "total send: %ldB\n",  total_data);
		// }


//		printf("this send: %dB, plen = %d i = %d\n", w_n, pLen, ntohl(i));

		usleep(time_dely*1000);
	}

	return 0;
}

static void* keep_linked_thread(void*){
	char msg[] = "###";

	while(1){
		write(sockfd, msg, sizeof(msg));

		sleep(5);
	}

	return 0;
}

//void get_fb_thread(void){
	//feedback * fb = (feedback *)malloc(sizeof(feedback));
	/*float recieve;
	char * fb_data = (char *)malloc(FB_DATA_SIZE*sizeof(char));
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&inset);
	FD_SET(sockfd, &inset);
	while(1){
		if( select(maxfd+1, &inset, NULL, NULL, &tv) > 0){
			read(sockfd, fb_data, sizeof(float));
			memcpy(&recieve, fb_data, sizeof(float));
			lose_q = recieve;
			printf("\n\n\n\nlose_q = %4f\n\n\n\n",lose_q);
		}
	}*/
//}
/*
static void main_msg_thread(void){
	printf("process_4:: I AM PROCESS_4\n");

	char sendbuff[100] = {NULL};

	int sock_fd;
	struct sockaddr_un sock_addr;
	socklen_t sock_len;
	sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	sock_addr.sun_family = AF_UNIX;
	strcpy(sock_addr.sun_path,"path_process_1");
	sock_len = sizeof(sock_addr);

	struct timeval now;
	gettimeofday(&now, NULL);
	struct linkmsg linkmsg_send;
	linkmsg_send.flag = 4;
	linkmsg_send.type = 1;
	linkmsg_send.x = 0.0;
	linkmsg_send.y = 0.0;
	linkmsg_send.collect_time = now;

	memcpy(sendbuff, &linkmsg_send, sizeof(linkmsg_send));

	if((connect(sock_fd,(struct sockaddr *)&sock_addr,sock_len)) == -1)
	{
		printf("process_4:: connect error\n");
	}
	else{
		while(1)
		{
			//printf("process_4:: connect process1 OK!\n");
			write(sock_fd, sendbuff, sizeof(linkmsg_send));
			usleep(1000*1000);
		}
	}
//	return 0;

}
*/
//quene function//
//void add_to_quene(quene* q, void *data, size_t datasize){
//	quene_node* new_node = NULL;
//	void *p;
//	printf("here111111111111111111111111111111111111\n");
//	new_node = (quene_node*) malloc(datasize + sizeof(quene_node)); //多分配了datasize空间,现在是(List+Data)的内存空间
//	printf("here222222222222222222222222222222222222\n");
//	p = (void *)(new_node + 1); //数据指针被藏在了new_list后面一个位置
//	printf("here3333333333333333333333333333333333\n");
//	memcpy(p, data, datasize);  //把数据拷到new_list后面的位置上
//	new_node->size = datasize;
//	new_node->next = NULL;
//	if(q->size > 0){
	//	printf("here44444444444444444444444444444444444\n");
//		q->last->next = new_node;
//		q->last = new_node;
//		q->size++;
//	}
//	else{
//		printf("here5555555555555555555555555555555555555555555\n");
//		q->size++;
//		q->first = q->last = new_node;
//	}
//}

//void get_data(quene * q,char * buf){
//	void *p, *d;
//	p = NULL;
//	d = NULL;
//	Frame_header * frame_header = (Frame_header *)malloc(sizeof(Frame_header));
//	printf("quene length : %d\n",q->size);
//	if(q->first){
//		p = (void *)(q->first + 1);

//		d = q->first;
//		if(q->first->next){
//			printf("what22222222222222222222222222222222222222\n");
//			memcpy(frame_header,p,sizeof(Frame_header));
		//	printf("get data slice_no  111:%d \n    ",ntohl(frame_header->slice_no));
//			memcpy(buf, p, sizeof(Frame_header)+t_init);
//			q->first = q->first->next;
//			q->size--;
//			free(d);
//			d = NULL;
//		}
//		else{
//			printf("what333333333333333333333333333333333333333333\n");
//			memcpy(frame_header,p,sizeof(Frame_header));
		//	printf("get data slice_no  222:%d \n    ",ntohl(frame_header->slice_no));
//			memcpy(buf, p, sizeof(Frame_header)+t_init);
//			q->first = q->first->next;
//			q->size--;
//			free(d);
//			d = NULL;
//		}
//	}


//}
////////////////////////////////////
/*
void audio_thread(void){
	int sockfd;
	struct sockaddr_in saddr;
	snd_pcm_uframes_t period_size = 80;
	snd_pcm_uframes_t buf_size = period_size*2;

	int frame_len = sizeof(struct frame_info)+buf_size;
	unsigned char * frame = (unsigned char *)malloc(frame_len);
	unsigned char * buf = (unsigned char *)malloc(buf_size);

	long frame_cnt = 1;

	//record init
	open_alsa_device();
	set_alsa_params();

	//socket init
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		perror("Socket error");
		exit(1);
	}
	bzero(&saddr, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT_A);
	if(inet_aton(ip, &saddr.sin_addr) < 0){
		perror("IP error");
		exit(1);
	}

	while(!finished){
		signal(SIGINT, (void (*)(int))ctrl_c);
		//record
		while(snd_pcm_readi(audio_handle, buf, period_size) != period_size)
			snd_pcm_prepare(audio_handle);

		//send by udp-socket
		//write frame_info
		struct frame_info frame_info;
		frame_info.type = 1;
		frame_info.frameNo = htonl(frame_cnt++);
		frame_info.count = htonl(buf_size);
		frame_info.fragmentLen = htonl(buf_size);
		frame_info.fragmentNo = htonl(1);

		//fix frame_info & frameData to frame
		memset(frame, 0, frame_len);
		memcpy(frame, &frame_info, sizeof(frame_info));
		memcpy(frame+sizeof(frame_info), buf, buf_size);
		printf("Audio | frameNo=%d, count=%d, fragmentLen=%d, fragmentNo=%d\n", ntohl(frame_info.frameNo), ntohl(frame_info.count), ntohl(frame_info.fragmentLen), ntohl(frame_info.fragmentNo));
		sendto(sockfd, frame, frame_len, 0, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	}
	//snd_pcm_close(audio_handle);
}

*/
/***************** Camera driver function *****************/
static int cam_p_init(void){
	int dev_fp = -1;

	if((dev_fp = open(PREVIEW_NODE, O_RDWR)) < 0){
		perror(PREVIEW_NODE);
		return -1;
	}
	return dev_fp;
}

static int cam_c_init(void){
	int dev_fp = -1;

	if((dev_fp = open(CODEC_NODE, O_RDWR)) < 0){
		perror(CODEC_NODE);
		printf("CODEC : Open Failed \n");
		return -1;
	}
	return dev_fp;
}

static int read_data(int fp, char *buf, int width, int height, int bpp){
	int ret;
	if(bpp == 16){
		if((ret = read(fp, buf, width * height * 2)) != width * height * 2){
			return 0;
		}
	}
	else{
		if((ret = read(fp, buf, width * height * 4)) != width * height * 4){
			return 0;
		}
	}
	return ret;
}

/***************** Display driver function *****************/
/*
static int fb_init(int win_num, int bpp, int x, int y, int width, int height, unsigned int *addr){
	int 			dev_fp = -1;
	int 			fb_size;
	s3c_win_info_t	fb_info_to_driver;

	switch(win_num){
		case 0:
			dev_fp = open(FB_DEV_NAME, O_RDWR);
			break;
		case 1:
			dev_fp = open(FB_DEV_NAME1, O_RDWR);
			break;
		case 2:
			dev_fp = open(FB_DEV_NAME2, O_RDWR);
			break;
		case 3:
			dev_fp = open(FB_DEV_NAME3, O_RDWR);
			break;
		case 4:
			dev_fp = open(FB_DEV_NAME4, O_RDWR);
			break;
		default:
			printf("Window number is wrong\n");
			return -1;
	}

	if(dev_fp < 0){
		perror(FB_DEV_NAME);
		return -1;
	}

	switch(bpp){
		case 16:
			fb_size = width * height * 2;
			break;
		case 24:
			fb_size = width * height * 4;
			break;
		default:
			printf("16 and 24 bpp support");
			return -1;
	}

	if((*addr = (unsigned int) mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fp, 0)) < 0){
		printf("mmap() error in fb_init()");
		return -1;
	}

	fb_info_to_driver.Bpp 		= bpp;
	fb_info_to_driver.LeftTop_x	= x;
	fb_info_to_driver.LeftTop_y	= y;
	fb_info_to_driver.Width 	= width;
	fb_info_to_driver.Height 	= height;

	if(ioctl(dev_fp, SET_OSD_INFO, &fb_info_to_driver)){
		printf("Some problem with the ioctl SET_VS_INFO!!!\n");
		return -1;
	}

	if(ioctl(dev_fp, SET_OSD_START)){
		printf("Some problem with the ioctl START_OSD!!!\n");
		return -1;
	}

	return dev_fp;
}
*/
#define MIN(x,y) ((x)>(y)?(y):(x))
static void draw(char *dest, char *src, int width, int height, int bpp){
	int x, y;
	unsigned long *rgb32;
	unsigned short *rgb16;

	int end_y = height;
	int end_x = MIN(lcd_width, width);

	if(bpp == 16){

#if !defined(LCD_24BIT)
		for(y = 0; y < end_y; y++){
			memcpy(dest + y * lcd_width * 2, src + y * width * 2, end_x * 2);
		}
#else
		for(y = 0; y < end_y; y++){
			rgb16 = (unsigned short *) (src + (y * width * 2));
			rgb32 = (unsigned long *) (dest + (y * lcd_width * 2));

			// TO DO : 16 bit RGB data -> 24 bit RGB data
			for(x = 0; x < end_x; x++){
				*rgb32 = ( ((*rgb16) & 0xF800) << 16 ) | ( ((*rgb16) & 0x07E0) << 8 ) |
					( (*rgb16) & 0x001F );
				rgb32++;
				rgb16++;
			}
		}

#endif
	}
	else if(bpp == 24){
#if !defined(LCD_24BIT)
		for(y = 0; y < end_y; y++){
			rgb32 = (unsigned long *) (src + (y * width * 4));
			rgb16 = (unsigned short *) (dest + (y * lcd_width * 2));

			// 24 bit RGB data -> 16 bit RGB data
			for(x = 0; x < end_x; x++){
				*rgb16 = ( (*rgb32 >> 8) & 0xF800 ) | ( (*rgb32 >> 5) & 0x07E0 ) | ( (*rgb32 >> 3) & 0x001F );
				rgb32++;
				rgb16++;
			}
		}
#else
		for(y = 0; y < end_y; y++){
			memcpy(dest + y * lcd_width * 4, src + y * width * 4, end_x * 4);
		}
#endif
	}
}



/***************** MFC driver function *****************/
void *mfc_encoder_init(int width, int height, int frame_rate, int bitrate, int gop_num){
	int				frame_size;
	void			*handle;
	int				ret;

	frame_size	= (width * height * 3) >> 1;

	handle = SsbSipH264EncodeInit(width, height, frame_rate, bitrate, gop_num);
	if(handle == NULL){
		LOG_MSG(LOG_ERROR, "Test_Encoder", "SsbSipH264EncodeInit Failed\n");
		return NULL;
	}

	ret = SsbSipH264EncodeExe(handle);

	return handle;
}

void *mfc_encoder_exe(void *handle, unsigned char *yuv_buf, int frame_size, int first_frame, long *size){
	unsigned char	*p_inbuf, *p_outbuf;
	int				hdr_size;
	int				ret;

	p_inbuf =(unsigned char*)SsbSipH264EncodeGetInBuf(handle, 0);

	memcpy(p_inbuf, yuv_buf, frame_size);


	ret = SsbSipH264EncodeExe(handle);
	if(first_frame){
		SsbSipH264EncodeGetConfig(handle, H264_ENC_GETCONF_HEADER_SIZE, &hdr_size);
		printf("------hdr_size := %d---------\n", hdr_size);
	}

	p_outbuf = (unsigned char*)SsbSipH264EncodeGetOutBuf(handle, size);

	if(first_frame){
		printf("------I buf_size := %ld---------\n", *size);
	}
	else
	{
		printf("------P buf_size := %ld---------\n", *size);
	}

	return p_outbuf;
}

void mfc_encoder_free(void *handle){
	SsbSipH264EncodeDeInit(handle);
}
/*
int extract_nal(char * buf, int length, void * dec_handle){
	void * pStrmBuf;
	int nFrameLeng = 0;
	FRAMEX_CTX * pFrameExCtx;
	FRAMEX_STRM_PTR file_strm;
	pFrameExCtx = FrameExtractorInit(FRAMEX_IN_TYPE_MEM, delimiter_h264, sizeof(delimiter_h264), 1);
	file_strm.p_start = file_strm.p_cur = (unsigned char *)buf;
	file_strm.p_end = (unsigned char *)(buf + length);
	FrameExtractorFirst(pFrameExCtx, &file_strm);

	if(dec_handle == NULL){
		printf("H264_Dec_Init_Failed.\n");
		return 1;
	}
	pStrmBuf = SsbSipH264DecodeGetInBuf(dec_handle, nFrameLeng);
	if(pStrmBuf == NULL){
		printf("SsbSipH264DecodeGetInBuf Failed.\n");
		return 1;
	}

	unsigned char frame_type[10];
	int nFrameSize;
	FrameExtractorPeek(pFrameExCtx, &file_strm, frame_type, sizeof(frame_type), (int *)&nFrameSize);
	int nal_type = frame_type[5];
	return nal_type;
}
*/
/*static void open_alsa_device(){
	snd_pcm_open(&audio_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
}*/
/*
static void set_alsa_params(){
	snd_pcm_hw_params_t * audio_params;
	unsigned int audio_sample_rate = 8000;

	snd_pcm_hw_params_malloc(&audio_params);
	snd_pcm_hw_params_any(audio_handle, audio_params);
	snd_pcm_hw_params_set_access(audio_handle, audio_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(audio_handle, audio_params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_rate_near(audio_handle, audio_params, &audio_sample_rate, 0);
	snd_pcm_hw_params_set_channels(audio_handle, audio_params, 1);
	snd_pcm_hw_params(audio_handle, audio_params);
	snd_pcm_hw_params_free(audio_params);
	snd_pcm_prepare(audio_handle);
}
*/
static void ctrl_c(int no){
	if(video_handle != NULL){
		mfc_encoder_free(video_handle);
		// snd_pcm_close((snd_pcm_t*)video_handle);

		//close file
		fclose(file_v);
		printf("********Interrupt and free the handle********\n");
	}
	if(video_handle1 != NULL){
		mfc_encoder_free(video_handle1);
		// snd_pcm_close((snd_pcm_t*)video_handle1);
		printf("********Interrupt and free the handle********\n");
	}
	//if(audio_handle != NULL)
	//	snd_pcm_close(audio_handle);
}

static void exit_from_app(){
	int start;
	int fb_size = lcd_width * lcd_height * 4;
	int ret;

	if(with_preview){
		// Stop previewing
	    start = 0;
	    ret = ioctl(cam_p_fp, VIDIOC_OVERLAY, &start);
	    if(ret < 0){
		    printf("V4L2 : ioctl on VIDIOC_OVERLAY failed\n");
		    exit(1);
	    }

	    close(cam_p_fp);
	    munmap(win0_fb_addr, fb_size);
	}


	switch(LCD_BPP){
		case 16:
			fb_size = lcd_width * lcd_height * 2;
			break;
		case 24:
			fb_size = lcd_width * lcd_height * 4;
			break;
		default:
			fb_size = lcd_width * lcd_height * 4;
			printf("LCD supports 16 or 24 bpp\n");
			break;
	}

	if(cam_c_fp){
		close(cam_c_fp);
	}

    if(video_handle){
    	mfc_encoder_free(video_handle);
    }
}


