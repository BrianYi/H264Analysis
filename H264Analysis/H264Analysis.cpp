#include "H264Analysis.h"
#include "H264PublicDef.h"
#include <math.h>

H264Analysis::H264Analysis(void)
{
	/// 初始化流结构
	m_fileLen = 0;
	m_lastByte = 0;
	m_binPos = 0;

	m_pStreamBuf = new DataStream;
	m_pStreamBuf->beg = NULL;
	m_pStreamBuf->end = NULL;
	m_pStreamBuf->p   = NULL;
	m_pStreamBuf->len = 0;
	m_pStreamBuf->tellgBase = 0;

	m_frameRate = 25;
	m_naluCount = 0;
	m_frameTotalTime = 0.0;

#ifdef TIME_TEST
	m_debugFileStream.open("log.txt", ios::trunc | ios::out);
#endif
}


H264Analysis::~H264Analysis(void)
{
	if (m_fileStream.is_open())
		CloseFile();

	if (m_pStreamBuf)
	{
		if (m_pStreamBuf->beg)
			delete m_pStreamBuf->beg;
		m_pStreamBuf->beg = NULL;
		m_pStreamBuf->end = NULL;
		m_pStreamBuf->p	= NULL;
		m_pStreamBuf->len = 0;
		m_pStreamBuf->tellgBase = 0;
		delete m_pStreamBuf;
	}

	if (m_pFilePath)
	{
		delete m_pFilePath;
		m_pFilePath = NULL;
	}

#ifdef TIME_TEST
	if (m_debugFileStream.is_open())
		m_debugFileStream.close();
#endif
}


//************************************
// 函数名:	H264Analysis::GetOpenFile
// 描述:	获取文件流
// 返回值:	ifstream& 
// 参数: 	const char* fileName
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
std::ifstream& H264Analysis::GetOpenFile( const char* fileName )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/// 初始化流结构
	m_fileLen = 0;
	m_lastByte = 0;
	m_binPos = 0;

	/// 文件流数据存放到流结构中
	UINT32 fileLen = strlen(fileName);
	m_pFilePath = new char[fileLen+1];
	memset(m_pFilePath, 0, fileLen+1);
	strcpy(m_pFilePath, fileName);
	if (m_fileStream.is_open())
	{
		// 关闭已经打开的文件
		m_fileStream.close();

		// 清空缓冲区
		ClearStreamBuf();
	}
	m_fileStream.open(m_pFilePath, std::ios::binary | std::ios::in);
	if (m_fileStream == NULL)
		return m_fileStream;
	m_fileStream.seekg(0, std::ios::end);
	m_fileLen = m_fileStream.tellg();
	m_fileStream.seekg(0);

	/// 读满缓冲区
	ClearStreamBuf();
	m_pStreamBuf->beg = new char[BUFSIZE];
	m_pStreamBuf->len = BUFSIZE;
	UINT32 outLengthRead = 0;
	if (Read(m_pStreamBuf->beg, &m_pStreamBuf->len, &outLengthRead) == Failed)
		m_pStreamBuf->len = outLengthRead;	// 读取数据不足时的处理
	m_pStreamBuf->end = m_pStreamBuf->beg + m_pStreamBuf->len;
	m_pStreamBuf->p = m_pStreamBuf->beg;
	
	/// 计算: 1.文件总Nalu数量 2.文件总时间
	m_naluCount = NaluCount();	// 这里耗时很多,容易造成连接失败
	m_frameTotalTime = (float)m_naluCount / m_frameRate;


	/// 读满缓冲区
	ClearStreamBuf();
	m_pStreamBuf->beg = new char[BUFSIZE];
	m_pStreamBuf->len = BUFSIZE;
	outLengthRead = 0;
	if (Read(m_pStreamBuf->beg, &m_pStreamBuf->len, &outLengthRead) == Failed)
		m_pStreamBuf->len = outLengthRead;	// 读取数据不足时的处理
	m_pStreamBuf->end = m_pStreamBuf->beg + m_pStreamBuf->len;
	m_pStreamBuf->p = m_pStreamBuf->beg;
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "GetOpenFile " << time_diff << endl;
#endif
	return m_fileStream;
}


