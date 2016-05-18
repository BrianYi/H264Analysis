#include "H264Analysis.h"

H264Analysis::H264Analysis(void)
{
	/// 初始化二进制指针
	m_lastByte = 0;
	m_binCurPos = 0;
}


H264Analysis::~H264Analysis(void)
{
	if (m_iFileStream.is_open())
		m_iFileStream.close();
}


//************************************
// 函数名:	H264Analysis::getOpenFile
// 描述:	获取并打开文件
// 返回值:	std::ifstream&
// 参数: 	const string & fileName
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
std::ifstream& H264Analysis::getOpenFile( const string &fileName )
{
	/// 初始化二进制指针
	m_lastByte = 0;
	m_binCurPos = 0;

	if (!m_iFileStream.is_open())
		m_iFileStream.open(fileName.c_str(), ios_base::binary);

	return m_iFileStream;
}


//************************************
// 函数名:	H264Analysis::closeFile
// 描述:	关闭文件
// 返回值:	void
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
void H264Analysis::closeFile()
{
	if (m_iFileStream.is_open())
	{
		m_iFileStream.close();
	}
	return ;
}


//************************************
// 函数名:	H264Analysis::readNaluData
// 描述:	读取Nalu数据部分（不包括startCode）并跳过startCode，完成后文件指针指向Nalu的数据部分开头
// 返回值:	size_t => Nalu数据部分长度
// 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::readNaluData( char **naluData )
{
	// 获取当前的Nalu位置(包含startCode)
	int curNaluPos = m_iFileStream.tellg();
	int len = 0;
	
	// 定位到跳过startCode位置
	int startCodeLen = skipNaluStartCode();

	// 获取Nalu数据部分的位置
	int curNaluDataPos = curNaluPos + startCodeLen;
	
	// 获取下一Nalu的位置(包含startCode)
	int nextNaluPos = 0;
	if (nextNalu(&nextNaluPos) == Failed)
		return Failed;

	// 回到原来Nalu数据部分的位置
	m_iFileStream.seekg(curNaluDataPos);

	// 读取Nalu的数据部分
	len = nextNaluPos - curNaluDataPos; // Nalu长度
	if (naluData && readNextBytes(naluData, len) == Failed)
		return Failed;
	m_iFileStream.seekg(curNaluDataPos);	// 回到原来的Nalu位置(跳过startCode)

	// 置位二进制指针标记
	if (naluData)
	{
		m_binCurPos = 0;
		m_lastByte = (*naluData)[0];
	}
	
	return len;
}

//************************************
// 函数名:	H264Analysis::nextNalu
// 描述:	找到下个Nalu的起始位置
// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
// 参数:	int * naluPos(out: 返回该Nalu的起始位置, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/16
// 作者: 	YJZ
// 修改记录:
//************************************
STATUS H264Analysis::nextNalu(int *naluPos)
{
	int i = 0;
	char c = 0;
	while(true) // 
	{
		if (readNextByte(&c) == Failed)
			return Failed;

		if (c == 0x00)
		{
			i++;
		}
		else if (c == 0x01 && ( i == 2 || i == 3 ))
		{
			i++;
			break;
		}
		else
		{
			i = 0;
		}
	}

	int curNaluDataPos = m_iFileStream.tellg();
	int curNaluPos = curNaluDataPos - i;
	m_iFileStream.seekg(curNaluPos);
	if (naluPos)
		*naluPos = curNaluPos;
	
	return Success;
}


