#include "def.h"
#include <malloc.h>
#include <math.h>
#include <string.h>

#define LEN_INT 4;
typedef struct {
	uint32 row;
	uint32 colum;
	//uint32 rowlen;
	//uint8 * data;
	uint8 ** rowpoint;
}MyMatrix,*mymatrix;

/*static*/ uint32 search_col_1(uint8** mat, uint32 start,uint32 row);
/*static*/ void row_exchange(uint32 row, uint32 col, uint8** mat,uint32 matsize);
/*static*/ void row_or(uint32 row1,uint32 row2,uint8** mat,uint32 matsize);
/*static*/ void matrix_free(uint8** mat,uint32 size);
int matrix_reset(uint32 rownum , uint32 columnum, mymatrix mymat);
int matrix_init(uint32 rownum, uint32 columnum, mymatrix mat);
int matrix_inverse(mymatrix A,mymatrix A_1);