//************************************
// 函数名:	H264Analysis::CloseFile
// 描述:	关闭文件
// 返回值:	void
// 日期: 	2016/05/19
// 作者: 	YJZ
// 修改记录:
//************************************
void H264Analysis::CloseFile()
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	if (m_fileStream.is_open())
	{
		m_fileStream.close();

		// 清空缓冲区
		ClearStreamBuf();

		m_fileLen = 0;
		m_binPos = 0;
		m_lastByte = 0;
	}
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "CloseFile " << time_diff << endl;
#endif
}

//************************************
// 函数名:	H264Analysis::NextNalu
// 描述:	获取下一个Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 日期: 	2016/05/16
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::NextNalu(char **outNaluData, UINT32 *outLength)
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
	 * 6. 读取naluLen个字节保存到参数outNaluData中传出
	 * 7. 文件指针还原到原来位置
	 * 8. 返回naluLen
	 */

	//char c;
	UINT32 startCodeLen = 0;

	// 1.找到当前startCode，获取文件位置
	// 2.跳过头部
	while (true)
	{
		if (m_pStreamBuf->p >= m_pStreamBuf->end)
		{
			if (CheckStreamBuf() == Failed)
				return Failed;
		}

		if (*m_pStreamBuf->p == 0x00)
		{
			startCodeLen++;
		}
		else if (*m_pStreamBuf->p == 0x01 && (startCodeLen == 2 || startCodeLen == 3))
		{
			startCodeLen++;
			m_pStreamBuf->p++;
			break;
		}
		else
			startCodeLen = 0;
		m_pStreamBuf->p++;
	}
	INT32 naluPos = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - startCodeLen;

	// 3. 找到下个startCode，获取文件位置
	startCodeLen = 0;
	while (true)
	{
		if (m_pStreamBuf->p >= m_pStreamBuf->end)
		{
			if (CheckStreamBuf() == Failed) ///< 已到文件结尾
				break;
		}

		if (*m_pStreamBuf->p == 0x00)
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
			if (m_pStreamBuf->p >= m_pStreamBuf->end)	// 缓冲区尾为startCode情况
			{
				m_fileStream.seekg(m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - startCodeLen); // 文件指针回退到最后连续的0x00之前
				startCodeLen = 0;
				continue;
			}
		}
		else if (*m_pStreamBuf->p == 0x01 && (startCodeLen == 2 || startCodeLen == 3))
		{
			startCodeLen++;
			m_pStreamBuf->p++;
			break;
		}
		else
			startCodeLen = 0;
		m_pStreamBuf->p++;
	}
	INT32 naluNextPos = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - startCodeLen;
	m_pStreamBuf->p -= startCodeLen;	///< 调整指针，指向Nalu开头

	// 4. 获取Nalu长度naluLen(下个startCode位置 - 当前startCode位置)
	UINT32 naluLen = naluNextPos - naluPos;

	// 5. 文件指针重定位到当前startCode
	// 6. 读取naluLen个字节保存到参数outNaluData中传出
	// 7. 文件指针还原到原来位置
	if (outNaluData)
	{
		*outNaluData = new char[naluLen];
		int filePtrPos = m_fileStream.tellg();
		UINT32 outLengthRead = 0;
		m_fileStream.seekg(naluPos);
		if (Read(*outNaluData, &naluLen, &outLengthRead) == Failed)	throw ;
		m_fileStream.seekg(filePtrPos);
	}

	// 8. 返回naluLen
	if (outLength)
		*outLength = naluLen;

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "NextNalu " << time_diff << endl;
#endif

	
	return Success;
}


//************************************
// 函数名:	H264Analysis::NextSpsNalu
// 描述:	获取下一个包含SPS的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::NextSpsNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete naluDataTmp
	 *		非空：则 *outNaluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete naluDataTmp
	 * 5. 返回长度
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	char *naluDataTmp = NULL;

	while (NextNalu(&naluDataTmp,&naluLen) == Success)	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		if (GetStartCodeLength(naluDataTmp, &startCodeLen) == Failed)	throw ;
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SPS)
		{
			// 3. 若为要找类型，则判断outNaluData是否为空:
			if (outNaluData) ///< 非空：则 *outNaluData = naluDataTmp
				*outNaluData = naluDataTmp;
			else	///< 为空：则 delete naluDataTmp
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}

			// 4. 返回长度
			if (outLength)
				*outLength = naluLen;
