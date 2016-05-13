#include "H264Analysis.h"

H264Analysis::H264Analysis(void)
{
	/// ��ʼ��������ָ��
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
 * ������: readNextBits
 * ����: ��ȡ����N������λ���ֽڶ��룬����һ���ֽڰ�һ���ֽڷ��䣩
 * ����: int n , BOOL* isFinished = NULL
 * ����ֵ: N����λ��ɵ�ֵ
 *		(1) ���������ֽڶ���, ��m_binCurPos = ������ mod 8
 *		(2) �������ֽڶ���, ��m_binCurPos = 0
 *		�������һ���ֽڵ����ݷ���m_lastByte��
 * ����: 2016/05/13
 * ����: YJZ
 * �޸ļ�¼: 
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
		int byteNum = 0; ///< �ֽڶ���ķ�ʽ��ȡ
		m_binCurPos = n % 8;
		byteNum = bits_get_byte_num(n);	///< ����һ�ֽڰ�һ�ֽڷ���
		char *p = new char[byteNum];
		m_iFileStream.read(p, byteNum);
		m_lastByte = p[byteNum-1];	///< ʼ��Ϊ���һ���ֽڵ�����
		p[byteNum-1] = m_binCurPos ? m_lastByte&(0xff<<(8-m_binCurPos)) : m_lastByte;	///< ���һ���ֽڵĴ���
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

