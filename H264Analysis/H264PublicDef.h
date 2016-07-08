#ifndef H264PUBLICDEF_H
#define H264PUBLICDEF_H

typedef struct MetaData{
	MetaData() 
	{ 
		strcpy(name,"NONE");
		num = 0; 
		bytes = 0.0;
	};
	char name[20];
	unsigned int num;
	float bytes;
}MetaData,*PMetaData;

enum 
{
	NALU = 0,
	P = 1,
	B = 2,
	I = 3,
	SI = 4,
	SP = 5,
	SPS = 6,
	PPS = 7,
	IDR = 8,
	ParamNum = 9
};

//
// �ֽ�λ�ߵ�(��: 0011 0001 => 1000 1100)
// 
unsigned char bits_reverse(unsigned char data);

//
// ���ֽ�λ�ߵ�(��: 0011 0001 0111 1111 => 1111 1110 1000 1100)
// 
char* bits_reverse(char *data, int len);


//
// ���ֽڶ��뷵���ֽ���
//
int bits_get_byte_num(int len);


//
// ��c�Զ�������ʽ���, ��ʽΪ���ҵ���(���λ�����λ)
//
// void bits_binary_printf(char c);

//
// ��data�Զ�������ʽ���, ��ʽΪ���ҵ���(���λ�����λ)
//
// void bits_binary_printf(char *data, int len);

//
// ��data��ʮ��������ʽ���, С�˴洢�ķ�ʽ
//
// void bytes_hex_printf(char *data, int len);

//
// ��ȡ��ȡ�ֽڵ�posλ��, ����Ϊlenλ���ֽ�����
//
const char B8_VAL_MASK(unsigned int pos, unsigned int len);

//
// ��ȡ��ȡ���ֽڵ�posλ��, ����Ϊnλ���ֽ�����
//
const UINT32 B32_VAL_MASK(unsigned int pos, unsigned int len);


//
// ��ȡ��ȡ�ֽڵ�posλ��ʼ���������ֽ�����
//
const char B8_VAL_MASK(unsigned int pos);

//
// ��ȡ��ȡ���ֽڵ�posλ��ʼ���������ֽ�����
//
const UINT32 B32_VAL_MASK(unsigned int pos);

//
// ��ȡ��ȡ�ֽ�val��posλ�𵽽�����ֵ�����������ұߵ�һλ��0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos);

//
// ��ȡ��ȡ���ֽ�val��posλ�𵽽�����ֵ�����������ұߵ�һλ��0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos);


//
// ��ȡ��ȡ�ֽ�val��posλ��, ����Ϊnλ���ֽڲ��������ұߵ�һλ��0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos, unsigned int len);

//
// ��ȡ��ȡ���ֽ�val��posλ��, ����Ϊnλ���ֽڲ��������ұߵ�һλ��0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos, unsigned int len);

//
// ��ȡ��ȡ�ֽ�val��posλ�𵽽�����ֵ������������ߵ�һλ��0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos);

//
// ��ȡ��ȡ���ֽ�val��posλ�𵽽�����ֵ������������ߵ�һλ��0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos);

//
// ��ȡ��ȡ�ֽ�val��posλ��, ����Ϊnλ���ֽڲ���������ߵ�һλ��0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos, unsigned int len);

//
// ��ȡ��ȡ���ֽ�val��posλ��, ����Ϊnλ���ֽڲ���������ߵ�һλ��0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos, unsigned int len);

//
// �ֽ�val��posλ��ʼ��lenλ����(����������)
//
const char B8_VAL_ZERO(unsigned char val, unsigned int pos, unsigned int len);

//
// ���ֽ�val��posλ��ʼ��lenλ����(����������)
//
const UINT32 B32_VAL_ZERO(UINT32 val, unsigned int pos, unsigned int len);

//
// �ֽ�val��posλ��ʼ��lenλ��һ(����������)
//
const char B8_VAL_ONE(unsigned char val, unsigned int pos, unsigned int len);

//
// ���ֽ�val��posλ��ʼ��lenλ��һ(����������)
//
const UINT32 B32_VAL_ONE(UINT32 val, unsigned int pos, unsigned int len);

//
// ���ֽ�srcVal�н�ȡsrcPos��ʼ��lenλ������䵽�ֽ�dstVal�д�dstPos��ʼ��lenλ(����������)
//
const char B8_VAL_FILL(unsigned char srcVal, unsigned int srcPos, unsigned char dstVal, unsigned int dstPos, unsigned int len);

//
// �����ֽ�srcVal�н�ȡsrcPos��ʼ��lenλ������䵽���ֽ�dstVal�д�dstPos��ʼ��lenλ(����������)
//
const UINT32 B32_VAL_FILL(UINT32 srcVal, unsigned int srcPos, UINT32 dstVal, unsigned int dstPos, unsigned int len);

//
// �ֽڵߵ�(��: 0011 0001 0111 1111 => 1111 0111 0001 0011)
//
char* bytes_reverse(char *data, int len);
#endif