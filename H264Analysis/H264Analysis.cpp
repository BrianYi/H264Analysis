#include "H264Analysis.h"

H264Analysis::H264Analysis(void)
{
	/// 初始化流结构
	//m_stream = new DataStream();
	//m_stream->buf = NULL;
	m_len = 0;
	//m_stream->ptr = NULL;
	m_lastByte = 0;
	m_binPos = 0;

	m_debugFileStream.open("log.txt", ios::trunc | ios::out);
}


H264Analysis::~H264Analysis(void)
{
	if (m_fileStream.is_open())
		closeFile();

	if (m_debugFileStream.is_open())
		m_debugFileStream.close();
}


//************************************
// 函数名:	H264Analysis::getOpenFile
// 描述:	获取并打开文件
// 返回值:	DataStream *
// 参数: 	const string & fileName
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
ifstream& H264Analysis::getOpenFile( const string &fileName )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/// 初始化流结构
	if (m_fileStream.is_open())
		m_fileStream.close();
	m_len = 0;
	m_lastByte = 0;
	m_binPos = 0;

	/// 文件流数据存放到流结构中
	m_fileStream.open(fileName.c_str(), ios_base::binary);
	m_fileStream.seekg(0, ios_base::end);
	m_len = m_fileStream.tellg();
	m_fileStream.seekg(ios_base::beg);
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "getOpenFile " << time_diff << endl;
#endif
	return m_fileStream;
}



//************************************
// 函数名:	H264Analysis::clearStream
// 描述:	清空流数据
// 返回值:	void
// 日期: 	2016/05/19
// 作者: 	YJZ
// 修改记录:
//************************************
void H264Analysis::closeFile()
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	if (m_fileStream.is_open())
	{
		m_fileStream.close();

		m_len = 0;
		m_binPos = 0;
		m_lastByte = 0;
	}
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "closeFile " << time_diff << endl;
#endif
}

//************************************
// 函数名:	H264Analysis::readNaluData
// 描述:	读取Nalu，完成后数据流指针指向Nalu的开头
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::readNaluData( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	// 获取当前的Nalu位置(包含startCode)
	int curNaluPos = m_fileStream.tellg();
	int len = 0;

	// 定位到跳过startCode位置
	int startCodeLen = skipNaluStartCode();

	// 获取Nalu数据部分的位置
	int curNaluDataPos = curNaluPos + startCodeLen;

	// 获取下一Nalu的位置(包含startCode)
	int nextNaluPos = 0;
	if (nextNalu(&nextNaluPos) == FileEnd)
	{
		// 已到文件结尾
		nextNaluPos = m_len;
	}

	// 读取Nalu的数据长度
	len = nextNaluPos - curNaluPos; // Nalu长度

	// 回到原来Nalu的位置
	m_fileStream.seekg(-len, ios::cur);
	
	// 置位二进制指针标记
	if (naluData)
	{
		// 读取Nalu数据
		if (readNextBytes(naluData, len) == Failed)
			throw exception();
		m_binPos = 0;
		m_lastByte = (*naluData)[0];
		m_fileStream.seekg(-len, ios::cur);	// 回到原来的Nalu位置
	}
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "readNaluData " << time_diff << endl;
#endif
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
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
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

	// 重新回到Nalu的头部
	m_fileStream.seekg(-i, ios::cur);
	//int curNaluDataPos = m_fileStream.tellg();
	//int curNaluPos = curNaluDataPos - i;
	//m_stream->ptr = m_stream->buf + curNaluPos;
	if (naluPos)
		*naluPos = m_fileStream.tellg();

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "nextNalu " << time_diff << endl;
#endif

	return Success;
}


//************************************
// 函数名:	H264Analysis::next_SPS_Nalu
// 描述:	获取下一个包含SPS的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_SPS_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< 跳过startCode
		//curNaluDataPos = m_fileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		//curNaluPos = curNaluDataPos - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SPS)
		{
			//m_stream->ptr = m_stream->buf + curNaluPos;
			m_fileStream.seekg(-startCodeLen - 1, ios::cur);
			size_t len = readNaluData(naluData);
		}
	}

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "next_SPS_Nalu " << time_diff << endl;
#endif

	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_PPS_Nalu
// 描述:	获取下一个包含PPS的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_PPS_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< 跳过startCode
		//curNaluDataPos = m_fileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		//curNaluPos = curNaluDataPos - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_PPS)
		{
			//m_stream->ptr = m_stream->buf + curNaluPos;
			m_fileStream.seekg(-startCodeLen - 1, ios::cur);

			size_t len = readNaluData(naluData);
#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
			return len;
		}
	}

	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_I_Nalu
// 描述:	获取下一个包含I帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_I_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
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
		//curNaluDataPos = m_fileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				//m_stream->ptr = m_stream->buf + curNaluPos;
				m_fileStream.seekg(curNaluPos);
				size_t len =  readNaluData(naluData);

#ifdef TIME_TEST
				DWORD time_diff = GetTickCount() - time_beg;
				m_debugFileStream << "next_I_Nalu " << time_diff << endl;
