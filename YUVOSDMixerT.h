
#ifndef YUVOSDMixerT_H
#define YUVOSDMixerT_H

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

#include <time.h>

#include "YUVOSDMixer.h"
#include "CharSet.h"
#include <string.h>
//#include "Utility.h"
//#include "CriticalSection.h"

//////////////////////////////////////////////////////////////////////////

typedef struct DrawParam
{
	LPBYTE pY;
	DWORD x;
	DWORD y;
	LONG lWidth;
	LONG lHeight;

	LONG lFrameIndex;
	LONG lCounter;			//	字符内有效点的计数，
	DWORD dwYShift;
	int bAdjustFontLuma;	//	自动调整字符亮度，

	BYTE byLuma;			//	暂存字符亮度, 
	BYTE byFontLuma;		//	如不自动调整，要设置的字符亮度
	BYTE byReserve[2];
}DrawParam;



typedef struct YUVImageEx YUVImageEx;
struct YUVImageEx : public YUVImage
{
	DWORD dwYShift;
};

enum
{
	COLOR_SWITCH_FACTOR = (0x0001 << 7) - 1, 
};

inline VOID _DrawPoint(LONG x, LONG y, DWORD dwContext)
{
	DrawParam *pParam = (DrawParam *) dwContext;
	LPBYTE pY = pParam->pY + ((pParam->lWidth * (pParam->y + y) + (pParam->x + x)) << (pParam->dwYShift));

	if (!(pParam->bAdjustFontLuma)) {
		*pY = pParam->byFontLuma;
	}
	else {
		if ((pParam->lCounter > 0) || (pParam->lFrameIndex & COLOR_SWITCH_FACTOR)) {
			*pY = pParam->byLuma;
		}
		else {
			*pY = (*pY & 0x80) ? 0x00 : 0xFF;
			pParam->byLuma = *pY;

			++(pParam->lCounter);
		}
	}
}

