/**
 * 位串处理
 */
#include <iostream>
#include <iomanip>
#include <bitset>
#include <Windows.h>

//
// 字节位颠倒(如: 0011 0001 => 1000 1100)
// 
unsigned char bits_reverse(unsigned char data) 
{ 
	data=((data&0xf0)>>4) | ((data&0x0f)<<4); 
	data=((data&0xCC)>>2) | ((data&0x33)<<2); 
	data=((data&0xAA)>>1) | ((data&0x55)<<1); 
	return data; 
}

//
// 多字节位颠倒(如: 0011 0001 0111 1111 => 1111 1110 1000 1100)
// 
char* bits_reverse(char *data, int len)
{
	char tmp;
	int i;
	for (i = 0; i < len/2; i++)
	{
		tmp = data[i];
		data[i] = data[len-i-1];
		data[len-i-1] = tmp;
	}
	for (i = 0; i < len; i++)
	{
		data[i] = bits_reverse(data[i]);
	}
	return data;
}


//
// 按字节对齐返回字节数
//
int bits_get_byte_num(int len)
{
	return (len % 8 ? len/8+1 : len/8);
}


//
// 将c以二进制形式输出, 格式为从右到左(最低位到最高位)
//
void bits_binary_printf(char c)
{
	std::bitset<8> tmp((int)c);
	std::cout << tmp;
}

//
// 将data以二进制形式输出, 格式为从右到左(最低位到最高位)
//
void bits_binary_printf(char *data, int len)
{
	for (int i = len-1; i >= 0; i--)
	{
		bits_binary_printf(data[i]);
		std::cout << " ";
	}
}

//
// 将data以十六进制形式输出, 小端存储的方式
//
void bytes_hex_printf(char *data, int len)
{
	for (int i = 0; i < len; i++)
	{
		if (!(i % 16) && i)
			std::cout << std::endl;
		std::cout << std::hex << std::setw(2) << std::setfill('0') << ((int)data[i] & 0xff) << " ";
	}
	std::cout << std::endl;
}

//
// 获取截取字节第pos位起, 长度为len位的字节掩码
//
const char B8_VAL_MASK(unsigned int pos, unsigned int len)	
{ 
	if (pos+len > 8)
	{
		throw std::exception();
		return 0;
	}
	return ((0xff >> (8-len))<<(8-pos-len)); 
}

//
// 获取截取四字节第pos位起, 长度为n位的字节掩码
//
const UINT32 B32_VAL_MASK(unsigned int pos, unsigned int len)
{
	if (pos+len > 32)
	{
		throw std::exception();
		return 0;
	}
	return ((0xffffffff >> (32-len))<<(32-pos-len));
}


//
// 获取截取字节第pos位开始到结束的字节掩码
//
const char B8_VAL_MASK(unsigned int pos)			
{ 
	if (pos >= 8)
	{
		throw std::exception();
		return 0;
	}
	return (0xff >> pos); 
}

//
// 获取截取四字节第pos位开始到结束的字节掩码
//
const UINT32 B32_VAL_MASK(unsigned int pos)			
{ 
	if (pos >= 32)
	{
		throw std::exception();
		return 0;
	}
	return (0xffffffff >> pos); 
}

//
// 获取截取字节val第pos位起到结束的值，并右移至右边第一位非0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos)
{
	return val & B8_VAL_MASK(pos);
}

//
// 获取截取四字节val第pos位起到结束的值，并右移至右边第一位非0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos)
{
	return val & B32_VAL_MASK(pos);
}


//
// 获取截取字节val第pos位起, 长度为n位的字节并右移至右边第一位非0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos, unsigned int len)
{
	return (val & B8_VAL_MASK(pos, len)) >> (8 - (pos + len));
}

//
// 获取截取四字节val第pos位起, 长度为n位的字节并右移至右边第一位非0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos, unsigned int len)
{
	return (val & B32_VAL_MASK(pos, len)) >> (32 - (pos + len));
}

//
// 获取截取字节val第pos位起到结束的值，并左移至左边第一位非0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos)
{
	return ((val & B8_VAL_MASK(pos)) << pos);
}

//
// 获取截取四字节val第pos位起到结束的值，并左移至左边第一位非0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos)
{
	return ((val & B32_VAL_MASK(pos)) << pos);
}

//
// 获取截取字节val第pos位起, 长度为n位的字节并左移至左边第一位非0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos, unsigned int len)
{
	return (val & B8_VAL_MASK(pos, len)) << pos;
}

//
// 获取截取四字节val第pos位起, 长度为n位的字节并左移至左边第一位非0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos, unsigned int len)
{
	return (val & B32_VAL_MASK(pos, len)) << pos;
}

//
// 字节val从pos位开始的len位清零(从左往右数)
//
const char B8_VAL_ZERO(unsigned char val, unsigned int pos, unsigned int len)
{
	return (val & (~B8_VAL_MASK(pos, len)));
}

//
// 四字节val从pos位开始的len位清零(从左往右数)
//
const UINT32 B32_VAL_ZERO(UINT32 val, unsigned int pos, unsigned int len)
{
	return (val & (~B32_VAL_MASK(pos, len)));
}

//
// 字节val从pos位开始的len位置一(从左往右数)
//
const char B8_VAL_ONE(unsigned char val, unsigned int pos, unsigned int len)
{
	return (val | B8_VAL_MASK(pos, len));
}

//
// 四字节val从pos位开始的len位置一(从左往右数)
//
const UINT32 B32_VAL_ONE(UINT32 val, unsigned int pos, unsigned int len)
{
	return (val | B32_VAL_MASK(pos, len));
}

//
// 从字节srcVal中截取srcPos开始的len位数据填充到字节dstVal中从dstPos开始的len位(从左往右数)
//
const char B8_VAL_FILL(unsigned char srcVal, unsigned int srcPos, unsigned char dstVal, unsigned int dstPos, unsigned int len)
{
	int diff = abs((int)(srcPos - dstPos));
	srcVal &= B8_VAL_MASK(srcPos, len);
	if (srcPos - dstPos < 0)
	{
		// 右移diff
		srcVal >>= diff;
	}
	else
	{
		// 左移diff
		srcVal <<= diff;
	}
	dstVal = B8_VAL_ZERO(dstVal, dstPos, len);
	return dstVal | srcVal;
}

//
// 从四字节srcVal中截取srcPos开始的len位数据填充到四字节dstVal中从dstPos开始的len位(从左往右数)
//
const UINT32 B32_VAL_FILL(UINT32 srcVal, unsigned int srcPos, UINT32 dstVal, unsigned int dstPos, unsigned int len)
{
	int diff = abs((int)(srcPos - dstPos));
	srcVal &= B32_VAL_MASK(srcPos, len);
	if (srcPos - dstPos < 0)
	{
		// 右移diff
		srcVal >>= diff;
	}
	else
	{
		// 左移diff
		srcVal <<= diff;
	}
	dstVal = B32_VAL_ZERO(dstVal, dstPos, len);
	return dstVal | srcVal;
}

//
// 字节颠倒(如: 0011 0001 0111 1111 => 1111 0111 0001 0011)
//
char* bytes_reverse(char *data, int len)
{
	char tmp;
	for (int i = 0; i < len/2; i++)
	{
		tmp = data[i];
		data[i] = data[len-i-1];
		data[len-i-1] = tmp;
	}
	return data;
}