#endif

				return len;
			}
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_P_Nalu
// 描述:	获取下一个包含P帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_P_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
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
		//curNaluDataPos = m_fileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_P1 || slice_type == SLICE_TYPE_P2)
			{
				//m_stream->ptr = m_stream->buf + curNaluPos;
				m_fileStream.seekg(curNaluPos);
				size_t len = readNaluData(naluData);

#ifdef TIME_TEST
				DWORD time_diff = GetTickCount() - time_beg;
				m_debugFileStream << "next_P_Nalu " << time_diff << endl;
#endif

				return len;
			}
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_B_Nalu
// 描述:	获取下一个包含B帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_B_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
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
		//curNaluDataPos = m_fileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_B1 || slice_type == SLICE_TYPE_B2)
			{
				//m_stream->ptr = m_stream->buf + curNaluPos;
				m_fileStream.seekg(curNaluPos);
				size_t len = readNaluData(naluData);

#ifdef TIME_TEST
				DWORD time_diff = GetTickCount() - time_beg;
				m_debugFileStream << "next_B_Nalu " << time_diff << endl;
#endif

				return len;
			}
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_SI_Nalu
// 描述:	获取下一个包含SI帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_SI_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
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
		//curNaluDataPos = m_fileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_SI1 || slice_type == SLICE_TYPE_SI2)
			{
				//m_stream->ptr = m_stream->buf + curNaluPos;
				m_fileStream.seekg(curNaluPos);
				size_t len = readNaluData(naluData);

#ifdef TIME_TEST
				DWORD time_diff = GetTickCount() - time_beg;
				m_debugFileStream << "next_SI_Nalu " << time_diff << endl;
#endif

				return len;
			}
		}
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_SP_Nalu
// 描述:	获取下一个包含SP帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的开头(Nalu为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_SP_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
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
		//curNaluDataPos = m_fileStream.tellg();	///< 获取当前Nalu数据部分的起始位置(不包含startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< 获取当前Nalu的起始位置(包含startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_SP1 || slice_type == SLICE_TYPE_SP2)
			{
				//m_stream->ptr = m_stream->buf + curNaluPos;
				m_fileStream.seekg(curNaluPos);
				size_t len = readNaluData(naluData);

#ifdef TIME_TEST
				DWORD time_diff = GetTickCount() - time_beg;
				m_debugFileStream << "next_SP_Nalu " << time_diff << endl;
#endif

				return len;
			}
		}
	}
	return Failed;
}


//************************************
// 函数名:	H264Analysis::skipTo
// 描述:	跳转到指定位置(0~100)
// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
// 参数: 	short int persent(in: 0~100, 传入要跳转的百分比)
// 日期: 	2016/05/23
// 作者: 	YJZ
// 修改记录:
//************************************
STATUS H264Analysis::skipTo( short int persent )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif

	if (persent < 0 || persent > 100)
		return Failed;

	size_t pos = m_len * persent * 0.01;
	m_fileStream.seekg(pos);

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "skipTo " << time_diff << endl;
#endif

	return Success;
}


//************************************
// 函数名:	H264Analysis::skipNaluStartCode
// 描述:	跳过Nalu的startCode, 并返回跳过的startCode长度，完成后数据流指针指向Nalu的开头
// 返回值:	size_t => 跳过的startCode长度
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::skipNaluStartCode()
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	int len = 0;
	char *p = new char[4];
	int curNaluPos = m_fileStream.tellg();
	if (readNextBytes(&p, 4) == Failed)
		return Failed;
	if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01)
	{
		len = 3;
		//m_stream->ptr = m_stream->buf + curNaluPos + 3;
		m_fileStream.seekg(-1, ios::cur);
	}
	else if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
	{
		len = 4;
	}
	delete [] p;

	if (len == 0)
	{
		m_fileStream.seekg(-4, ios::cur);
	}

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "skipNaluStartCode " << time_diff << endl;
#endif

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
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	if (len <= 0 || (int)m_fileStream.tellg() + len > m_len)
	{
		return Failed;
	}

	*p = new char[len];
	m_fileStream.read(*p, len);
// 	char *c = *p;
// 	while(len--)
// 		*c++ = *m_stream->ptr++;
// 		
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "readNextBytes " << time_diff << endl;
#endif

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
// #ifdef TIME_TEST
// 	DWORD time_beg = GetTickCount();
// #endif
	if (m_fileStream.tellg() >= m_len)
	{
		return Failed;
	}

	m_fileStream.read(c, 1);
	//*c = *m_stream->ptr++;

// #ifdef TIME_TEST
// 	DWORD time_diff = GetTickCount() - time_beg;
// 	m_debugFileStream << "readNextByte " << time_diff << endl;
// #endif

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
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	int leadingZeroBits = -1;
	for (char b = 0; !b; leadingZeroBits++)
	{
		if (m_binPos == 0)
		{
			if (readNextByte((char*)&m_lastByte) == Failed)
				return Failed;
		}
		b = B8_VAL_BASE_R(m_lastByte, m_binPos, 1);	// read next bit
		m_binPos++;	// 指针下移
		if (m_binPos % 8 == 0)
			m_binPos = 0;
	}

	if (leadingZeroBits > 32)
	{
		throw exception();
		return -1;
	}

	// 指针还有剩余位置未读
	int readBits = 0;
	int leftBits = 8 - m_binPos;
	int leadingZeroBytes = 0;
	if (m_binPos != 0)
	{
		if (leadingZeroBits == 0)
		{
			*codeNum = 0;
		}
		else
		{
			if (leadingZeroBits <= leftBits)	// 剩余位置足够满足哥伦布解码
			{
				readBits = B8_VAL_BASE_R(m_lastByte, m_binPos, leadingZeroBits);
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
				golombCode = (golombCode>>leftBits)|(B32_VAL_BASE_L((UINT32)B8_VAL_BASE_L(m_lastByte, m_binPos), 24));
				golombCode = B32_VAL_BASE_R(golombCode, 0, leadingZeroBits);
				*codeNum = golombCode;

				// 存放最后字节
				m_lastByte = p[leftLeadingZeroBytes-1];
				delete [] p;
			}
			m_binPos += leadingZeroBits;
			m_binPos %= 8;
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

			m_binPos += leadingZeroBits;
			m_binPos %= 8;
			*codeNum = pow(2.0, leadingZeroBits) - 1 + readBits;
		}
	}

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "ueDecode " << time_diff << endl;
#endif

	return Success;
}

