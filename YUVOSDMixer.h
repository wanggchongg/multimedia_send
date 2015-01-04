
#ifndef YUVOSDMixer_H
#define YUVOSDMixer_H

//////////////////////////////////////////////////////////////////////////

#include "myHead.h"

//////////////////////////////////////////////////////////////////////////

enum
{
	YUV_FMT_YV12 = 0, 
	YUV_FMT_I420 = 1, 
	YUV_FMT_YUYV = 2, 
	YUV_FMT_YVYU = 3, 
	YUV_FMT_UYVY = 4, 
};

enum
{
	FONT_SIZE_12 = 0, 
	FONT_SIZE_16 = 1, 
	FONT_SIZE_24 = 2, 
};

enum
{
	TIME_FMT_BASE	= 0x9000, 
	TIME_FMT_YEAR4	= TIME_FMT_BASE + 1, 
	TIME_FMT_YEAR2	= TIME_FMT_BASE + 2, 
	TIME_FMT_MONTH3	= TIME_FMT_BASE + 3, 
	TIME_FMT_MONTH2	= TIME_FMT_BASE + 4, 
	TIME_FMT_DAY	= TIME_FMT_BASE + 5, 
	TIME_FMT_WEEK3	= TIME_FMT_BASE + 6, 
	TIME_FMT_CWEEK1	= TIME_FMT_BASE + 7, 
	TIME_FMT_HOUR24	= TIME_FMT_BASE + 8, 
	TIME_FMT_HOUR12	= TIME_FMT_BASE + 9, 
	TIME_FMT_MINUTE	= TIME_FMT_BASE + 10, 
	TIME_FMT_SECOND	= TIME_FMT_BASE + 11, 
};

enum
{
	MAX_TEXT_LENGTH	= 128, 
	MAX_TEXT_COUNT	= 4, 
	MAX_MASK_COUNT	= 4, 
};

//////////////////////////////////////////////////////////////////////////

typedef struct TextConfig
{
	int bEnable;
	DWORD x;
	DWORD y;
	DWORD dwFontSize;

	int bAdjustFontLuma;	//	auto adjust font color per 32 frames. two color, white and black.
	BYTE byFontLuma;		//	the value of Y
	BYTE byReserve[3];
	union
	{
		CHAR szText[MAX_TEXT_LENGTH];
		BYTE tFormat[MAX_TEXT_LENGTH];
	};
}TextConfig;

typedef struct MaskConfig
{
	int bEnable;
	myRECT rtMask;
}MaskConfig;

typedef struct MixerConfig
{
	TextConfig timeConfig;
	TextConfig textConfig[MAX_TEXT_COUNT];
	MaskConfig maskConfig[MAX_MASK_COUNT];
}MixerConfig;

typedef struct YUVImage
{
	LPBYTE lpYUVImage;
	DWORD dwPitch;
	DWORD dwHeight;
	DWORD dwYUVFmt;
}YUVImage;

//////////////////////////////////////////////////////////////////////////

HANDLE YOM_Initialize();
DWORD YOM_MixOSD(HANDLE hMixer, const MixerConfig *pConfig, const YUVImage *pYUVImage);
VOID YOM_Uninitialize(HANDLE hMixer);

//////////////////////////////////////////////////////////////////////////

#endif
