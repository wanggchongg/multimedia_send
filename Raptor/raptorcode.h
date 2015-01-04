#include "def.h"
#include "matrix.h"

typedef struct {
	uint32 a;
	uint32 b;
	uint8 d;
}Triple,*triple;


typedef struct {
	uint32 K;
	uint32 S;
	uint32 L;
	uint32 H;
	triple trp;
	mymatrix Amat;
	mymatrix A_1mat;
}RaptorParam,*RParam;


typedef struct {
	uint32 K;
	uint32 S;
	uint32 L;
	uint32 H;
	uint32 M;
	uint32 N;
	mymatrix Amat_dec;
	uint16* list;
}RaptorParam_dec,*RParam_dec;

/*static*/ void trip(int K, int X, triple trp);
/*static*/ uint8 deg(uint32 v);
/*static*/ uint16 gray_m(int j,float k);
/*static*/ uint32 myrand(int x,int i,int m);
/*static*/ void raptor_getLDPC(RParam parameter);
/*static*/ void raptor_getH(RParam parameter);
/*static*/ void raptor_getLT(RParam parameter);
/*static*/ void my_xor(uint8 * x, uint8 * y, uint32 size);

int raptor_reset(uint32 k,RParam parameter);
int raptor_init(uint32 k,RParam parameter);
/*static*/ int raptor_intermediate(RParam parameter);
int raptor_encode(RParam parameter,uint32 R,uint8* input,uint8* intermediate,uint8* output,uint32 size);
void raptor_parameterfree(RParam parameter);
