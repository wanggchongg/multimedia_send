
#ifndef _MYHEAD_H_
#define _MYHEAD_H_ 

#include <stdio.h>

#define VOID void

typedef char CHAR;

typedef short SHORT;

typedef long LONG;

typedef int INT;

typedef unsigned long       DWORD;

typedef unsigned char       BYTE;

typedef unsigned short      WORD;

typedef float               FLOAT;

typedef FLOAT               *PFLOAT;

typedef BYTE                *LPBYTE;

typedef  const CHAR 		*LPCSTR;

typedef unsigned long 		ULONG;

typedef unsigned short 		USHORT;

typedef unsigned char 		UCHAR;

typedef unsigned int        UINT;

typedef unsigned int        *PUINT;

typedef const BYTE 			*LPCBYTE;

typedef CHAR 				*LPSTR;


typedef struct myRECT

{

    LONG    left;

    LONG    top;

    LONG    right;

    LONG    bottom;

}myRECT;

typedef void 				*HANDLE;

typedef BYTE           		*PBYTE;



typedef CHAR *PCHAR, *LPCH, *PCH;

typedef const CHAR  *LPCCH, *PCCH;

typedef SHORT *PSHORT;  

typedef LONG *PLONG; 



#define RtlEqualMemory(Destination,Source,Length) (!memcmp((Destination),(Source),(Length)))

#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))

#define RtlCopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))

#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))

#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))

#define MoveMemory RtlMoveMemory

#define CopyMemory RtlCopyMemory

#define FillMemory RtlFillMemory

#define ZeroMemory RtlZeroMemory

#define SecureZeroMemory RtlSecureZeroMemory

#define ERROR_INVALID_PARAMETER          87L

#define ERROR_SUCCESS                    0L


#endif