//************************************
// 函数名:	H264Analysis::next_SPS_Nalu
// 描述:	获取下一个包含SPS的Nalu, 并将数据放入参数Nalu传出，完成后文件指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => NALU数据部分长度
// 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_SPS_Nalu( char **naluData )
{
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< 跳过startCode
		curNaluDataPos = m_iFileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SPS)
		{
			m_iFileStream.seekg(curNaluPos);
			return readNaluData(naluData);
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_PPS_Nalu
// 描述:	获取下一个包含PPS的Nalu, 并将数据放入参数Nalu传出，完成后文件指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => NALU数据部分长度
// 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_PPS_Nalu( char **naluData )
{
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< 跳过startCode
		curNaluDataPos = m_iFileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_PPS)
		{
			m_iFileStream.seekg(curNaluPos);
			return readNaluData(naluData);
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_I_Nalu
// 描述:	获取下一个包含I帧的Nalu, 并将数据放入参数Nalu传出，完成后文件指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => NALU数据部分长度
// 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_I_Nalu( char **naluData )
{
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	UINT32 first_mb_in_slice = 0;
	UINT32 slice_type = 0;
	static int i = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< 跳过startCode
		curNaluDataPos = m_iFileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_P_Nalu
// 描述:	获取下一个包含P帧的Nalu, 并将数据放入参数Nalu传出，完成后文件指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => NALU数据部分长度
// 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_P_Nalu( char **naluData )
{
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	UINT32 first_mb_in_slice = 0;
	UINT32 slice_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< 跳过startCode
		curNaluDataPos = m_iFileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_P1 || slice_type == SLICE_TYPE_P2)
			{
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_B_Nalu
// 描述:	获取下一个包含B帧的Nalu, 并将数据放入参数Nalu传出，完成后文件指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => NALU数据部分长度
// 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_B_Nalu( char **naluData )
{
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	UINT32 first_mb_in_slice = 0;
	UINT32 slice_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< 跳过startCode
		curNaluDataPos = m_iFileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_B1 || slice_type == SLICE_TYPE_B2)
			{
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_SI_Nalu
// 描述:	获取下一个包含SI帧的Nalu, 并将数据放入参数Nalu传出，完成后文件指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => NALU数据部分长度
// 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_SI_Nalu( char **naluData )
{
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	UINT32 first_mb_in_slice = 0;
	UINT32 slice_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< 跳过startCode
		curNaluDataPos = m_iFileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_SI1 || slice_type == SLICE_TYPE_SI2)
			{
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_SP_Nalu
// 描述:	获取下一个包含SP帧的Nalu, 并将数据放入参数Nalu传出，完成后文件指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => NALU数据部分长度
// 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_SP_Nalu( char **naluData )
{
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	UINT32 first_mb_in_slice = 0;
	UINT32 slice_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< 跳过startCode
		curNaluDataPos = m_iFileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_SP1 || slice_type == SLICE_TYPE_SP2)
			{
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::skipNaluStartCode
// 描述:	跳过Nalu的startCode, 并返回跳过的startCode长度，完成后文件指针指向Nalu的数据部分开头
// 返回值:	size_t => 跳过的startCode长度
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::skipNaluStartCode()
{
	int len = 0;
	char *p = new char[4];
	int curNaluPos = m_iFileStream.tellg();
	if (readNextBytes(&p, 4) == Failed)
		return Failed;
	if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01)
	{
		len = 3;
		m_iFileStream.seekg(curNaluPos+3);
	}
	else if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
	{
		len = 4;
	}
	delete [] p;

	if (len == 0)
	{
		m_iFileStream.seekg(curNaluPos);
	}

	return len;
}


//************************************
// 函数名:	H264Analysis::readNextBytes
// 描述:	读取接下来的len个字节
// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
// 参数: 	char * * p(out: 传出读取的字节数据，需要调用者释放内存)
// 参数: 	int len(in: 读取字节的长度)
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
STATUS H264Analysis::readNextBytes( char **p, int len)
{
	if (len <= 0 || m_iFileStream.eof())
	{
		return Failed;
	}

	*p = new char[len];
	m_iFileStream.read(*p, len);
	return Success;
}


//************************************
// 函数名:	H264Analysis::readNextByte
// 描述:	读取接下来的1个字节
// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
// 参数: 	char * c(out: 传出字节数据)
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
STATUS H264Analysis::readNextByte( char *c )
{
	if (m_iFileStream.eof())
	{
		return Failed;
	}

	m_iFileStream.read(c, 1);
	return Success;
}


//************************************
// 函数名:	H264Analysis::ueDecode
// 描述:	解码Exp-Golomb-Code(指数哥伦布编码)
// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
// 参数: 	UINT32 * codeNum(out: 传出解码后的值)
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
STATUS H264Analysis::ueDecode(UINT32 *codeNum)
{
	int leadingZeroBits = -1;
	for (char b = 0; !b; leadingZeroBits++)
	{
		if (m_binCurPos == 0)
		{
			if (readNextByte((char*)&m_lastByte) == Failed)
				return Failed;
		}
		b = B8_VAL_BASE_R(m_lastByte, m_binCurPos, 1);	// read next bit
		m_binCurPos++;	// 指针下移
		if (m_binCurPos % 8 == 0)
			m_binCurPos = 0;
	}

	if (leadingZeroBits > 32)
	{
		throw exception("");
		return -1;
	}

	// 指针还有剩余位置未读
	int readBits = 0;
	int leftBits = 8 - m_binCurPos;
	int leadingZeroBytes = 0;
	if (m_binCurPos != 0)
	{
		if (leadingZeroBits == 0)
		{
			*codeNum = 0;
		}
		else
		{
			if (leadingZeroBits <= leftBits)	// 剩余位置足够满足哥伦布解码
			{
				readBits = B8_VAL_BASE_R(m_lastByte, m_binCurPos, leadingZeroBits);
			}
			else	// 剩余位置不够满足哥伦布解码
			{
				int leftLeadingZeroBits = leadingZeroBits - leftBits;
				int leftLeadingZeroBytes = bits_get_byte_num(leftLeadingZeroBits);
				char *p = new char[leftLeadingZeroBytes];
				UINT32 golombCode = 0;
				if (readNextBytes(&p, leftLeadingZeroBytes) == Failed)
					return Failed;
				memcpy((void*)&golombCode, p, leftLeadingZeroBytes);
				bytes_reverse((char*)&golombCode, sizeof(golombCode));

				// 提取完整的Exp-Golomb-Code
				golombCode &= B32_VAL_MASK(0, leftLeadingZeroBits);
				golombCode = (golombCode>>leftBits)|(B32_VAL_BASE_L((UINT32)B8_VAL_BASE_L(m_lastByte, m_binCurPos), 24));
				golombCode = B32_VAL_BASE_R(golombCode, 0, leadingZeroBits);
				*codeNum = golombCode;
				
				// 存放最后字节
				m_lastByte = p[leftLeadingZeroBytes-1];
				delete [] p;
			}
			m_binCurPos += leadingZeroBits;
			m_binCurPos %= 8;
			*codeNum = pow(2.0, leadingZeroBits) - 1 + readBits;
		}
	}
	else	// 指针没有剩余位置未读
	{
		if (leadingZeroBits == 0)
		{
			*codeNum = 0;
		}
		else
		{
			leadingZeroBytes = bits_get_byte_num(leadingZeroBits);
			char *p = new char[leadingZeroBytes];
			UINT32 golombCode = 0;
			if (readNextBytes(&p, leadingZeroBytes) == Failed)
				return Failed;
			memcpy((void*)&golombCode, p, leadingZeroBytes);
			bytes_reverse((char*)&golombCode, sizeof(golombCode));

			// 提取完整的Exp-Golomb-Code
			golombCode &= B32_VAL_MASK(0, leadingZeroBits);
			golombCode = B32_VAL_BASE_R(golombCode, 0, leadingZeroBits);
			*codeNum = golombCode;

			// 存放最后字节
			m_lastByte = p[leadingZeroBytes-1];
			delete [] p;

			m_binCurPos += leadingZeroBits;
			m_binCurPos %= 8;
			*codeNum = pow(2.0, leadingZeroBits) - 1 + readBits;
		}
	}
	return Success;
}