#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
			return Success;
		}
		else
		{
			delete naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}

//************************************
// 函数名:	H264Analysis::NextPpsNalu
// 描述:	获取下一个包含PPS的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::NextPpsNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断outNaluData是否为空:
	 *		为空：则 delete [] naluDataTmp
	 *		非空：则 *outNaluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete [] naluDataTmp
	 * 5. 返回长度
	 */

	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断outNaluData是否为空:
	 *		为空：则 delete naluDataTmp
	 *		非空：则 *outNaluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete naluDataTmp
	 * 5. 返回长度
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	char *naluDataTmp = NULL;

	while (NextNalu(&naluDataTmp,&naluLen) == Success)	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_PPS)
		{
			// 3. 若为要找类型，则判断outNaluData是否为空:
			if (outNaluData) ///< 非空：则 *outNaluData = naluDataTmp
				*outNaluData = naluDataTmp;
			else	///< 为空：则 delete naluDataTmp
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}
			// 4. 返回长度
			if (outLength)
				*outLength = naluLen;
#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
			
			return Success;
		}
		else
		{
			delete naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}
	return Failed;
}

//************************************
// 函数名:	H264Analysis::NextIdrNalu
// 描述:	获取下一个包含IDR帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 日期: 	2016/05/30
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::NextIdrNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断outNaluData是否为空:
	 *		为空：则 delete naluDataTmp
	 *		非空：则 *outNaluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete naluDataTmp
	 * 5. 返回长度
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	UINT32 egcDataPos = 0;
	UINT32 egcDataLen = 0;
	UINT32 egcSize = 0;
	char *naluDataTmp = NULL;
	bool flag = 0;
	UINT32 flagCount = 0;
	UINT32 flagFilePtrPos = 0;
	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)
			throw ;

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
			flagFilePtrPos = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - naluLen;
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
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				// 3. 若为要找类型，则判断outNaluData是否为空:
				if (outNaluData) ///< 非空：则 *outNaluData = naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - flagFilePtrPos;	///< 要拷贝的长度
						UINT32 filePtrPos = m_fileStream.tellg();
						m_fileStream.seekg(flagFilePtrPos);
						*outNaluData = new char[naluLen];
						UINT32 outLengthRead = 0;
						if (Read(*outNaluData, &naluLen, &outLengthRead) == Failed)	throw ;
						m_fileStream.seekg(filePtrPos);
						flag = false;
						flagCount = 0;
						delete naluDataTmp;
						naluDataTmp = NULL;
					}
					else
						*outNaluData = naluDataTmp;
				}
				else	///< 为空：则 delete naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - flagFilePtrPos;
						flag = false;
						flagCount = 0;
					}
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. 返回长度
				if (outLength)
					*outLength = naluLen;

				return Success;
			}
			else
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}
	return Failed;
}

// rtsp rtp rtcp sdp

//************************************
// 函数名:	H264Analysis::NextInalu
// 描述:	描述:	获取下一个包含I帧的Nalu(若前面有SPS,PPS或SEI则会将他们与I帧一起打包), 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 参数: 	UINT16 inSpeed(in: 默认为1，代表正常速度找下一个I帧，inSpeed为2时代表每2个I帧只播第一个I帧，后面依次类推)
// 日期: 	2016/06/02
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::NextInalu( char **outNaluData, UINT32 *outLength, UINT16 inSpeed /*= 1*/)
{
	if (inSpeed < 1)	throw ;

	if (Next_I_Nalu(outNaluData, outLength) == Failed)
		return Failed;

	while (--inSpeed)
		if (Next_I_Nalu(NULL,NULL) == Failed)
			return Failed;

	return Success;
}


