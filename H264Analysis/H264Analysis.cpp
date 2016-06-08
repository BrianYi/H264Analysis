#include "H264Analysis.h"

H264Analysis::H264Analysis(void)
{
	/// 初始化流结构
	m_len = 0;
	m_lastByte = 0;
	m_binPos = 0;

	m_pStreamBuf = new DataStream;
	m_pStreamBuf->buf = NULL;
	m_pStreamBuf->pos = 0;
	m_pStreamBuf->len = 0;
	m_pStreamBuf->tellgBase = 0;

#ifdef TIME_TEST
	m_debugFileStream.open("log.txt", ios::trunc | ios::out);
#endif
}


H264Analysis::~H264Analysis(void)
{
	if (m_fileStream.is_open())
		closeFile();

	if (m_pStreamBuf)
	{
		if (m_pStreamBuf->buf)
		{
			delete [] m_pStreamBuf->buf;
			m_pStreamBuf->buf = NULL;
		}
		delete m_pStreamBuf;
	}

#ifdef TIME_TEST
	if (m_debugFileStream.is_open())
		m_debugFileStream.close();
#endif
}


//************************************
// 函数名:	H264Analysis::getOpenFile
// 描述:	获取文件流
// 返回值:	ifstream& 
// 参数: 	const string & fileName
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
std::ifstream& H264Analysis::getOpenFile( const std::string &fileName )
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
	m_fileStream.open(fileName.c_str(), std::ios_base::binary);
	m_fileStream.seekg(0, std::ios_base::end);
	m_len = m_fileStream.tellg();
	m_fileStream.seekg(std::ios_base::beg);
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "getOpenFile " << time_diff << endl;
#endif
	return m_fileStream;
}



//************************************
// 函数名:	H264Analysis::closeFile
// 描述:	关闭文件
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
// 函数名:	H264Analysis::nextNalu
// 描述:	获取下一个Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
// 参数:	char *naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/16
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::nextNalu(char **naluData)
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 找到当前startCode，获取文件位置
	 * 2. 跳过头部
	 * 3. 找到下个startCode，获取文件位置
	 * 4. 获取Nalu长度naluLen(下个startCode位置 - 当前startCode位置)
	 * 5. 文件指针重定位到当前startCode
	 * 6. 读取naluLen个字节保存到参数naluData中传出
	 * 7. 文件指针还原到原来位置
	 * 8. 返回naluLen
	 */

	char c;
	int startCodeLen = 0;

	// 1.找到当前startCode，获取文件位置
	// 2.跳过头部
	while (true)
	{
		if (checkStreamBuf() == Failed)
			return Failed;

		c = m_pStreamBuf->buf[m_pStreamBuf->pos];
		m_pStreamBuf->pos++;

		if (c == 0x00)
		{
			startCodeLen++;
		}
		else if (c == 0x01 && (startCodeLen == 2 || startCodeLen == 3))
		{
			startCodeLen++;
			break;
		}
		else
			startCodeLen = 0;
	}
	int naluPos = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - startCodeLen;

	// 3. 找到下个startCode，获取文件位置
	startCodeLen = 0;
	while (true)
	{
		if (checkStreamBuf() == Failed) ///< 已到文件结尾
			break;

		c = m_pStreamBuf->buf[m_pStreamBuf->pos];
		m_pStreamBuf->pos++;

		if (c == 0x00)
		{
			startCodeLen++;
			/**							
			 * 问题：							  -----------
			 *			出现 m_pStreamBuf->buf =>|xx .. xx 00|  
			 *									  -----------
			 *									  -----------
			 *			  新 m_pStreamBuf->buf =>|00 01 xx ..|
			 *									  -----------
			 *										 ^
			 *										 | (这个位置才找到startCode)
			 *	
			 * 处理方式：文件指针指向最后连续00之前，然后从新分配m_pStreamBuf->buf
			 *	
			 *								   --------------
			 *	 处理后的 m_pStreamBuf->buf =>|00 00 01 .. xx|
			 *								   --------------
			 *								   ^
			 *								   | (m_pStreamBuf->pos = 0)
			 */
			if (m_pStreamBuf->pos >= m_pStreamBuf->len)	// 缓冲区尾为startCode情况
			{
				m_fileStream.seekg(m_pStreamBuf->tellgBase + m_pStreamBuf->pos - startCodeLen); // 文件指针回退到最后连续的0x00之前
				startCodeLen = 0;
				continue;
			}
		}
		else if (c == 0x01 && (startCodeLen == 2 || startCodeLen == 3))
		{
			startCodeLen++;
			break;
		}
		else
			startCodeLen = 0;
	}
	int naluNextPos = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - startCodeLen;
	m_pStreamBuf->pos -= startCodeLen;	///< 调整指针，指向Nalu开头

	// 4. 获取Nalu长度naluLen(下个startCode位置 - 当前startCode位置)
	size_t naluLen = naluNextPos - naluPos;

	// 5. 文件指针重定位到当前startCode
	// 6. 读取naluLen个字节保存到参数naluData中传出
	// 7. 文件指针还原到原来位置
	if (naluData)
	{
		*naluData = new char[naluLen];
		int filePtrPos = m_fileStream.tellg();
		m_fileStream.seekg(naluPos);
		readNextBytes(*naluData, naluLen);
		m_fileStream.seekg(filePtrPos);
	}

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "nextNalu " << time_diff << endl;
#endif

	// 8. 返回naluLen
	return naluLen;
}


