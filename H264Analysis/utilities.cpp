/**
 * λ������
 */
#include <iostream>
#include <iomanip>
#include <bitset>
#include <Windows.h>

//
// �ֽ�λ�ߵ�(��: 0011 0001 => 1000 1100)
// 
unsigned char bits_reverse(unsigned char data) 
{ 
	data=((data&0xf0)>>4) | ((data&0x0f)<<4); 
	data=((data&0xCC)>>2) | ((data&0x33)<<2); 
	data=((data&0xAA)>>1) | ((data&0x55)<<1); 
	return data; 
}

//
// ���ֽ�λ�ߵ�(��: 0011 0001 0111 1111 => 1111 1110 1000 1100)
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
// ���ֽڶ��뷵���ֽ���
//
int bits_get_byte_num(int len)
{
	return (len % 8 ? len/8+1 : len/8);
}


//
// ��c�Զ�������ʽ���, ��ʽΪ���ҵ���(���λ�����λ)
//
void bits_binary_printf(char c)
{
	std::bitset<8> tmp((int)c);
	std::cout << tmp;
}

//
// ��data�Զ�������ʽ���, ��ʽΪ���ҵ���(���λ�����λ)
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
// ��data��ʮ��������ʽ���, С�˴洢�ķ�ʽ
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
// ��ȡ��ȡ�ֽڵ�posλ��, ����Ϊlenλ���ֽ�����
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
// ��ȡ��ȡ���ֽڵ�posλ��, ����Ϊnλ���ֽ�����
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
// ��ȡ��ȡ�ֽڵ�posλ��ʼ���������ֽ�����
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
// ��ȡ��ȡ���ֽڵ�posλ��ʼ���������ֽ�����
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
// ��ȡ��ȡ�ֽ�val��posλ�𵽽�����ֵ�����������ұߵ�һλ��0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos)
{
	return val & B8_VAL_MASK(pos);
}

//
// ��ȡ��ȡ���ֽ�val��posλ�𵽽�����ֵ�����������ұߵ�һλ��0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos)
{
	return val & B32_VAL_MASK(pos);
}


//
// ��ȡ��ȡ�ֽ�val��posλ��, ����Ϊnλ���ֽڲ��������ұߵ�һλ��0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos, unsigned int len)
{
	return (val & B8_VAL_MASK(pos, len)) >> (8 - (pos + len));
}

//
// ��ȡ��ȡ���ֽ�val��posλ��, ����Ϊnλ���ֽڲ��������ұߵ�һλ��0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos, unsigned int len)
{
	return (val & B32_VAL_MASK(pos, len)) >> (32 - (pos + len));
}

//
// ��ȡ��ȡ�ֽ�val��posλ�𵽽�����ֵ������������ߵ�һλ��0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos)
{
	return ((val & B8_VAL_MASK(pos)) << pos);
}

//
// ��ȡ��ȡ���ֽ�val��posλ�𵽽�����ֵ������������ߵ�һλ��0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos)
{
	return ((val & B32_VAL_MASK(pos)) << pos);
}

//
// ��ȡ��ȡ�ֽ�val��posλ��, ����Ϊnλ���ֽڲ���������ߵ�һλ��0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos, unsigned int len)
{
	return (val & B8_VAL_MASK(pos, len)) << pos;
}

//
// ��ȡ��ȡ���ֽ�val��posλ��, ����Ϊnλ���ֽڲ���������ߵ�һλ��0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos, unsigned int len)
{
	return (val & B32_VAL_MASK(pos, len)) << pos;
}

//
// �ֽ�val��posλ��ʼ��lenλ����(����������)
//
const char B8_VAL_ZERO(unsigned char val, unsigned int pos, unsigned int len)
{
	return (val & (~B8_VAL_MASK(pos, len)));
}

//
// ���ֽ�val��posλ��ʼ��lenλ����(����������)
//
const UINT32 B32_VAL_ZERO(UINT32 val, unsigned int pos, unsigned int len)
{
	return (val & (~B32_VAL_MASK(pos, len)));
}

//
// �ֽ�val��posλ��ʼ��lenλ��һ(����������)
//
const char B8_VAL_ONE(unsigned char val, unsigned int pos, unsigned int len)
{
	return (val | B8_VAL_MASK(pos, len));
}

//
// ���ֽ�val��posλ��ʼ��lenλ��һ(����������)
//
const UINT32 B32_VAL_ONE(UINT32 val, unsigned int pos, unsigned int len)
{
	return (val | B32_VAL_MASK(pos, len));
}

//
// ���ֽ�srcVal�н�ȡsrcPos��ʼ��lenλ������䵽�ֽ�dstVal�д�dstPos��ʼ��lenλ(����������)
//
const char B8_VAL_FILL(unsigned char srcVal, unsigned int srcPos, unsigned char dstVal, unsigned int dstPos, unsigned int len)
{
	int diff = abs((int)(srcPos - dstPos));
	srcVal &= B8_VAL_MASK(srcPos, len);
	if (srcPos - dstPos < 0)
	{
		// ����diff
		srcVal >>= diff;
	}
	else
	{
		// ����diff
		srcVal <<= diff;
	}
	dstVal = B8_VAL_ZERO(dstVal, dstPos, len);
	return dstVal | srcVal;
}

//
// �����ֽ�srcVal�н�ȡsrcPos��ʼ��lenλ������䵽���ֽ�dstVal�д�dstPos��ʼ��lenλ(����������)
//
const UINT32 B32_VAL_FILL(UINT32 srcVal, unsigned int srcPos, UINT32 dstVal, unsigned int dstPos, unsigned int len)
{
	int diff = abs((int)(srcPos - dstPos));
	srcVal &= B32_VAL_MASK(srcPos, len);
	if (srcPos - dstPos < 0)
	{
		// ����diff
		srcVal >>= diff;
	}
	else
	{
		// ����diff
		srcVal <<= diff;
	}
	dstVal = B32_VAL_ZERO(dstVal, dstPos, len);
	return dstVal | srcVal;
}

//
// �ֽڵߵ�(��: 0011 0001 0111 1111 => 1111 0111 0001 0011)
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