//************************************
// 函数名:	H264Analysis::NextPnalu
// 描述:	获取下一个包含P帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::NextPnalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断outNaluData是否为空:
	 *		为空：则 delete naluDataTmp
	 *		非空：则 *outNaluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete naluDataTmp
	 * 5. 返回长度
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	UINT32 egcDataPos = 0;
	UINT32 egcDataLen = 0;
	UINT32 egcSize = 0;
	char *naluDataTmp = NULL;

	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零


			if (slice_type == SLICE_TYPE_P1 || slice_type == SLICE_TYPE_P2)
			{
				// 3. 若为要找类型，则判断outNaluData是否为空:
				if (outNaluData) ///< 非空：则 *outNaluData = naluDataTmp
					*outNaluData = naluDataTmp;
				else	///< 为空：则 delete naluDataTmp
				{
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. 返回长度
				if (outLength)
					*outLength = naluLen;

				return Success;
			}
			else
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}

//************************************
// 函数名:	H264Analysis::NextBnalu
// 描述:	获取下一个包含B帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::NextBnalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断outNaluData是否为空:
	 *		为空：则 delete naluDataTmp
	 *		非空：则 *outNaluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete naluDataTmp
	 * 5. 返回长度
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	UINT32 egcDataPos = 0;
	UINT32 egcDataLen = 0;
	UINT32 egcSize = 0;
	char *naluDataTmp = NULL;

	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零


			if (slice_type == SLICE_TYPE_B1 || slice_type == SLICE_TYPE_B2)
			{
				// 3. 若为要找类型，则判断outNaluData是否为空:
				if (outNaluData) ///< 非空：则 *outNaluData = naluDataTmp
					*outNaluData = naluDataTmp;
				else	///< 为空：则 delete naluDataTmp
				{
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. 返回长度
				if (outLength)
					*outLength = naluLen;

				return Success;
			}
			else
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}

//************************************
// 函数名:	H264Analysis::NextSiNalu
// 描述:	获取下一个包含SI帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::NextSiNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断outNaluData是否为空:
	 *		为空：则 delete naluDataTmp
	 *		非空：则 *outNaluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete naluDataTmp
	 * 5. 返回长度
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	UINT32 egcDataPos = 0;
	UINT32 egcDataLen = 0;
	UINT32 egcSize = 0;
	char *naluDataTmp = NULL;

	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零


			if (slice_type == SLICE_TYPE_SI1 || slice_type == SLICE_TYPE_SI2)
			{
				// 3. 若为要找类型，则判断outNaluData是否为空:
				if (outNaluData) ///< 非空：则 *outNaluData = naluDataTmp
					*outNaluData = naluDataTmp;
				else	///< 为空：则 delete naluDataTmp
				{
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. 返回长度
				if (outLength)
					*outLength = naluLen;

				return Success;
			}
			else
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}


	return Failed;
}

//************************************
// 函数名:	H264Analysis::NextSpNalu
// 描述:	获取下一个包含SP帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::NextSpNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断outNaluData是否为空:
	 *		为空：则 delete naluDataTmp
	 *		非空：则 *outNaluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete naluDataTmp
	 * 5. 返回长度
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	UINT32 egcDataPos = 0;
	UINT32 egcDataLen = 0;
	UINT32 egcSize = 0;
	char *naluDataTmp = NULL;

	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. 获取下一字节，判断Nalu类型
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零


			if (slice_type == SLICE_TYPE_SP1 || slice_type == SLICE_TYPE_SP2)
			{
				// 3. 若为要找类型，则判断outNaluData是否为空:
				if (outNaluData) ///< 非空：则 *outNaluData = naluDataTmp
					*outNaluData = naluDataTmp;
				else	///< 为空：则 delete naluDataTmp
				{
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. 返回长度
				if (outLength)
					*outLength = naluLen;

				return Success;
			}
			else
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}


