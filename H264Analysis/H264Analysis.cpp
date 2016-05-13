#include "H264Analysis.h"

H264Analysis::H264Analysis(void)
{
	/// 初始化二进制指针
	m_lastByte = 0;
	m_binCurPos = 0;
}


H264Analysis::~H264Analysis(void)
{
}


const std::ifstream& H264Analysis::getOpenFile( const string &fileName )
{
	m_iFileStream.open(fileName.c_str(), ios_base::binary);
	return m_iFileStream;
}

/*!
 * 函数名: readNextBits
 * 描述: 读取下面N个比特位（字节对齐，不足一个字节按一个字节分配）
 * 参数: int n , BOOL* isFinished = NULL
 * 返回值: N比特位组成的值
 *		(1) 若不符合字节对齐, 则将m_binCurPos = 比特数 mod 8
 *		(2) 若符合字节对齐, 则将m_binCurPos = 0
 *		都将最后一个字节的内容放入m_lastByte中
 * 日期: 2016/05/13
 * 作者: YJZ
 * 修改记录: 
 */
UINT32 H264Analysis::readNextBits( int n , BOOL* isFinished)
{
	if (n <=0 || n >= 33)
	{
		//
		if (isFinished)
			*isFinished = FALSE;
		return -1;
	}

	UINT32 bitsVal = 0;
	if (m_binCurPos == 0)
	{
		int byteNum = 0; ///< 字节对齐的方式读取
		m_binCurPos = n % 8;
		byteNum = bits_get_byte_num(n);	///< 不足一字节按一字节分配
		char *p = new char[byteNum];
		m_iFileStream.read(p, byteNum);
		m_lastByte = p[byteNum-1];	///< 始终为最后一个字节的内容
		p[byteNum-1] = m_binCurPos ? m_lastByte&(0xff<<(8-m_binCurPos)) : m_lastByte;	///< 最后一个字节的处理
		memcpy((void*)&bitsVal, p, byteNum);
		delete [] p;
	}
	else
	{
		
	}
	return bitsVal;
}

void H264Analysis::closeFile()
{
	if (m_iFileStream && m_iFileStream.is_open())
	{
		m_iFileStream.close();
	}
	return ;
}

int H264Analysis::ueDecode()
{
	int leadingZeroBits = -1;
	for (unsigned char b = 0; !b; leadingZeroBits++)
	{
		b = *(unsigned char*)readNextBits(1);
		m_lastByte = b;
		m_binCurPos = 1;
	}
	UINT32 p = readNextBits(leadingZeroBits);
	int codeNum = pow(2.0, leadingZeroBits) - 1 /* + s */;
	return 1;
}

