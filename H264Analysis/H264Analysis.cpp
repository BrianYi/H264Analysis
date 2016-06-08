#include "H264Analysis.h"

H264Analysis::H264Analysis(void)
{
	/// ��ʼ�����ṹ
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
// ������:	H264Analysis::getOpenFile
// ����:	��ȡ�ļ���
// ����ֵ:	ifstream& 
// ����: 	const string & fileName
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
//************************************
std::ifstream& H264Analysis::getOpenFile( const std::string &fileName )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/// ��ʼ�����ṹ
	if (m_fileStream.is_open())
		m_fileStream.close();
	m_len = 0;
	m_lastByte = 0;
	m_binPos = 0;

	/// �ļ������ݴ�ŵ����ṹ��
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
// ������:	H264Analysis::closeFile
// ����:	�ر��ļ�
// ����ֵ:	void
// ����: 	2016/05/19
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::nextNalu
// ����:	��ȡ��һ��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/16
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::nextNalu(char **naluData)
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. �ҵ���ǰstartCode����ȡ�ļ�λ��
	 * 2. ����ͷ��
	 * 3. �ҵ��¸�startCode����ȡ�ļ�λ��
	 * 4. ��ȡNalu����naluLen(�¸�startCodeλ�� - ��ǰstartCodeλ��)
	 * 5. �ļ�ָ���ض�λ����ǰstartCode
	 * 6. ��ȡnaluLen���ֽڱ��浽����naluData�д���
	 * 7. �ļ�ָ�뻹ԭ��ԭ��λ��
	 * 8. ����naluLen
	 */

	char c;
	int startCodeLen = 0;

	// 1.�ҵ���ǰstartCode����ȡ�ļ�λ��
	// 2.����ͷ��
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

	// 3. �ҵ��¸�startCode����ȡ�ļ�λ��
	startCodeLen = 0;
	while (true)
	{
		if (checkStreamBuf() == Failed) ///< �ѵ��ļ���β
			break;

		c = m_pStreamBuf->buf[m_pStreamBuf->pos];
		m_pStreamBuf->pos++;

		if (c == 0x00)
		{
			startCodeLen++;
			/**							
			 * ���⣺							  -----------
			 *			���� m_pStreamBuf->buf =>|xx .. xx 00|  
			 *									  -----------
			 *									  -----------
			 *			  �� m_pStreamBuf->buf =>|00 01 xx ..|
			 *									  -----------
			 *										 ^
			 *										 | (���λ�ò��ҵ�startCode)
			 *	
			 * ����ʽ���ļ�ָ��ָ���������00֮ǰ��Ȼ����·���m_pStreamBuf->buf
			 *	
			 *								   --------------
			 *	 ������ m_pStreamBuf->buf =>|00 00 01 .. xx|
			 *								   --------------
			 *								   ^
			 *								   | (m_pStreamBuf->pos = 0)
			 */
			if (m_pStreamBuf->pos >= m_pStreamBuf->len)	// ������βΪstartCode���
			{
				m_fileStream.seekg(m_pStreamBuf->tellgBase + m_pStreamBuf->pos - startCodeLen); // �ļ�ָ����˵����������0x00֮ǰ
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
	m_pStreamBuf->pos -= startCodeLen;	///< ����ָ�룬ָ��Nalu��ͷ

	// 4. ��ȡNalu����naluLen(�¸�startCodeλ�� - ��ǰstartCodeλ��)
	size_t naluLen = naluNextPos - naluPos;

	// 5. �ļ�ָ���ض�λ����ǰstartCode
	// 6. ��ȡnaluLen���ֽڱ��浽����naluData�д���
	// 7. �ļ�ָ�뻹ԭ��ԭ��λ��
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

	// 8. ����naluLen
	return naluLen;
}


//************************************
// ������:	H264Analysis::next_SPS_Nalu
// ����:	��ȡ��һ������SPS��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::next_SPS_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete [] naluDataTmp
	 *		�ǿգ��� *naluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete [] naluDataTmp
	 * 5. ���س���
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	char *naluDataTmp = NULL;

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SPS)
		{
			// 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
			if (naluData) ///< �ǿգ��� *naluData = naluDataTmp
				*naluData = naluDataTmp;
			else	///< Ϊ�գ��� delete [] naluDataTmp
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
			// 4. ���س���
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
// ������:	H264Analysis::next_PPS_Nalu
// ����:	��ȡ��һ������PPS��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::next_PPS_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete [] naluDataTmp
	 *		�ǿգ��� *naluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete [] naluDataTmp
	 * 5. ���س���
	 */

	size_t naluLen = 0;
	int startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	char *naluDataTmp = NULL;

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_PPS)
		{
			// 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
			if (naluData) ///< �ǿգ��� *naluData = naluDataTmp
				*naluData = naluDataTmp;
			else	///< Ϊ�գ��� delete [] naluDataTmp
			{
				delete [] naluDataTmp;
				naluDataTmp = NULL;
			}

#ifdef TIME_TEST
			DWORD time_diff = GetTickCount() - time_beg;
			m_debugFileStream << "next_PPS_Nalu " << time_diff << endl;
#endif
			// 4. ���س���
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
// ������:	H264Analysis::next_IDR_Nalu
// ����:	��ȡ��һ������IDR֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/30
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::next_IDR_Nalu( char **naluData /*= NULL*/ )
{
	#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete [] naluDataTmp
	 *		�ǿգ��� *naluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete [] naluDataTmp
	 * 5. ���س���
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
	while (naluLen = nextNalu(&naluDataTmp))	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		startCodeLen = scLen(naluDataTmp);

		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;

		// ��SPSλ�ü�¼�����󷽵�2�����3��ΪI֡����ѴӸ�SPSλ�ÿ�ʼ��I֡���������ݷ���naluData�д���
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
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				// 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
				if (naluData) ///< �ǿգ��� *naluData = naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - flagFilePtrPos;	///< Ҫ�����ĳ���
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
				else	///< Ϊ�գ��� delete [] naluDataTmp
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
				// 4. ���س���
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
// ������:	H264Analysis::next_I_Nalu
// ����:	����:	��ȡ��һ������I֡��Nalu(��ǰ����SPS,PPS��SEI��Ὣ������I֡һ����), �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	unsigned int speed(in: Ĭ��Ϊ1�����������ٶ�����һ��I֡��speedΪ2ʱ����ÿ2��Iֻ֡����һ��I֡��������������)
// ����: 	2016/06/02
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::next_P_Nalu
// ����:	��ȡ��һ������P֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::next_P_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete [] naluDataTmp
	 *		�ǿգ��� *naluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete [] naluDataTmp
	 * 5. ���س���
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

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���


			if (slice_type == SLICE_TYPE_P1 || slice_type == SLICE_TYPE_P2)
			{
				// 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
				if (naluData) ///< �ǿգ��� *naluData = naluDataTmp
					*naluData = naluDataTmp;
				else	///< Ϊ�գ��� delete [] naluDataTmp
				{
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. ���س���
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
// ������:	H264Analysis::next_B_Nalu
// ����:	��ȡ��һ������B֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::next_B_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
			/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete [] naluDataTmp
	 *		�ǿգ��� *naluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete [] naluDataTmp
	 * 5. ���س���
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

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���


			if (slice_type == SLICE_TYPE_B1 || slice_type == SLICE_TYPE_B2)
			{
				// 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
				if (naluData) ///< �ǿգ��� *naluData = naluDataTmp
					*naluData = naluDataTmp;
				else	///< Ϊ�գ��� delete [] naluDataTmp
				{
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. ���س���
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
// ������:	H264Analysis::next_SI_Nalu
// ����:	��ȡ��һ������SI֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::next_SI_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
			/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete [] naluDataTmp
	 *		�ǿգ��� *naluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete [] naluDataTmp
	 * 5. ���س���
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

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���


			if (slice_type == SLICE_TYPE_SI1 || slice_type == SLICE_TYPE_SI2)
			{
				// 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
				if (naluData) ///< �ǿգ��� *naluData = naluDataTmp
					*naluData = naluDataTmp;
				else	///< Ϊ�գ��� delete [] naluDataTmp
				{
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. ���س���
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
// ������:	H264Analysis::next_SP_Nalu
// ����:	��ȡ��һ������SP֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::next_SP_Nalu( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
			/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete [] naluDataTmp
	 *		�ǿգ��� *naluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete [] naluDataTmp
	 * 5. ���س���
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

	while (naluLen = nextNalu(&naluDataTmp))	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		startCodeLen = scLen(naluDataTmp);
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���


			if (slice_type == SLICE_TYPE_SP1 || slice_type == SLICE_TYPE_SP2)
			{
				// 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
				if (naluData) ///< �ǿգ��� *naluData = naluDataTmp
					*naluData = naluDataTmp;
				else	///< Ϊ�գ��� delete [] naluDataTmp
				{
					delete [] naluDataTmp;
					naluDataTmp = NULL;
				}
				// 4. ���س���
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
// ������:	H264Analysis::skipTo
// ����:	��ת��ָ��λ��(0~100)
// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
// ����: 	short int persent(in: 0~100, ����Ҫ��ת�İٷֱ�)
// ����: 	2016/05/23
// ����: 	YJZ
// �޸ļ�¼:
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
	
	// ��ջ���
	if (m_pStreamBuf->buf)
	{
		delete [] m_pStreamBuf->buf;
		m_pStreamBuf->buf = NULL;
		m_pStreamBuf->len = 0;
		m_pStreamBuf->pos = 0;
		m_pStreamBuf->tellgBase = 0;
	}

	// �ҵ���һ��I֡������������ָ�붨λ����I֡��ͷ��ǰ��������SPS,PPS����ָ��SPSǰ��
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
// ������:	H264Analysis::getNaluType
// ����:	��ȡNalu������
// ����ֵ:	NalUnitType => Nalu����
// ����: 	char * naluData(in: ����Nalu����)
// ����: 	2016/05/27
// ����: 	YJZ
// �޸ļ�¼:
//************************************
NalUnitType H264Analysis::getNaluType( char *naluData )
{
	return (NalUnitType)B8_VAL_BASE_R(naluData[scLen(naluData)], 3, 5);
}

//************************************
// ������:	H264Analysis::getSliceType
// ����:	��ȡ֡����
// ����ֵ:	SliceType => ֡����
// ����: 	char * naluData(in: ����Nalu����)
// ����: 	size_t naluLen(in: ����Nalu����)
// ����: 	2016/05/27
// ����: 	YJZ
// �޸ļ�¼:
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
		// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
		if (ueDecode(&naluData[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
			ueDecode(&naluData[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
		{
			throw std::exception();
		}
		m_binPos = 0;	///< ������ָ��λ�ù���
		m_lastByte = 0;	///< �ֽڹ���

		return (SliceType)slice_type;
	}
	else
		return SLICE_TYPE_NONE;
}

//************************************
// ������:	H264Analysis::scLen
// ����:	����startCode����
// ����ֵ:	size_t => startCode����
// ����: 	char * p
// ����: 	2016/05/25
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::scLen(char *p)
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	size_t len = 0;
	// ����ͷ��
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
// ������:	H264Analysis::ueDecode
// ����:	����Exp-Golomb-Code(ָ�����ײ�����)
// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *egcData(in: ����Ҫ�����Exp-Golomb-Code����)
// ����: 	size_t len(in: ����Exp-Golomb-Code���ݵĳ���)
// ����: 	UINT32 * codeNum(out: ����������ֵ)
// ����:	unsigned int * egcSize(out: ���������õ��ֽ���)
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
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
		m_binPos++;	// ָ������
		if (m_binPos % 8 == 0)
			m_binPos = 0;
	}

	if (leadingZeroBits > 32)
	{
		throw std::exception();
		return -1;
	}

	// ָ�뻹��ʣ��λ��δ��
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
			if (leadingZeroBits <= leftBits)	// ʣ��λ���㹻������ײ�����
			{
				readBits = B8_VAL_BASE_R(m_lastByte, m_binPos, leadingZeroBits);
			}
			else	// ʣ��λ�ò���������ײ�����
			{
				int leftLeadingZeroBits = leadingZeroBits - leftBits;
				int leftLeadingZeroBytes = bits_get_byte_num(leftLeadingZeroBits);
				char *p = new char[leftLeadingZeroBytes];
			
				if (egcPtrPos + leftLeadingZeroBytes >= len)
					return Failed;

				/**
				 * ԭ����ֽ�ʣ��λ��nλ����������ײ����룬����ȡ������������ֽ�(�����������Ҫ��������ֽڲſɽ����EGC),
				 * �Ƚ������ֽڷ���UINT32���͵�golomb�У����ֽڵ��򣬽�����ȡ��ֻ��ȡҪ����EGC����Ҫ������λ(λ����),
				 * Ȼ��readBits������nλ����ԭ����ֽ�ʣ��λ�õ�nλ�������(λ����)�����յó�readbits�Ķ�����ֵ
				 */
				memcpy(p, &pEgcData[egcPtrPos], leftLeadingZeroBytes);
				egcPtrPos += leftLeadingZeroBytes;
				memcpy((void*)&readBits, p, leftLeadingZeroBytes);
				bytes_reverse((char*)&readBits, sizeof(readBits));

				// ��ȡ������Exp-Golomb-Code
				readBits &= B32_VAL_MASK(0, leftLeadingZeroBits);
				readBits = (readBits>>leftBits)|(B32_VAL_BASE_L((UINT32)B8_VAL_BASE_L(m_lastByte, m_binPos), 24));
				readBits = B32_VAL_BASE_R(readBits, 0, leadingZeroBits);

				// �������ֽ�
				m_lastByte = p[leftLeadingZeroBytes-1];
				delete [] p;
			}
			m_binPos += leadingZeroBits;
			m_binPos %= 8;
			*codeNum = pow(2.0, leadingZeroBits) - 1 + readBits;
		}
	}
	else	// ָ��û��ʣ��λ��δ��
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

			// ��ȡ������Exp-Golomb-Code
			readBits &= B32_VAL_MASK(0, leadingZeroBits);
			readBits = B32_VAL_BASE_R(readBits, 0, leadingZeroBits);

			// �������ֽ�
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

	*egcSize = egcPtrPos; ///< ���ؽ���EGCʱ��ȡ���ֽ���
	return Success;
}

//************************************
// ������:	H264Analysis::next_I_Nalu
// ����:	��ȡ��һ������I֡��Nalu(��ǰ����SPS,PPS��SEI��Ὣ������I֡һ����), �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::nextInalu( char **naluData /*= NULL*/ )
{
	#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete [] naluDataTmp
	 *		�ǿգ��� *naluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete [] naluDataTmp
	 * 5. ���س���
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
	while (naluLen = nextNalu(&naluDataTmp))	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		startCodeLen = scLen(naluDataTmp);

		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;

		// ��SPSλ�ü�¼�����󷽵�2�����3��ΪI֡����ѴӸ�SPSλ�ÿ�ʼ��I֡���������ݷ���naluData�д���
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
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ueDecode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ueDecode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				// 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
				if (naluData) ///< �ǿգ��� *naluData = naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->pos - flagFilePtrPos;	///< Ҫ�����ĳ���
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
				else	///< Ϊ�գ��� delete [] naluDataTmp
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
				// 4. ���س���
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
// ������:	H264Analysis::readNextBytes
// ����:	��ȡ��������len���ֽ�
// ����ֵ:	size_t => �ɹ���ȡ���ֽ���
// ����: 	char * p(out: ������ȡ���ֽ����ݣ���Ҫ�����߷�����ͷ��ڴ�)
// ����: 	int len(in: ��ȡ�ֽڵĳ���)
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::checkStreamBuf
// ����:	��黺�����������Ƿ񻹹������������ٴζ�ȡBUFSIZE�ֽڵ��ļ�����
// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
// ����: 	2016/05/24
// ����: 	YJZ
// �޸ļ�¼:
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
		if (m_pStreamBuf->tellgBase >= m_len)	///< �Ƿ񵽴��ļ���β
			return Failed;

		m_pStreamBuf->buf = new char[BUFSIZE];
		m_pStreamBuf->len = readNextBytes(m_pStreamBuf->buf, BUFSIZE);
		m_pStreamBuf->pos = 0;
	}
	return Success;
}