//************************************
// 函数名:	H264Analysis::SeekDstToPos
// 描述:	跳转到指定位置(百分比表示0.0~1.0)
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数: 	float inPersent(in: 0.0~1.0, 传入要跳转的百分比)
// 日期: 	2016/05/23
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::SeekDstToPos( float inPersent )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif

	if (inPersent < 0 || inPersent > 1)
		return Failed;

	UINT32 pos = m_fileLen * inPersent;
	m_fileStream.seekg(pos);

	// 清空缓存
	ClearStreamBuf();

	// 分配内存
	m_pStreamBuf->tellgBase = m_fileStream.tellg();
	m_pStreamBuf->beg = new char[BUFSIZE];
	m_pStreamBuf->len = BUFSIZE;
	UINT32 outLengthRead = 0;
	if (Read(m_pStreamBuf->beg, &m_pStreamBuf->len, &outLengthRead) == Failed)
		m_pStreamBuf->len = outLengthRead;	///< 最后一次读取数据不足时，不退出
	m_pStreamBuf->end = m_pStreamBuf->beg + m_pStreamBuf->len;
	m_pStreamBuf->p = m_pStreamBuf->beg;

	// 找到下一个I帧，并将缓冲区指针定位到该I帧的头部前（若包含SPS,PPS，则指向SPS前）
	UINT32 naluLength = 0;
	if (Next_I_Nalu(NULL, &naluLength) == Failed)
		return Failed;

	if (m_pStreamBuf->p - m_pStreamBuf->beg < naluLength)
		throw ;
	m_pStreamBuf->p -= naluLength;
	
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "SeekDstToPos " << time_diff << endl;
#endif

	return Success;
}



//************************************
// 函数名:	H264Analysis::GetNaluType
// 描述:	获取Nalu的类型
// 返回值:	NalUnitType => Nalu类型
// 参数: 	char * inNaluData(in: 传入Nalu数据)
// 日期: 	2016/05/27
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::NalUnitType H264Analysis::GetNaluType( char *inNaluData )
{
	UINT32 startCodeLength = 0;
	if (GetStartCodeLength(inNaluData,&startCodeLength) == Failed)	throw;
	return (NalUnitType)B8_VAL_BASE_R(inNaluData[startCodeLength], 3, 5);
}