template <class FunDrawPoint>
void MatchDotMatrix(PCHAR pTxt, DWORD dwTxtWidth, DWORD dwTxtHeight, FunDrawPoint funCallBack, DWORD dwContext)
{	
	BYTE byFlag = 0;
	DWORD dwLineBytes = ((dwTxtWidth >> 3) + ((dwTxtWidth % 0x08) != 0));

	if (pTxt == NULL || funCallBack == NULL) {
		return;
	}

	for (DWORD i = 0; i < dwTxtHeight; ++i) {
		for (DWORD j = 0; j < dwLineBytes; ++j) {
			byFlag = pTxt[i * dwLineBytes + j];
			if (byFlag == 0) {
				continue;
			}

			for (DWORD k = 0; k < 8; ++k) {
				if (byFlag & 0x01) {
					funCallBack(j * 8 + (7 - k), i, dwContext);
				}

				byFlag >>= 1;
				if (byFlag == 0) {
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////



template <DWORD dwUnique>
class YUVOSDMixerT
{	
public:
	YUVOSDMixerT()
		:	m_lCurrentFrame(0)
	{
		ZeroMemory(&m_timeLuma[0], sizeof(m_timeLuma));
		ZeroMemory(&m_textLuma[0], sizeof(m_textLuma));
	}

	DWORD MixOSD(const MixerConfig *pConfig, const YUVImage *pYUVImage)
	{
		if (pConfig == NULL || pYUVImage == NULL) {
			return ERROR_INVALID_PARAMETER;
		}
		//CGuard<CCriticalSection> guard(m_csYUVOSDMixer);

		const MixerConfig &mixerConfig = *pConfig;

		YUVImageEx yuvImage;
		CopyMemory(&yuvImage, pYUVImage, sizeof(yuvImage));

		//int bPlanner = false;
		int bPlanner = 0;
		LPBYTE lpYUVImage = yuvImage.lpYUVImage;
		switch (yuvImage.dwYUVFmt) {
			case YUV_FMT_YV12:
			case YUV_FMT_I420:
		//		bPlanner = true;
				bPlanner = 1;
				yuvImage.dwYShift = 0;
				break;
			case YUV_FMT_UYVY:
				yuvImage.lpYUVImage++;	//	adjust to Y
			case YUV_FMT_YUYV:
			case YUV_FMT_YVYU:
			//	bPlanner = false;
				bPlanner = 0;
				yuvImage.dwYShift = 1;
				break;
			
		}

		LPBYTE lpY = lpYUVImage;
		LPBYTE lpU = NULL;
		LPBYTE lpV = NULL;
		for (DWORD i=0; i<MAX_TEXT_COUNT; ++i) {
			if (mixerConfig.maskConfig[i].bEnable) {
				if (bPlanner) {
					lpU = lpY + (yuvImage.dwPitch * yuvImage.dwHeight);
					lpV = lpU + (yuvImage.dwPitch * yuvImage.dwHeight >> 2);
					DrawMask(lpY, lpU, lpV, yuvImage.dwPitch, yuvImage.dwHeight, mixerConfig.maskConfig[i].rtMask);
				}
				else {
					DrawMask(lpYUVImage, yuvImage.dwPitch, yuvImage.dwHeight, 1, mixerConfig.maskConfig[i].rtMask);
				}
			}
		}

		if (mixerConfig.timeConfig.bEnable) {
			DrawTime(mixerConfig.timeConfig, yuvImage);
		}

		for (DWORD i=0; i<MAX_TEXT_COUNT; ++i) {
			TextPrintEx(mixerConfig.textConfig[i].szText, mixerConfig.textConfig[i], yuvImage, &m_textLuma[i][0]);
		}

		++m_lCurrentFrame;
		return ERROR_SUCCESS;
	}
	VOID DrawTime(const TextConfig &timeConfig, const YUVImageEx &yuvImage)
	{
		time_t curTime;
		time(&curTime);

		CHAR szTime[MAX_TEXT_LENGTH * 4] = { 0 };
		_BuildOSDTime(szTime, sizeof(szTime), (LPCBYTE) &timeConfig.tFormat[0], localtime(&curTime));
		TextPrintEx(szTime, timeConfig, yuvImage, m_timeLuma);
	}
	VOID TextPrintEx(LPCSTR lpszText, const TextConfig &textConfig, const YUVImageEx &yuvImage, LPBYTE lpLuma)
	{
		if (!textConfig.bEnable) {
			return;
		}

		switch (textConfig.dwFontSize) {
			case FONT_SIZE_12:
				TextPrint<6, 12>(yuvImage.lpYUVImage, yuvImage.dwPitch, yuvImage.dwHeight, textConfig.x, textConfig.y, lpszText, textConfig.bAdjustFontLuma, textConfig.byFontLuma, lpLuma, yuvImage.dwYShift, (LPBYTE) &szASC12[0], (LPBYTE) &szHZK12[0]);
				break;
			case FONT_SIZE_16:
				TextPrint<8, 16>(yuvImage.lpYUVImage, yuvImage.dwPitch, yuvImage.dwHeight, textConfig.x, textConfig.y, lpszText, textConfig.bAdjustFontLuma, textConfig.byFontLuma, lpLuma, yuvImage.dwYShift, (LPBYTE) &szASC16[0], (LPBYTE) &szHZK16[0]);
				break;
			case FONT_SIZE_24:
				TextPrint<12, 24>(yuvImage.lpYUVImage, yuvImage.dwPitch, yuvImage.dwHeight, textConfig.x, textConfig.y, lpszText, textConfig.bAdjustFontLuma, textConfig.byFontLuma, lpLuma, yuvImage.dwYShift, (LPBYTE) &szASC24[0], (LPBYTE) &szHZK24[0]);
				break;
		}
	}

	template <LONG FONT_BASE_WIDTH, LONG FONT_HEIGHT>
	VOID TextPrint(LPBYTE pBuffer, DWORD dwPitch, DWORD dwHeight, LONG x, LONG y, PCCH lpszText, int bAdjustFontLuma, BYTE byFontLuma, LPBYTE lpLuma, DWORD dwYStep, PBYTE pASC, PBYTE pHZK)
	{
		PCCH pszTextBase	= lpszText;
		PCCH pszTextCurrent	= pszTextBase;

		DrawParam param;
		param.x = x;
		param.y = y;
		param.pY= pBuffer;
		param.lWidth = dwPitch;
		param.lHeight= dwHeight;
		param.lFrameIndex = m_lCurrentFrame;
		param.dwYShift = dwYStep;
		param.bAdjustFontLuma = bAdjustFontLuma;
		param.byFontLuma = byFontLuma;

		while (*pszTextCurrent) {
			param.lCounter = 0;

			if (*pszTextCurrent > 0) {
				const LONG lOneLineSize	= FONT_BASE_WIDTH / 8 + ((FONT_BASE_WIDTH % 8) != 0);

				if (param.x + FONT_BASE_WIDTH > dwPitch) {
					break;
				}
				if (param.y + FONT_HEIGHT > dwHeight) {
					break;
				}

				param.byLuma = lpLuma[pszTextCurrent - pszTextBase];
				MatchDotMatrix((PCHAR) (&(pASC[(*pszTextCurrent)*(lOneLineSize*FONT_HEIGHT)])), FONT_BASE_WIDTH, FONT_HEIGHT, _DrawPoint, (DWORD) &param);
				lpLuma[pszTextCurrent - pszTextBase] = param.byLuma;

				pszTextCurrent += 1;
				param.x += FONT_BASE_WIDTH;
			}
			else {
				const LONG lOneLineSize	= (FONT_BASE_WIDTH << 1) / 8 + (((FONT_BASE_WIDTH << 1) % 8) != 0);

				if (param.x + (FONT_BASE_WIDTH << 1) > dwPitch) {
					break;
				}
				if (param.y + FONT_HEIGHT > dwHeight) {
					break;
				}

				PCHAR pCH1 = (PCHAR) pszTextCurrent;
				PCHAR pCH2 = pCH1 + 1;

				UCHAR nQH = *pCH1 - 0xA0;
				UCHAR nWH = *pCH2 - 0xA0;
				ULONG lLocation = (94 * (nQH - 1) + (nWH - 1)) * FONT_HEIGHT * lOneLineSize;

				param.byLuma = lpLuma[pszTextCurrent - pszTextBase];
				MatchDotMatrix((PCHAR) (&(pHZK[lLocation])), (FONT_BASE_WIDTH << 1), FONT_HEIGHT, _DrawPoint, (DWORD) &param);
				lpLuma[pszTextCurrent - pszTextBase] = param.byLuma;

				pszTextCurrent += 2;
				param.x += (FONT_BASE_WIDTH << 1);
			}
		}
	}
	VOID DrawMask(LPBYTE pY, LPBYTE pU, LPBYTE pV, DWORD dwPitch, DWORD dwHeight, const myRECT &rtMask)
	{
		DrawMask(pY, dwPitch, dwHeight, 0, rtMask);

		myRECT rect = { 0 };
		rect.top	= rtMask.top >> 1;
		rect.left	= rtMask.left >> 1;
		rect.bottom = rtMask.bottom >> 1;
		rect.right	= rtMask.right >> 1;
		DrawMask(pU, dwPitch>>1, dwHeight>>1, 0, rect);
		DrawMask(pV, dwPitch>>1, dwHeight>>1, 0, rect);
	}
	VOID DrawMask(LPBYTE pBuffer, DWORD dwPitch, DWORD dwHeight, DWORD dwShift, const myRECT &rtMask)
	{
		LPBYTE pBase = pBuffer + ((dwPitch * rtMask.top + rtMask.left) << dwShift);

		DWORD dwMaskWidth = ((rtMask.right > dwPitch) ? dwPitch : rtMask.right) - rtMask.left;
		DWORD dwMaskHeight= ((rtMask.bottom > dwHeight) ? dwHeight : rtMask.bottom) - rtMask.top;

		for (LONG j=rtMask.top; j<rtMask.bottom; ++j) {
			FillMemory(pBase, dwMaskWidth << dwShift, 0x7F);
			pBase += (dwPitch << dwShift);
		}
	}

private:
	static VOID _BuildOSDTime(LPSTR pszTime, size_t nBufferSize, LPCBYTE pInputFormat, struct tm *pTMTime)
	{
		static LPSTR pszWeekcTxt[] = 
		{
			"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 
		};

		if (pszTime == NULL || nBufferSize == 0 || pInputFormat == NULL || pTMTime == 0) {
			return;
		}

		PSHORT pIterator = (PSHORT) pInputFormat;

		CHAR szTimeFormat[MAX_TEXT_LENGTH] = { 0 };
		LPSTR pszTimeFormat = szTimeFormat;

		ZeroMemory(pszTime, nBufferSize);

		PCHAR pChar	= NULL;
		PCHAR pCharF= NULL;
		while (pIterator[0]) {
			switch (pIterator[0] & 0xFFFF) {
				case TIME_FMT_YEAR4:
					strcpy(pszTimeFormat, "%Y");
					break;
				case TIME_FMT_YEAR2:
					strcpy(pszTimeFormat, "%y");
					break;
				case TIME_FMT_MONTH3:
					strcpy(pszTimeFormat, "%b");
					break;
				case TIME_FMT_MONTH2:
					strcpy(pszTimeFormat, "%m");
					break;
				case TIME_FMT_DAY:
					strcpy(pszTimeFormat, "%d");
					break;
				case TIME_FMT_WEEK3:
					strcpy(pszTimeFormat, "%a");
					break;
				case TIME_FMT_CWEEK1:
					strcpy(pszTimeFormat, pszWeekcTxt[pTMTime->tm_wday]);
					break;
				case TIME_FMT_HOUR24:
					strcpy(pszTimeFormat, "%H");
					break;
				case TIME_FMT_HOUR12:
					strcpy(pszTimeFormat, "%I");
					break;
				case TIME_FMT_MINUTE:
					strcpy(pszTimeFormat, "%M");
					break;
				case TIME_FMT_SECOND:
					strcpy(pszTimeFormat, "%S");
					break;
				default:
					pChar	= (PCHAR) pIterator;
					pCharF	= (PCHAR) pszTimeFormat;
					if (pChar[0] != 0) {
						pCharF[0] = pChar[0];
					}
					if (pChar[1] != 0) {
						pCharF[1] = pChar[1];
					}
					break;
			}

			pszTimeFormat += strlen(pszTimeFormat);
			++pIterator;
		}

		strftime(pszTime, nBufferSize, szTimeFormat, pTMTime);
	}

private:
	LONG m_lCurrentFrame;

	BYTE m_timeLuma[MAX_TEXT_LENGTH];
	BYTE m_textLuma[MAX_TEXT_COUNT][MAX_TEXT_LENGTH];

	//CCriticalSection m_csYUVOSDMixer;
};

//////////////////////////////////////////////////////////////////////////

typedef YUVOSDMixerT<0> CYUVOSDMixer;

//////////////////////////////////////////////////////////////////////////

#endif