//************************************
// 函数名:	H264Analysis::next_SPS_Nalu
// 描述:	获取下一个包含SPS的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_SPS_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete [] naluDataTmp
	 *		非空：则 *naluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete [] naluDataTmp
	 * 5. 返回长度
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	char *naluDataTmp = NULL;

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SPS)
		{
			// 3. 若为要找类型，则判断naluData是否为空:
			if (naluData) ///< 非空：则 *naluData = naluDataTmp
				*naluData = naluDataTmp;
			else	///< 为空：则 delete [] naluDataTmp
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
			// 4. 返回长度
			return naluLen;
		}
		else
		{
			delete [] naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_PPS_Nalu
// 描述:	获取下一个包含PPS的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_PPS_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete [] naluDataTmp
	 *		非空：则 *naluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete [] naluDataTmp
	 * 5. 返回长度
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	char *naluDataTmp = NULL;

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_PPS)
		{
			// 3. 若为要找类型，则判断naluData是否为空:
			if (naluData) ///< 非空：则 *naluData = naluDataTmp
				*naluData = naluDataTmp;
			else	///< 为空：则 delete [] naluDataTmp
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
			// 4. 返回长度
			return naluLen;
		}
		else
		{
			delete [] naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_IDR_Nalu
// 描述:	获取下一个包含IDR帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/30
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_IDR_Nalu( char **naluData /*= NULL*/ )
{
	#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete [] naluDataTmp
	 *		非空：则 *naluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete [] naluDataTmp
	 * 5. 返回长度
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	unsigned int egcDataPos = 0;
	unsigned int egcDataLen = 0;
	unsigned int egcSize = 0;
	char *naluDataTmp = NULL;
	bool flag = 0;
	int flagCount = 0;
	int flagFilePtrPos = 0;
	int filePtrPos = 0;
	while (naluLen = nextNalu(&naluDataTmp))	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		startCodeLen = scLen(naluDataTmp);

		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;

		// 将SPS位置记录，若后方第2个或第3个为I帧，则把从该SPS位置开始到I帧结束的数据放入naluData中传出
		if (!flag && 
			(nal_unit_type == NAL_SPS ||
			nal_unit_type == NAL_PPS ||
			nal_unit_type == NAL_SEI)
			)
		{
			flag = true;
			flagFilePtrPos = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - naluLen;
		}
		if (flag)
			flagCount++;
		if (flagCount > 4)
		{
			flagCount = 0;
			flag = false;
			flagFilePtrPos = 0;
		}

		if (nal_unit_type == NAL_IDR_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				// 3. 若为要找类型，则判断naluData是否为空:
				if (naluData) ///< 非空：则 *naluData = naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - flagFilePtrPos;	///< 要拷贝的长度
						int filePtrPos = m_fileStream.tellg();
						m_fileStream.seekg(flagFilePtrPos);
						*naluData = new char[naluLen];
						if (readNextBytes(*naluData, naluLen) < naluLen)
							throw std::exception();
						m_fileStream.seekg(filePtrPos);
						flag = false;
						flagCount = 0;
						delete [] naluDataTmp;
						naluDataTmp = NULL;
					}
					else
						*naluData = naluDataTmp;
				}
				else	///< 为空：则 delete [] naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - flagFilePtrPos;
						flag = false;
						flagCount = 0;
					}
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. 返回长度
				return naluLen;
			}
			else
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete [] naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}