//************************************
// 函数名:	H264Analysis::GetSliceType
// 描述:	获取帧类型
// 返回值:	SliceType => 帧类型
// 参数: 	char * inNaluData(in: 传入Nalu数据)
// 参数: 	UINT32 inLength(in: 传入Nalu长度)
// 日期: 	2016/05/27
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::SliceType H264Analysis::GetSliceType( char *inNaluData , UINT32 inLength )
{
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	UINT32 egcDataPos = 0;
	UINT32 egcDataLen = 0;
	UINT32 egcSize = 0;
	if (GetStartCodeLength(inNaluData, &startCodeLen) == Failed)	throw;
	nal_unit_type = GetNaluType(inNaluData);
	if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
	{
		egcDataPos = startCodeLen + 1;
		egcDataLen = inLength - egcDataPos;
		// EGC解码第一次获得first_mb_in_slice, EGC解码第二次获得slice_type
		if (ParseUeExpGolombCode(&inNaluData[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
			ParseUeExpGolombCode(&inNaluData[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
			throw ;

		m_binPos = 0;	///< 二进制指针位置归零
		m_lastByte = 0;	///< 字节归零

		return (SliceType)slice_type;
	}
	else
		return SLICE_TYPE_NONE;
}

//************************************
// 函数名:	H264Analysis::GetStartCodeLength
// 描述:	返回startCode长度
// 返回值:	size_t => startCode长度
// 参数: 	char * inNaluData
// 参数: 	UINT32 outLength(out: 传出Nalu长度)
// 日期: 	2016/05/25
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::GetStartCodeLength(char *inNaluData, UINT32 *outLength)
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	UINT32 naluLength = 0;
	// 跳过头部
	if (inNaluData[0] == 0x00 && 
		inNaluData[1] == 0x00)
	{
		if (inNaluData[2] == 0x01)
			naluLength = 3;
		else if (inNaluData[2] == 0x00 &&
			inNaluData[3] == 0x01)
			naluLength = 4;
		else
			return Failed;
	} 
	else
		return Failed;

	if (outLength)
		*outLength = naluLength;

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "skipNaluStartCode " << time_diff << endl;
#endif

	return Success;
}

//************************************
// 函数名:	H264Analysis::ParseUeExpGolombCode
// 描述:	解码Exp-Golomb-Code(指数哥伦布编码)
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *inUeExpGolombCodeData(in: 传入要解码的Exp-Golomb-Code数据)
// 参数: 	UINT32 inLength(in: 传入Exp-Golomb-Code数据的长度)
// 参数: 	UINT32 * outResult(out: 传出解码后的值)
// 参数:	UINT32 * outResultLength(out: 传出解码用的字节数)
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::ParseUeExpGolombCode(char *inUeExpGolombCodeData, UINT32 inLength, UINT32 *outResult, UINT32 *outResultLength)
{
	/**
	 * 1. 
	 */
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	char *pEgcData = inUeExpGolombCodeData;
/*	char *pEgcDataBeg = egcData;*/
	INT32 leadingZeroBits = -1;
	UINT32 egcPtrPos = 0;
	for (char b = 0; !b; leadingZeroBits++)
	{
		if (m_binPos == 0)
		{
			if (egcPtrPos == inLength)
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
		throw ;
		return Failed;
	}

	// 指针还有剩余位置未读
	UINT32 readBits = 0;
	INT32 leftBits = 8 - m_binPos;
	INT32 leadingZeroBytes = 0;
	if (m_binPos != 0)
	{
		if (leadingZeroBits == 0)
		{
			*outResult = 0;
		}
		else
		{
			if (leadingZeroBits <= leftBits)	// 剩余位置足够满足哥伦布解码
			{
				readBits = B8_VAL_BASE_R(m_lastByte, m_binPos, leadingZeroBits);
			}
			else	// 剩余位置不够满足哥伦布解码
			{
				INT32 leftLeadingZeroBits = leadingZeroBits - leftBits;
				INT32 leftLeadingZeroBytes = bits_get_byte_num(leftLeadingZeroBits);
				char *p = new char[leftLeadingZeroBytes];
			
				if (egcPtrPos + leftLeadingZeroBytes >= inLength)
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
				delete p;
			}
			m_binPos += leadingZeroBits;
			m_binPos %= 8;
			*outResult = pow(2.0, leadingZeroBits) - 1 + readBits;
		}
	}
	else	// 指针没有剩余位置未读
	{
		if (leadingZeroBits == 0)
		{
			*outResult = 0;
		}
		else
		{
			leadingZeroBytes = bits_get_byte_num(leadingZeroBits);
			INT32 leftLeadingZeroBits = leadingZeroBits - leftBits;	
			INT32 leftLeadingZeroBytes = bits_get_byte_num(leftLeadingZeroBits); 
			char *p = new char[leadingZeroBytes];
			
 			if (egcPtrPos + leftLeadingZeroBytes >= inLength)
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
			delete p;

			m_binPos += leadingZeroBits;
			m_binPos %= 8;
			*outResult = pow(2.0, leadingZeroBits) - 1 + readBits;
		}
	}

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "ParseUeExpGolombCode " << time_diff << endl;
#endif

	*outResultLength = egcPtrPos; ///< 返回解码EGC时读取的字节数
	return Success;
}


//************************************
// 函数名:	H264Analysis::CheckStreamBuf
// 描述:	检查缓冲区的数据是否还够，若不够则再次读取BUFSIZE字节的文件内容
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 日期: 	2016/05/24
// 作者: 	YJZ
// 修改记录:
//************************************
inline H264Analysis::StatusCode H264Analysis::CheckStreamBuf()
{
	if ((m_pStreamBuf->p >= m_pStreamBuf->end) && (m_pStreamBuf->end != NULL))
	{
		ClearStreamBuf();

		m_pStreamBuf->tellgBase = m_fileStream.tellg();
		if (m_pStreamBuf->tellgBase >= m_fileLen)	///< 是否到达文件结尾
			return Failed;

		m_pStreamBuf->beg = new char[BUFSIZE];
		m_pStreamBuf->len = BUFSIZE;
		UINT32 outLengthRead = 0;
		if (Read(m_pStreamBuf->beg, &m_pStreamBuf->len, &outLengthRead) == Failed)
			m_pStreamBuf->len = outLengthRead;	///< 最后一次读取数据不足时，不退出
		m_pStreamBuf->end = m_pStreamBuf->beg + m_pStreamBuf->len;
		m_pStreamBuf->p = m_pStreamBuf->beg;
		return Success;
	}
	return Failed;
}


//************************************
// 函数名:	H264Analysis::ClearStreamBuf
// 描述:	清空缓冲区
// 返回值:	void
// 日期: 	2016/06/15
// 作者: 	YJZ
// 修改记录:
//************************************
inline void H264Analysis::ClearStreamBuf()
{
	if (m_pStreamBuf->beg)
		delete m_pStreamBuf->beg;
	m_pStreamBuf->beg = NULL;
	m_pStreamBuf->end = NULL;
	m_pStreamBuf->len = 0;
	m_pStreamBuf->p = NULL;
	m_pStreamBuf->tellgBase = 0;
}

//************************************
// 函数名:	H264Analysis::SeekStreamBufPtrPos
// 描述:		将缓冲区指针移动到指定位置
// 返回值:	H264Analysis::StatusCode
// 参数: 	UINT32 inNewPos (in:传入新的位置，相对起始位置)
// 日期: 	2016/08/16
// 作者: 	YJZ
// 修改记录:	
//************************************
H264Analysis::StatusCode H264Analysis::SeekStreamBufPtrPos(UINT32 inNewPos)
{
	if (inNewPos >= m_pStreamBuf->len)
		return Failed;
	else
		m_pStreamBuf->p = m_pStreamBuf->beg + inNewPos;

	return Success;
}

//************************************
// 函数名:	H264Analysis::SeekStreamBufPtrPos
// 描述:		将缓冲区指针移动到指定位置
// 返回值:	H264Analysis::StatusCode
// 参数: 	INT32 inOffset (in:相对偏移量)
// 参数: 	StreamBufBase inWay (in:相对偏移类型)
// 日期: 	2016/08/16
// 作者: 	YJZ
// 修改记录:	
//************************************
H264Analysis::StatusCode H264Analysis::SeekStreamBufPtrPos(INT32 inOffset, StreamBufBase inWay)
{
	char *p = m_pStreamBuf->p;
	switch (inWay)
	{
	case StreamBufBase::beg:
		return SeekStreamBufPtrPos(inOffset);
		break;
	case StreamBufBase::cur:
		p += inOffset;
		break;
	case StreamBufBase::end:
		p = m_pStreamBuf->end;
		p += inOffset;
		break;
	default:
		break;
	}

	if (p >= m_pStreamBuf->beg && p <= m_pStreamBuf->end)
	{
		m_pStreamBuf->p = p;
		return Success;
	}

	return Failed;
}

//************************************
// 函数名:	H264Analysis::Next_I_Nalu
// 描述:	获取下一个包含I帧的Nalu(若前面有SPS,PPS或SEI则会将他们与I帧一起打包), 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数:	char *outNaluData	
//			1. Not NULL => 返回Nalu, 包含startCode
//			2. NULL		=> 不获取数据
// 参数:	UINT32 *outLength
//			1. Not NULL => 传出Nalu的长度
//			2. NULL		=> 不获取数据
// 日期: 	2016/05/17
// 作者: 	YJZ
// 修改记录:
//************************************
H264Analysis::StatusCode H264Analysis::Next_I_Nalu( char **outNaluData, UINT32 *outLength )
{
	#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	 * 2. 获取下一字节，判断Nalu类型
	 * 3. 若为要找类型，则判断naluData是否为空:
	 *		为空：则 delete naluDataTmp
	 *		非空：则 *naluData = naluDataTmp
	 * 4. 若不为要找的类型，则 delete naluDataTmp
	 * 5. 返回长度
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	UINT32 first_mb_in_slice = 0;
	SliceType slice_type = SLICE_TYPE_NONE;
	UINT32 egcDataPos = 0;
	UINT32 egcDataLen = 0;
	UINT32 egcSize = 0;
	char *naluDataTmp = NULL;
	bool flag = 0;
	UINT32 flagCount = 0;
	UINT32 flagFilePtrPos = 0;
	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. 获取Nalu数据暂存到 char *naluDataTmp中，长度存到naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw;

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
			flagFilePtrPos = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - naluLen;
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
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< 二进制指针位置归零
			m_lastByte = 0;	///< 字节归零

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				// 3. 若为要找类型，则判断naluData是否为空:
				if (outNaluData) ///< 非空：则 *naluData = naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - flagFilePtrPos;	///< 要拷贝的长度
						UINT32 filePtrPos = m_fileStream.tellg();
						m_fileStream.seekg(flagFilePtrPos);
						*outNaluData = new char[naluLen];
						UINT32 outLengthRead = 0;
						if (Read(*outNaluData, &naluLen, &outLengthRead) == Failed)
							throw ;
						m_fileStream.seekg(filePtrPos);
						flag = false;
						flagCount = 0;
						delete naluDataTmp;
						naluDataTmp = NULL;
					}
					else
						*outNaluData = naluDataTmp;
				}
				else	///< 为空：则 delete naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - flagFilePtrPos;
						flag = false;
						flagCount = 0;
					}
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. 返回长度
				if (outLength)
					*outLength = naluLen;

				return Success;
			}
			else
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
		}
		else
		{
			delete naluDataTmp;
			naluDataTmp = NULL;
		}
		
	}

	return Failed;
}


