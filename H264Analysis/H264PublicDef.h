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
// 字节位颠倒(如: 0011 0001 => 1000 1100)
// 
unsigned char bits_reverse(unsigned char data);

//
// 多字节位颠倒(如: 0011 0001 0111 1111 => 1111 1110 1000 1100)
// 
char* bits_reverse(char *data, int len);


//
// 按字节对齐返回字节数
//
int bits_get_byte_num(int len);


//
// 将c以二进制形式输出, 格式为从右到左(最低位到最高位)
//
// void bits_binary_printf(char c);

//
// 将data以二进制形式输出, 格式为从右到左(最低位到最高位)
//
// void bits_binary_printf(char *data, int len);

//
// 将data以十六进制形式输出, 小端存储的方式
//
// void bytes_hex_printf(char *data, int len);

//
// 获取截取字节第pos位起, 长度为len位的字节掩码
//
const char B8_VAL_MASK(unsigned int pos, unsigned int len);

//
// 获取截取四字节第pos位起, 长度为n位的字节掩码
//
const UINT32 B32_VAL_MASK(unsigned int pos, unsigned int len);


//
// 获取截取字节第pos位开始到结束的字节掩码
//
const char B8_VAL_MASK(unsigned int pos);

//
// 获取截取四字节第pos位开始到结束的字节掩码
//
const UINT32 B32_VAL_MASK(unsigned int pos);

//
// 获取截取字节val第pos位起到结束的值，并右移至右边第一位非0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos);

//
// 获取截取四字节val第pos位起到结束的值，并右移至右边第一位非0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos);


//
// 获取截取字节val第pos位起, 长度为n位的字节并右移至右边第一位非0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos, unsigned int len);

//
// 获取截取四字节val第pos位起, 长度为n位的字节并右移至右边第一位非0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos, unsigned int len);

//
// 获取截取字节val第pos位起到结束的值，并左移至左边第一位非0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos);

//
// 获取截取四字节val第pos位起到结束的值，并左移至左边第一位非0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos);

//
// 获取截取字节val第pos位起, 长度为n位的字节并左移至左边第一位非0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos, unsigned int len);

//
// 获取截取四字节val第pos位起, 长度为n位的字节并左移至左边第一位非0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos, unsigned int len);

//
// 字节val从pos位开始的len位清零(从左往右数)
//
const char B8_VAL_ZERO(unsigned char val, unsigned int pos, unsigned int len);

//
// 四字节val从pos位开始的len位清零(从左往右数)
//
const UINT32 B32_VAL_ZERO(UINT32 val, unsigned int pos, unsigned int len);

//
// 字节val从pos位开始的len位置一(从左往右数)
//
const char B8_VAL_ONE(unsigned char val, unsigned int pos, unsigned int len);

//
// 四字节val从pos位开始的len位置一(从左往右数)
//
const UINT32 B32_VAL_ONE(UINT32 val, unsigned int pos, unsigned int len);

//
// 从字节srcVal中截取srcPos开始的len位数据填充到字节dstVal中从dstPos开始的len位(从左往右数)
//
const char B8_VAL_FILL(unsigned char srcVal, unsigned int srcPos, unsigned char dstVal, unsigned int dstPos, unsigned int len);

//
// 从四字节srcVal中截取srcPos开始的len位数据填充到四字节dstVal中从dstPos开始的len位(从左往右数)
//
const UINT32 B32_VAL_FILL(UINT32 srcVal, unsigned int srcPos, UINT32 dstVal, unsigned int dstPos, unsigned int len);

//
// 字节颠倒(如: 0011 0001 0111 1111 => 1111 0111 0001 0011)
//
char* bytes_reverse(char *data, int len);
#endif