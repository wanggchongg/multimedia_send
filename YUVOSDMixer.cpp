
#include "YUVOSDMixer.h"
#include "YUVOSDMixerT.h"

//////////////////////////////////////////////////////////////////////////

//BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
//{
//	switch (dwReasonForCall) {
//		case DLL_PROCESS_ATTACH:
//		case DLL_THREAD_ATTACH:
//		case DLL_THREAD_DETACH:
//		case DLL_PROCESS_DETACH:
//			break;
//	}
//    return TRUE;
//}

//////////////////////////////////////////////////////////////////////////

HANDLE YOM_Initialize()
{
	return new CYUVOSDMixer;
}
DWORD YOM_MixOSD(HANDLE hMixer, const MixerConfig *pConfig, const YUVImage *pYUVImage)
{
	CYUVOSDMixer *pMixer = (CYUVOSDMixer *) hMixer;
	return pMixer->MixOSD(pConfig, pYUVImage);
}
VOID YOM_Uninitialize(HANDLE hMixer)
{
	CYUVOSDMixer *pMixer = (CYUVOSDMixer *) hMixer;
	delete pMixer;
	pMixer = NULL;
}