//************************************
// 函数名:	H264Analysis::Read
// 描述:	读取接下来的ioReadLength个字节，若成功读取的字节数(*outLengthRead)不足(*inBufferLength)个，则会返回Failed，其他情况返回Success
// 返回值:	StatusCode 状态值( Failed => 0, Success => 1 )
// 参数: 	char * outBuffer(out: 传出读取的字节数据，需要调用者分配和释放内存)
// 参数: 	UINT32 *inBufferLength(in: 传入要读取数据的字节长度)
// 参数: 	UINT32 *outLengthRead(out: 返回成功读取数据的字节长度)
// 注意:	一般情况下，*inBufferLength == *outLengthRead，只有在快读到文件结尾时，两者的值才会不同 *inBufferLength > *outLengthRead
// 日期: 	2016/05/18
// 作者: 	YJZ
// 修改记录:
//************************************
inline H264Analysis::StatusCode H264Analysis::Read( char *outBuffer, UINT32 *inBufferLength, UINT32 *outLengthRead)
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	UINT32 filePtrPos = m_fileStream.tellg();
	int c = m_fileStream.tellg();

	if (filePtrPos + *inBufferLength > m_fileLen)
	{
		*outLengthRead = m_fileLen - filePtrPos;
		m_fileStream.read(outBuffer, *outLengthRead);
		return Failed;
	}
	else
	{
		*outLengthRead = *inBufferLength;
		m_fileStream.read(outBuffer, *outLengthRead);
		c = m_fileStream.tellg();
	}
	
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "Read " << time_diff << endl;
#endif

	return Success;
}


//************************************
// 函数名:	H264Analysis::GetNaluCount
// 描述:	获取NALU总数
// 返回值:	UINT32
// 日期: 	2016/06/15
// 作者: 	YJZ
// 修改记录:
//************************************
UINT32 H264Analysis::NaluCount()
{
	UINT32 naluCount = 0;
	UINT32 startCodeLen = 0;
	while (true)
	{
		if (m_pStreamBuf->p >= m_pStreamBuf->end)
		{
			if (CheckStreamBuf() == Failed)
				break;
		}

		if (*m_pStreamBuf->p == 0x00)
			startCodeLen++;
		else if (*m_pStreamBuf->p == 0x01 && (startCodeLen == 2 || startCodeLen == 3))
		{
			startCodeLen = 0;
			naluCount++;
		}
		else
			startCodeLen = 0;
		m_pStreamBuf->p++;
	}
	ClearStreamBuf();
	m_fileStream.seekg(0);	// 还原文件指针位置
	return naluCount;
}