// rtsp rtp rtcp sdp

//************************************
// 函数名:	H264Analysis::next_I_Nalu
// 描述:	描述:	获取下一个包含I帧的Nalu(若前面有SPS,PPS或SEI则会将他们与I帧一起打包), 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 参数: 	unsigned int speed(in: 默认为1，代表正常速度找下一个I帧，speed为2时代表每2个I帧只播第一个I帧，后面依次类推)
// 日期: 	2016/06/02
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_I_Nalu( char **naluData /*= NULL*/, unsigned int speed /*= 1*/)
{
	if (speed <= 0)
		throw std::exception();

	size_t len = nextInalu(naluData);
	while (--speed)
		nextInalu();
	return len;
}

//************************************
// 函数名:	H264Analysis::next_P_Nalu
// 描述:	获取下一个包含P帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_P_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete [] naluDataTmp
	 *		非空：则 *naluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete [] naluDataTmp
	 * 5. 返回长度
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	unsigned int egcDataPos = 0;
	unsigned int egcDataLen = 0;
	unsigned int egcSize = 0;
	char *naluDataTmp = NULL;

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零


			if (slice_type == SLICE_TYPE_P1 || slice_type == SLICE_TYPE_P2)
			{
				// 3. 若为要找类型，则判断naluData是否为空:
				if (naluData) ///< 非空：则 *naluData = naluDataTmp
					*naluData = naluDataTmp;
				else	///< 为空：则 delete [] naluDataTmp
				{
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. 返回长度
				return naluLen;
			}
			else
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}
			

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete [] naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_B_Nalu
// 描述:	获取下一个包含B帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_B_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
			/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete [] naluDataTmp
	 *		非空：则 *naluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete [] naluDataTmp
	 * 5. 返回长度
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	unsigned int egcDataPos = 0;
	unsigned int egcDataLen = 0;
	unsigned int egcSize = 0;
	char *naluDataTmp = NULL;

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零


			if (slice_type == SLICE_TYPE_B1 || slice_type == SLICE_TYPE_B2)
			{
				// 3. 若为要找类型，则判断naluData是否为空:
				if (naluData) ///< 非空：则 *naluData = naluDataTmp
					*naluData = naluDataTmp;
				else	///< 为空：则 delete [] naluDataTmp
				{
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. 返回长度
				return naluLen;
			}
			else
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete [] naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_SI_Nalu
// 描述:	获取下一个包含SI帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_SI_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
			/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete [] naluDataTmp
	 *		非空：则 *naluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete [] naluDataTmp
	 * 5. 返回长度
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	unsigned int egcDataPos = 0;
	unsigned int egcDataLen = 0;
	unsigned int egcSize = 0;
	char *naluDataTmp = NULL;

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零


			if (slice_type == SLICE_TYPE_SI1 || slice_type == SLICE_TYPE_SI2)
			{
				// 3. 若为要找类型，则判断naluData是否为空:
				if (naluData) ///< 非空：则 *naluData = naluDataTmp
					*naluData = naluDataTmp;
				else	///< 为空：则 delete [] naluDataTmp
				{
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. 返回长度
				return naluLen;
			}
			else
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete [] naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}

//************************************
// 函数名:	H264Analysis::next_SP_Nalu
// 描述:	获取下一个包含SP帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::next_SP_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
			/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete [] naluDataTmp
	 *		非空：则 *naluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete [] naluDataTmp
	 * 5. 返回长度
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	unsigned int egcDataPos = 0;
	unsigned int egcDataLen = 0;
	unsigned int egcSize = 0;
	char *naluDataTmp = NULL;

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零


			if (slice_type == SLICE_TYPE_SP1 || slice_type == SLICE_TYPE_SP2)
			{
				// 3. 若为要找类型，则判断naluData是否为空:
				if (naluData) ///< 非空：则 *naluData = naluDataTmp
					*naluData = naluDataTmp;
				else	///< 为空：则 delete [] naluDataTmp
				{
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. 返回长度
				return naluLen;
			}
			else
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete [] naluDataTmp;
			naluDataTmp = NULL;
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

	size_t pos = m_len * (persent * 0.01);
	m_fileStream.seekg(pos);
	
	// 清空缓存
	if (m_pStreamBuf->buf)
	{
		delete [] m_pStreamBuf->buf;
		m_pStreamBuf->buf = NULL;
		m_pStreamBuf->len = 0;
		m_pStreamBuf->pos = 0;
		m_pStreamBuf->tellgBase = 0;
	}

	// 找到下一个I帧，并将缓冲区指针定位到该I帧的头部前（若包含SPS,PPS，则指向SPS前）
	size_t len = next_I_Nalu();
	if (m_pStreamBuf->pos < len)
		throw std::exception();
	m_pStreamBuf->pos -= len;
	
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "skipTo " << time_diff << endl;
#endif

	return Success;
}



//************************************
// 函数名:	H264Analysis::getNaluType
// 描述:	获取Nalu的类型
// 返回值:	NalUnitType => Nalu类型
// 参数: 	char * naluData(in: 传入Nalu数据)
// 日期: 	2016/05/27
// 作者: 	YJZ
// 修改记录:
//************************************
NalUnitType H264Analysis::getNaluType( char *naluData )
{
	return (NalUnitType)B8_VAL_BASE_R(naluData[scLen(naluData)], 3, 5);
}

//************************************
// 函数名:	H264Analysis::getSliceType
// 描述:	获取帧类型
// 返回值:	SliceType => 帧类型
// 参数: 	char * naluData(in: 传入Nalu数据)
// 参数: 	size_t naluLen(in: 传入Nalu长度)
// 日期: 	2016/05/27
// 作者: 	YJZ
// 修改记录:
//************************************
SliceType H264Analysis::getSliceType( char *naluData , size_t naluLen )
{
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	unsigned int egcDataPos = 0;
	unsigned int egcDataLen = 0;
	unsigned int egcSize = 0;
	startCodeLen = scLen(naluData);
	nal_unit_type = getNaluType(naluData);
	if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
	{
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
		if (ueDecode(&naluData[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
			ueDecode(&naluData[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
		{
			throw std::exception();
		}
		m_binPos = 0;	///< 二进制指针位置归零
		m_lastByte = 0;	///< 字节归零

		return (SliceType)slice_type;
	}
	else
		return SLICE_TYPE_NONE;
}

//************************************
// 函数名:	H264Analysis::scLen
// 描述:	返回startCode长度
// 返回值:	size_t => startCode长度
// 参数: 	char * p
// 日期: 	2016/05/25
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::scLen(char *p)
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	size_t len = 0;
	// 跳过头部
	if (p[0] == 0x00 && 
		p[1] == 0x00)
	{
		if (p[2] == 0x01)
			len = 3;
		else if (p[2] == 0x00 &&
			p[3] == 0x01)
			len = 4;
		else
			throw std::exception();
	} 
	else
		throw std::exception();

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "skipNaluStartCode " << time_diff << endl;
#endif

	return len;
}

//************************************
// 函数名:	H264Analysis::ueDecode
// 描述:	解码Exp-Golomb-Code(指数哥伦布编码)
// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
// 参数:	char *egcData(in: 传入要解码的Exp-Golomb-Code数据)
// 参数: 	size_t len(in: 传入Exp-Golomb-Code数据的长度)
// 参数: 	UINT32 * codeNum(out: 传出解码后的值)
// 参数:	unsigned int * egcSize(out: 传出解码用的字节数)
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
STATUS H264Analysis::ueDecode(char *egcData, size_t len, UINT32 *codeNum, unsigned int *egcSize)
{
	/**
	 * 1. 
	 */
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	char *pEgcData = egcData;
/*	char *pEgcDataBeg = egcData;*/
	int leadingZeroBits = -1;
	unsigned int egcPtrPos = 0;
	for (char b = 0; !b; leadingZeroBits++)
	{
		if (m_binPos == 0)
		{
			if (egcPtrPos == len)
				return Failed;

			m_lastByte = pEgcData[egcPtrPos];
			//egcPtrPos = pEgcData - pEgcDataBeg;
			egcPtrPos++;
		}
		b = B8_VAL_BASE_R(m_lastByte, m_binPos, 1);	// read next bit
		m_binPos++;	// 指针下移
		if (m_binPos % 8 == 0)
			m_binPos = 0;
	}

	if (leadingZeroBits > 32)
	{
		throw std::exception();
		return -1;
	}

	// 指针还有剩余位置未读
	UINT32 readBits = 0;
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
			
				if (egcPtrPos + leftLeadingZeroBytes >= len)
					return Failed;

				/**
				 * 原最后字节剩余位置n位不够满足哥伦布解码，于是取出后面的若干字节(经计算出，需要后面多少字节才可解码出EGC),
				 * 先将后面字节放入UINT32类型的golomb中，并字节倒序，进行提取，只提取要计算EGC而需要的若干位(位操作),
				 * 然后将readBits向右移n位，用原最后字节剩余位置的n位进行填充(位操作)，最终得出readbits的二进制值
				 */
				memcpy(p, &pEgcData[egcPtrPos], leftLeadingZeroBytes);
				egcPtrPos += leftLeadingZeroBytes;
				memcpy((void*)&readBits, p, leftLeadingZeroBytes);
				bytes_reverse((char*)&readBits, sizeof(readBits));

				// 提取完整的Exp-Golomb-Code
				readBits &= B32_VAL_MASK(0, leftLeadingZeroBits);
				readBits = (readBits>>leftBits)|(B32_VAL_BASE_L((UINT32)B8_VAL_BASE_L(m_lastByte, m_binPos), 24));
				readBits = B32_VAL_BASE_R(readBits, 0, leadingZeroBits);

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
			int leftLeadingZeroBits = leadingZeroBits - leftBits;	
			int leftLeadingZeroBytes = bits_get_byte_num(leftLeadingZeroBits); 
			char *p = new char[leadingZeroBytes];
			
 			if (egcPtrPos + leftLeadingZeroBytes >= len)
 				return Failed;
 			memcpy(p, &pEgcData[egcPtrPos], leftLeadingZeroBytes);
 			egcPtrPos += leftLeadingZeroBytes;
			memcpy((void*)&readBits, p, leadingZeroBytes);
			bytes_reverse((char*)&readBits, sizeof(readBits));

			// 提取完整的Exp-Golomb-Code
			readBits &= B32_VAL_MASK(0, leadingZeroBits);
			readBits = B32_VAL_BASE_R(readBits, 0, leadingZeroBits);

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

	*egcSize = egcPtrPos; ///< 返回解码EGC时读取的字节数
	return Success;
}

//************************************
// 函数名:	H264Analysis::next_I_Nalu
// 描述:	获取下一个包含I帧的Nalu(若前面有SPS,PPS或SEI则会将他们与I帧一起打包), 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
// 返回值:	size_t => Nalu长度（包含startCode）
// 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::nextInalu( char **naluData /*= NULL*/ )
{
	#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete [] naluDataTmp
	 *		非空：则 *naluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete [] naluDataTmp
	 * 5. 返回长度
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	unsigned int egcDataPos = 0;
	unsigned int egcDataLen = 0;
	unsigned int egcSize = 0;
	char *naluDataTmp = NULL;
	bool flag = 0;
	int flagCount = 0;
	int flagFilePtrPos = 0;
	int filePtrPos = 0;
	while (naluLen = nextNalu(&naluDataTmp))	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		startCodeLen = scLen(naluDataTmp);

		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;

		// 将SPS位置记录，若后方第2个或第3个为I帧，则把从该SPS位置开始到I帧结束的数据放入naluData中传出
		if (!flag && 
			(nal_unit_type == NAL_SPS ||
			nal_unit_type == NAL_PPS ||
			nal_unit_type == NAL_SEI)
			)
		{
			flag = true;
			flagFilePtrPos = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - naluLen;
		}
		if (flag)
			flagCount++;
		if (flagCount > 4)
		{
			flagCount = 0;
			flag = false;
			flagFilePtrPos = 0;
		}

		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				// 3. 若为要找类型，则判断naluData是否为空:
				if (naluData) ///< 非空：则 *naluData = naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - flagFilePtrPos;	///< 要拷贝的长度
						int filePtrPos = m_fileStream.tellg();
						m_fileStream.seekg(flagFilePtrPos);
						*naluData = new char[naluLen];
						if (readNextBytes(*naluData, naluLen) < naluLen)
							throw std::exception();
						m_fileStream.seekg(filePtrPos);
						flag = false;
						flagCount = 0;
						delete [] naluDataTmp;
						naluDataTmp = NULL;
					}
					else
						*naluData = naluDataTmp;
				}
				else	///< 为空：则 delete [] naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - flagFilePtrPos;
						flag = false;
						flagCount = 0;
					}
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. 返回长度
				return naluLen;
			}
			else
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete [] naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}


//************************************
// 函数名:	H264Analysis::readNextBytes
// 描述:	读取接下来的len个字节
// 返回值:	size_t => 成功读取的字节数
// 参数: 	char * p(out: 传出读取的字节数据，需要调用者分配和释放内存)
// 参数: 	int len(in: 读取字节的长度)
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
size_t H264Analysis::readNextBytes( char *p, int len)
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	size_t readLen = len;
	size_t filePtrPos = m_fileStream.tellg();
	if (filePtrPos + len > m_len)
	{
		readLen = m_len - filePtrPos;
		m_fileStream.read(p, readLen);
	}
	else
		m_fileStream.read(p, len);
	
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "readNextBytes " << time_diff << endl;
#endif

	return readLen;
}



//************************************
// 函数名:	H264Analysis::checkStreamBuf
// 描述:	检查缓冲区的数据是否还够，若不够则再次读取BUFSIZE字节的文件内容
// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
// 日期: 	2016/05/24
// 作者: 	YJZ
// 修改记录:
//************************************
STATUS H264Analysis::checkStreamBuf()
{
	if (m_pStreamBuf->pos >= m_pStreamBuf->len) 
	{
		if (m_pStreamBuf->buf)
		{
			delete [] m_pStreamBuf->buf;
			m_pStreamBuf->buf = NULL;
			m_pStreamBuf->len = 0;
			m_pStreamBuf->pos = 0;
			m_pStreamBuf->tellgBase = 0;
		}

		m_pStreamBuf->tellgBase = m_fileStream.tellg();
		if (m_pStreamBuf->tellgBase >= m_len)	///< 是否到达文件结尾
			return Failed;

		m_pStreamBuf->buf = new char[BUFSIZE];
		m_pStreamBuf->len = readNextBytes(m_pStreamBuf->buf, BUFSIZE);
		m_pStreamBuf->pos = 0;
	}
	return Success;
}