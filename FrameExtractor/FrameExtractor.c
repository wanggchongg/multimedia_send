#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FrameExtractor.h"

//
// FRAMEX_CTX *FrameExtractorInit(unsigned char delimiter[], int delim_leng, int delim_insert)
//
// Description
//		This function initializes the FRAMEX_CTX structure with the delimiter
// Return
//		FRAME_CTX* : FRAMEX_CTX structure. It should be released by FrameExtractorFinal() function.
// Parameters
//		delimiter    [IN]: pointer to the array that holds the delimiter octets.
//		delim_leng   [IN]: length of the delimiter octets
//		delim_insert [IN]: 0-delimiter�� outbuf�� ���� ����. 1-delimiter�� outbuf�� ����
//
FRAMEX_CTX *FrameExtractorInit(FRAMEX_IN_TYPE type, unsigned char delimiter[], int delim_leng, int delim_insert)
{
	FRAMEX_CTX *pCTX;

	// parameter checking
	if (delimiter == NULL || (delim_leng <= 0 || delim_leng > QUEUE_CAPACITY))
		return NULL;


	pCTX = (FRAMEX_CTX *) malloc(sizeof(FRAMEX_CTX));
	memset(pCTX, 0, sizeof(FRAMEX_CTX));

	pCTX->in_type    = type;
	pCTX->delim_ptr  = (unsigned char *) malloc(delim_leng);
	pCTX->delim_leng = delim_leng;
	memcpy(pCTX->delim_ptr, delimiter, delim_leng);

	pCTX->delim_insert = delim_insert;
	pCTX->cont_offset  = 0;

	return pCTX;
}


#define Q_INIT(queue, qp_s, qp_e, q_size, q_capacity)	\
{	\
	memset(queue, 0xFF, q_capacity);	\
	qp_s = qp_e = q_size = 0;			\
}


#define Q_PUSH(value, queue, qp_s, qp_e, q_size, q_capacity)	\
{	\
	queue[qp_s] = value;	\
	qp_s = (qp_s + 1) % q_capacity;	\
	q_size++;	\
}

#define Q_POP(value, queue, qp_s, qp_e, q_size, q_capacity)	\
{	\
	value = queue[qp_e];	\
	queue[qp_e] = 0xFE;		\
	qp_e = (qp_e + 1) % q_capacity;	\
	q_size--;	\
}

#define Q_PEEK(value, queue, qp_s, qp_e)	\
{	\
	value = queue[qp_e];	\
}

// ������ delimiter�� ã�´�.
static int next_delimiter(FRAMEX_CTX *pCTX, FILE *fpin, unsigned char *outbuf, const int outbuf_size, int *n_fill)
{
	int r = 0;
	int l;
	int nbytes_to_write;
	int qp_s, qp_e, q_size;
	unsigned char queue[QUEUE_CAPACITY], b, c;

	int offset;

	offset = 0;
	nbytes_to_write = outbuf_size;
	Q_INIT(queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)

	// ������ outbuf ������ �� �տ� delimiter�� ä���ִ´�.
	if (outbuf != NULL) {
		if (pCTX->cont_offset == 0) {
			if (nbytes_to_write <= pCTX->delim_leng) {
				return FRAMEX_ERR_BUFSIZE_TOO_SMALL;
			}

			if (pCTX->delim_insert) {
				memcpy(outbuf, pCTX->delim_ptr, pCTX->delim_leng);
				outbuf += pCTX->delim_leng;
				nbytes_to_write -= pCTX->delim_leng;	// ������ ũ�⸦ delimiter ���̸�ŭ ������
				*n_fill = pCTX->delim_leng;
			}
			else
				*n_fill = 0;
		}
		else {
			// continue�� ���� ���� (pCTX->cont_offset != 0)
			offset = pCTX->cont_offset;
			outbuf += (pCTX->cont_offset - pCTX->delim_leng);

			for (l=0; l<pCTX->delim_leng; l++) {
				Q_PUSH(*outbuf, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
				outbuf++;
			}

		}
	}

	for (l=0; l<pCTX->delim_leng; l++) {
		if (q_size == 0 || l == q_size) {

			if (outbuf != NULL && offset >= nbytes_to_write) {
				if (pCTX->cont_offset == 0)
					pCTX->cont_offset = (pCTX->delim_leng + offset);
				else
					pCTX->cont_offset = offset;
				return FRAMEX_CONTINUE;
			}

			r = fread(&b, 1, 1, fpin);
			if (r != 1)
				break;
			offset++;
			if (outbuf != NULL) {
				*outbuf = b; outbuf++; (*n_fill)++;
			}
		}
		else {
			r = -1;
			Q_PEEK(b, queue, qp_s, qp_e + l)
		}


		if (b == pCTX->delim_ptr[l]) {
			if (r == 1)	// ���� ���� ��쿡�� PUSH�Ѵ�.
				Q_PUSH(b, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
		}
		else {
			if (r != 1)
				// ���� ���� ��찡 �ƴ϶��, �ϳ� ��������.
				Q_POP(c, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
			else if (l > 0) {
				// ���� ���� ��� �߿���, delimiter�� ù octet(l=0)�� ���ϴ� ����
				// QUEUE�� ���� �ʰ� �ٷ� �Ѿ��.
				// 2��° octet�̻�(l>0)�� ���ϴ� ����,
				// �� �տ� �ϳ��� ���� �� �ڿ� ���� �ϳ��� �ִ´�.

				Q_POP(c, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
				Q_PUSH(b, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
			}

			l = -1;
		}

	}

	if (r == 1) {
		if (outbuf != NULL) {
			*n_fill -= pCTX->delim_leng;
			pCTX->cont_offset = 0;
		}

		return FRAMEX_OK;
	}
	else if (r == 0)
		return FRAMEX_ERR_EOS;

	return FRAMEX_ERR_NOTFOUND;
}


// ������ frame���� ���ϴ� ���� octets ���� Ȯ�θ� �Ѵ�.
static int next_frame_peek(FRAMEX_CTX *pCTX, FILE *fpin, unsigned char *peekbuf, const int peek_size, int *n_fill)
{
	int r = 0;
	int l;
	int nbytes_to_peek;
	int qp_s, qp_e, q_size;
	unsigned char queue[QUEUE_CAPACITY], b, c;

	int offset;

	offset = 0;
	nbytes_to_peek = peek_size;
	Q_INIT(queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)

	// ������ outbuf ������ �� �տ� delimiter�� ä���ִ´�.
	if (peekbuf != NULL) {
		if (pCTX->cont_offset == 0) {
			if (nbytes_to_peek <= pCTX->delim_leng) {
				return FRAMEX_ERR_BUFSIZE_TOO_SMALL;
			}

			if (pCTX->delim_insert) {
				memcpy(peekbuf, pCTX->delim_ptr, pCTX->delim_leng);
				peekbuf += pCTX->delim_leng;
				nbytes_to_peek -= pCTX->delim_leng;	// ������ ũ�⸦ delimiter ���̸�ŭ ������
				*n_fill = pCTX->delim_leng;
			}
			else
				*n_fill = 0;
		}
		else {
			// continue�� ���� ���� ERROR��.

		}
	}

	for (l=0; l<pCTX->delim_leng; l++) {
		if (q_size == 0 || l == q_size) {

			r = fread(&b, 1, 1, fpin);
			if (r != 1)
				break;
			offset++;
			if (peekbuf != NULL) {
				*peekbuf = b; peekbuf++; (*n_fill)++;
			}

			// fread ������ peek_size�� �Ǹ�, Ż���Ѵ�.
			if (offset >= nbytes_to_peek)
				break;
		}
		else {
			r = -1;
			Q_PEEK(b, queue, qp_s, qp_e + l)
		}


		if (b == pCTX->delim_ptr[l]) {
			if (r == 1)	// ���� ���� ��쿡�� PUSH�Ѵ�.
				Q_PUSH(b, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
		}
		else {
			if (r != 1)
				// ���� ���� ��찡 �ƴ϶��, �ϳ� ��������.
				Q_POP(c, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
			else if (l > 0) {
				// ���� ���� ��� �߿���, delimiter�� ù octet(l=0)�� ���ϴ� ����
				// QUEUE�� ���� �ʰ� �ٷ� �Ѿ��.
				// 2��° octet�̻�(l>0)�� ���ϴ� ����,
				// �� �տ� �ϳ��� ���� �� �ڿ� ���� �ϳ��� �ִ´�.

				Q_POP(c, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
				Q_PUSH(b, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
			}

			l = -1;
		}

	}

	if (r == 1) {
		if (peekbuf != NULL) {
			*n_fill -= pCTX->delim_leng;
			pCTX->cont_offset = 0;
		}

		fseek(fpin, -(peek_size-pCTX->delim_leng), SEEK_CUR);

		return FRAMEX_OK;
	}
	else if (r == 0)
		return FRAMEX_ERR_EOS;

	return FRAMEX_ERR_NOTFOUND;
}


// ������ delimiter�� ã�´�.
static int next_delimiter_mem(FRAMEX_CTX *pCTX, FRAMEX_STRM_PTR *strm_ptr, unsigned char *outbuf, const int outbuf_size, int *n_fill)
{
	int r = 0;
	int l;
	int nbytes_to_write;
	int qp_s, qp_e, q_size;
	unsigned char queue[QUEUE_CAPACITY], b, c;

	int offset;

	offset = 0;
	nbytes_to_write = outbuf_size;
	Q_INIT(queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)

	// ������ outbuf ������ �� �տ� delimiter�� ä���ִ´�.
	if (outbuf != NULL) {
		if (pCTX->cont_offset == 0) {
			if (nbytes_to_write <= pCTX->delim_leng) {
				return FRAMEX_ERR_BUFSIZE_TOO_SMALL;
			}

			if (pCTX->delim_insert) {
				memcpy(outbuf, pCTX->delim_ptr, pCTX->delim_leng);
				outbuf += pCTX->delim_leng;
				nbytes_to_write -= pCTX->delim_leng;	// ������ ũ�⸦ delimiter ���̸�ŭ ������
				*n_fill = pCTX->delim_leng;
			}
			else
				*n_fill = 0;
		}
		else {
			// continue�� ���� ���� (pCTX->cont_offset != 0)
			offset = pCTX->cont_offset;
			outbuf += (pCTX->cont_offset - pCTX->delim_leng);

			for (l=0; l<pCTX->delim_leng; l++) {
				Q_PUSH(*outbuf, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
				outbuf++;
			}

		}
	}

	for (l=0; l<pCTX->delim_leng; l++) {
		if (q_size == 0 || l == q_size) {

			if (outbuf != NULL && offset >= nbytes_to_write) {
				if (pCTX->cont_offset == 0)
					pCTX->cont_offset = (pCTX->delim_leng + offset);
				else
					pCTX->cont_offset = offset;
				return FRAMEX_CONTINUE;
			}

			if ((int) strm_ptr->p_cur > (int) strm_ptr->p_end)
				break;
			b = *(strm_ptr->p_cur); strm_ptr->p_cur++; r = 1;
			offset++;
			if (outbuf != NULL) {
				*outbuf = b; outbuf++; (*n_fill)++;
			}
		}
		else {
			r = -1;
			Q_PEEK(b, queue, qp_s, qp_e + l)
		}


		if (b == pCTX->delim_ptr[l]) {
			if (r == 1)	// ���� ���� ��쿡�� PUSH�Ѵ�.
				Q_PUSH(b, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
		}
		else {
			if (r != 1)
				// ���� ���� ��찡 �ƴ϶��, �ϳ� ��������.
				Q_POP(c, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
			else if (l > 0) {
				// ���� ���� ��� �߿���, delimiter�� ù octet(l=0)�� ���ϴ� ����
				// QUEUE�� ���� �ʰ� �ٷ� �Ѿ��.
				// 2��° octet�̻�(l>0)�� ���ϴ� ����,
				// �� �տ� �ϳ��� ���� �� �ڿ� ���� �ϳ��� �ִ´�.

				Q_POP(c, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
				Q_PUSH(b, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
			}

			l = -1;
		}

	}

	if (r == 1) {
		if (outbuf != NULL) {
			*n_fill -= pCTX->delim_leng;
			pCTX->cont_offset = 0;
		}

		return FRAMEX_OK;
	}
	else if (r == 0)
		return FRAMEX_ERR_EOS;

	return FRAMEX_ERR_NOTFOUND;
}


// ������ frame���� ���ϴ� ���� octets ���� Ȯ�θ� �Ѵ�.
static int next_frame_peek_mem(FRAMEX_CTX *pCTX, FRAMEX_STRM_PTR *strm_ptr, unsigned char *peekbuf, const int peek_size, int *n_fill)
{
	int r = 0;
	int l;
	int nbytes_to_peek;
	int qp_s, qp_e, q_size;
	unsigned char queue[QUEUE_CAPACITY], b, c;

	int offset;

	offset = 0;
	nbytes_to_peek = peek_size;
	Q_INIT(queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)

	// ������ outbuf ������ �� �տ� delimiter�� ä���ִ´�.
	if (peekbuf != NULL) {
		if (pCTX->cont_offset == 0) {
			if (nbytes_to_peek <= pCTX->delim_leng) {
				return FRAMEX_ERR_BUFSIZE_TOO_SMALL;
			}

			if (pCTX->delim_insert) {
				memcpy(peekbuf, pCTX->delim_ptr, pCTX->delim_leng);
				peekbuf += pCTX->delim_leng;
				nbytes_to_peek -= pCTX->delim_leng;	// ������ ũ�⸦ delimiter ���̸�ŭ ������
				*n_fill = pCTX->delim_leng;
			}
			else
				*n_fill = 0;
		}
		else {
			// continue�� ���� ���� ERROR��.

		}
	}

	for (l=0; l<pCTX->delim_leng; l++) {
		if (q_size == 0 || l == q_size) {

			if ((int) strm_ptr->p_cur > (int) strm_ptr->p_end)
				break;
			b = *(strm_ptr->p_cur); strm_ptr->p_cur++; r = 1;
			offset++;
			if (peekbuf != NULL) {
				*peekbuf = b; peekbuf++; (*n_fill)++;
			}

			// fread ������ peek_size�� �Ǹ�, Ż���Ѵ�.
			if (offset >= nbytes_to_peek)
				break;
		}
		else {
			r = -1;
			Q_PEEK(b, queue, qp_s, qp_e + l)
		}


		if (b == pCTX->delim_ptr[l]) {
			if (r == 1)	// ���� ���� ��쿡�� PUSH�Ѵ�.
				Q_PUSH(b, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
		}
		else {
			if (r != 1)
				// ���� ���� ��찡 �ƴ϶��, �ϳ� ��������.
				Q_POP(c, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
			else if (l > 0) {
				// ���� ���� ��� �߿���, delimiter�� ù octet(l=0)�� ���ϴ� ����
				// QUEUE�� ���� �ʰ� �ٷ� �Ѿ��.
				// 2��° octet�̻�(l>0)�� ���ϴ� ����,
				// �� �տ� �ϳ��� ���� �� �ڿ� ���� �ϳ��� �ִ´�.

				Q_POP(c, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
				Q_PUSH(b, queue, qp_s, qp_e, q_size, QUEUE_CAPACITY)
			}

			l = -1;
		}

	}

	if (r == 1) {
		if (peekbuf != NULL) {
			*n_fill -= pCTX->delim_leng;
			pCTX->cont_offset = 0;
		}

//		fseek(fpin, -(peek_size-pCTX->delim_leng), SEEK_CUR);
		strm_ptr->p_cur = (strm_ptr->p_cur  -  (peek_size-pCTX->delim_leng));

		return FRAMEX_OK;
	}
	else if (r == 0)
		return FRAMEX_ERR_EOS;

	return FRAMEX_ERR_NOTFOUND;
}


//
// int FrameExtractorFirst(FRAMEX_CTX *pCTX, FILE *fpin)
//
// Description
//		���� ó���� delimiter�� ������ ���� ã�´�.
// Return
//
// Parameters
//
int FrameExtractorFirst(FRAMEX_CTX *pCTX, FRAMEX_IN in)
{
	if (pCTX == NULL)
		return FRAMEX_INVALID_PARAM;
	
	if (pCTX->in_type == FRAMEX_IN_TYPE_FILE)
		return next_delimiter(pCTX, (FILE *) in, NULL, 0, NULL);
	else
		return next_delimiter_mem(pCTX, (FRAMEX_STRM_PTR *) in, NULL, 0, NULL);
}

//
// int FrameExtractorNext(FRAMEX_CTX *pCTX, FILE *fpin, unsigned char outbuf[], int outbuf_size, int *n_fill)
//
// Description
//		FrameExtractorFirst() �Լ��� ȣ���Ͽ� ù frame�� delimiter�� ã�� �ĺ��� ȣ���Ѵ�.
//		���� frame�� delimiter�� ���ö������� frame�� �����Ͽ�, outbuf�� �����Ѵ�.
// Return
//
// Parameters
//		outbuf     [OUT]: pointer to the array that will be filled with the frame data
//		outbuf_size[IN] : length of the outbuf size
//		n_fill     [OUT]: number of octets filled in the out_buf
//
int FrameExtractorNext(FRAMEX_CTX *pCTX, FRAMEX_IN in, unsigned char outbuf[], int outbuf_size, int *n_fill)
{
	if (pCTX == NULL)
		return FRAMEX_INVALID_PARAM;
	
	if (pCTX->in_type == FRAMEX_IN_TYPE_FILE)
		return next_delimiter(pCTX, (FILE *) in, outbuf, outbuf_size, n_fill);
	else
		return next_delimiter_mem(pCTX, (FRAMEX_STRM_PTR *) in, outbuf, outbuf_size, n_fill);
}


//
// int FrameExtractorPeek(FRAMEX_CTX *pCTX, FILE *fpin, unsigned char peekbuf[], int peek_size, int *n_fill)
//
// Description
//		FrameExtractorFirst() �Լ��� ȣ���Ͽ� ù frame�� delimiter�� ã�� �ĺ��� ȣ���Ѵ�.
//		���� frame�� ���� peek_size��ŭ peekbuf�� �����Ѵ�.
//		�� ��, Buffer�� PEEK�� �ϴ� ���̹Ƿ�, frame�� �����ϴ� ���� �ƴϴ�.
// Return
//
// Parameters
//		peekbuf    [OUT]: pointer to the array that will be filled with the frame data
//		peek_size  [IN] : length of the outbuf size
//		n_fill     [OUT]: number of octets filled in the out_buf
//
int FrameExtractorPeek(FRAMEX_CTX *pCTX, FRAMEX_IN in, unsigned char peekbuf[], int peek_size, int *n_fill)
{
	if (pCTX == NULL)
		return FRAMEX_INVALID_PARAM;
	
	if (pCTX->in_type == FRAMEX_IN_TYPE_FILE)
		return next_frame_peek(pCTX, (FILE *) in, peekbuf, peek_size, n_fill);
	else
		return next_frame_peek_mem(pCTX, (FRAMEX_STRM_PTR *) in, peekbuf, peek_size, n_fill);
}



//
// int FrameExtractorFinal(FRAMEX_CTX *pCTX)
//
// Description
//		FRAMEX_CTX ����ü �����͸� �����Ѵ�.
// Return
//
// Parameters
//
int FrameExtractorFinal(FRAMEX_CTX *pCTX)
{
	if (pCTX == NULL)
		return FRAMEX_INVALID_PARAM;
	
	free(pCTX->delim_ptr);
	free(pCTX);

	return 0;
}
