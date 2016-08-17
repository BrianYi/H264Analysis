#include "H264Analysis.h"
#include "H264PublicDef.h"
#include <math.h>

H264Analysis::H264Analysis(void)
{
	/// ��ʼ�����ṹ
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
// ������:	H264Analysis::GetOpenFile
// ����:	��ȡ�ļ���
// ����ֵ:	ifstream& 
// ����: 	const char* fileName
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
//************************************
std::ifstream& H264Analysis::GetOpenFile( const char* fileName )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/// ��ʼ�����ṹ
	m_fileLen = 0;
	m_lastByte = 0;
	m_binPos = 0;

	/// �ļ������ݴ�ŵ����ṹ��
	UINT32 fileLen = strlen(fileName);
	m_pFilePath = new char[fileLen+1];
	memset(m_pFilePath, 0, fileLen+1);
	strcpy(m_pFilePath, fileName);
	if (m_fileStream.is_open())
	{
		// �ر��Ѿ��򿪵��ļ�
		m_fileStream.close();

		// ��ջ�����
		ClearStreamBuf();
	}
	m_fileStream.open(m_pFilePath, std::ios::binary | std::ios::in);
	if (m_fileStream == NULL)
		return m_fileStream;
	m_fileStream.seekg(0, std::ios::end);
	m_fileLen = m_fileStream.tellg();
	m_fileStream.seekg(0);

	/// ����������
	ClearStreamBuf();
	m_pStreamBuf->beg = new char[BUFSIZE];
	m_pStreamBuf->len = BUFSIZE;
	UINT32 outLengthRead = 0;
	if (Read(m_pStreamBuf->beg, &m_pStreamBuf->len, &outLengthRead) == Failed)
		m_pStreamBuf->len = outLengthRead;	// ��ȡ���ݲ���ʱ�Ĵ���
	m_pStreamBuf->end = m_pStreamBuf->beg + m_pStreamBuf->len;
	m_pStreamBuf->p = m_pStreamBuf->beg;
	
	/// ����: 1.�ļ���Nalu���� 2.�ļ���ʱ��
	m_naluCount = NaluCount();	// �����ʱ�ܶ�,�����������ʧ��
	m_frameTotalTime = (float)m_naluCount / m_frameRate;


	/// ����������
	ClearStreamBuf();
	m_pStreamBuf->beg = new char[BUFSIZE];
	m_pStreamBuf->len = BUFSIZE;
	outLengthRead = 0;
	if (Read(m_pStreamBuf->beg, &m_pStreamBuf->len, &outLengthRead) == Failed)
		m_pStreamBuf->len = outLengthRead;	// ��ȡ���ݲ���ʱ�Ĵ���
	m_pStreamBuf->end = m_pStreamBuf->beg + m_pStreamBuf->len;
	m_pStreamBuf->p = m_pStreamBuf->beg;
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "GetOpenFile " << time_diff << endl;
#endif
	return m_fileStream;
}


//************************************
// ������:	H264Analysis::CloseFile
// ����:	�ر��ļ�
// ����ֵ:	void
// ����: 	2016/05/19
// ����: 	YJZ
// �޸ļ�¼:
//************************************
void H264Analysis::CloseFile()
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	if (m_fileStream.is_open())
	{
		m_fileStream.close();

		// ��ջ�����
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
// ������:	H264Analysis::NextNalu
// ����:	��ȡ��һ��Nalu, �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	2016/05/16
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::NextNalu(char **outNaluData, UINT32 *outLength)
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
	 * 6. ��ȡnaluLen���ֽڱ��浽����outNaluData�д���
	 * 7. �ļ�ָ�뻹ԭ��ԭ��λ��
	 * 8. ����naluLen
	 */

	//char c;
	UINT32 startCodeLen = 0;

	// 1.�ҵ���ǰstartCode����ȡ�ļ�λ��
	// 2.����ͷ��
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

	// 3. �ҵ��¸�startCode����ȡ�ļ�λ��
	startCodeLen = 0;
	while (true)
	{
		if (m_pStreamBuf->p >= m_pStreamBuf->end)
		{
			if (CheckStreamBuf() == Failed) ///< �ѵ��ļ���β
				break;
		}

		if (*m_pStreamBuf->p == 0x00)
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
			if (m_pStreamBuf->p >= m_pStreamBuf->end)	// ������βΪstartCode���
			{
				m_fileStream.seekg(m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - startCodeLen); // �ļ�ָ����˵����������0x00֮ǰ
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
	m_pStreamBuf->p -= startCodeLen;	///< ����ָ�룬ָ��Nalu��ͷ

	// 4. ��ȡNalu����naluLen(�¸�startCodeλ�� - ��ǰstartCodeλ��)
	UINT32 naluLen = naluNextPos - naluPos;

	// 5. �ļ�ָ���ض�λ����ǰstartCode
	// 6. ��ȡnaluLen���ֽڱ��浽����outNaluData�д���
	// 7. �ļ�ָ�뻹ԭ��ԭ��λ��
	if (outNaluData)
	{
		*outNaluData = new char[naluLen];
		int filePtrPos = m_fileStream.tellg();
		UINT32 outLengthRead = 0;
		m_fileStream.seekg(naluPos);
		if (Read(*outNaluData, &naluLen, &outLengthRead) == Failed)	throw ;
		m_fileStream.seekg(filePtrPos);
	}

	// 8. ����naluLen
	if (outLength)
		*outLength = naluLen;

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "NextNalu " << time_diff << endl;
#endif

	
	return Success;
}


//************************************
// ������:	H264Analysis::NextSpsNalu
// ����:	��ȡ��һ������SPS��Nalu, �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::NextSpsNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete naluDataTmp
	 *		�ǿգ��� *outNaluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete naluDataTmp
	 * 5. ���س���
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	char *naluDataTmp = NULL;

	while (NextNalu(&naluDataTmp,&naluLen) == Success)	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		if (GetStartCodeLength(naluDataTmp, &startCodeLen) == Failed)	throw ;
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SPS)
		{
			// 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
			if (outNaluData) ///< �ǿգ��� *outNaluData = naluDataTmp
				*outNaluData = naluDataTmp;
			else	///< Ϊ�գ��� delete naluDataTmp
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}

			// 4. ���س���
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
// ������:	H264Analysis::NextPpsNalu
// ����:	��ȡ��һ������PPS��Nalu, �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::NextPpsNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete [] naluDataTmp
	 *		�ǿգ��� *outNaluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete [] naluDataTmp
	 * 5. ���س���
	 */

	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete naluDataTmp
	 *		�ǿգ��� *outNaluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete naluDataTmp
	 * 5. ���س���
	 */

	UINT32 naluLen = 0;
	UINT32 startCodeLen = 0;
	unsigned char nextByte = 0;
	NalUnitType nal_unit_type = NAL_FF_IGNORE;
	char *naluDataTmp = NULL;

	while (NextNalu(&naluDataTmp,&naluLen) == Success)	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_PPS)
		{
			// 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
			if (outNaluData) ///< �ǿգ��� *outNaluData = naluDataTmp
				*outNaluData = naluDataTmp;
			else	///< Ϊ�գ��� delete naluDataTmp
			{
				delete naluDataTmp;
				naluDataTmp = NULL;
			}
			// 4. ���س���
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
// ������:	H264Analysis::NextIdrNalu
// ����:	��ȡ��һ������IDR֡��Nalu, �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	2016/05/30
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::NextIdrNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete naluDataTmp
	 *		�ǿգ��� *outNaluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete naluDataTmp
	 * 5. ���س���
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
	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)
			throw ;

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
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				// 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
				if (outNaluData) ///< �ǿգ��� *outNaluData = naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - flagFilePtrPos;	///< Ҫ�����ĳ���
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
				else	///< Ϊ�գ��� delete naluDataTmp
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

				// 4. ���س���
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
// ������:	H264Analysis::NextInalu
// ����:	����:	��ȡ��һ������I֡��Nalu(��ǰ����SPS,PPS��SEI��Ὣ������I֡һ����), �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	UINT16 inSpeed(in: Ĭ��Ϊ1�����������ٶ�����һ��I֡��inSpeedΪ2ʱ����ÿ2��Iֻ֡����һ��I֡��������������)
// ����: 	2016/06/02
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::NextPnalu
// ����:	��ȡ��һ������P֡��Nalu, �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::NextPnalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete naluDataTmp
	 *		�ǿգ��� *outNaluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete naluDataTmp
	 * 5. ���س���
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

	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���


			if (slice_type == SLICE_TYPE_P1 || slice_type == SLICE_TYPE_P2)
			{
				// 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
				if (outNaluData) ///< �ǿգ��� *outNaluData = naluDataTmp
					*outNaluData = naluDataTmp;
				else	///< Ϊ�գ��� delete naluDataTmp
				{
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. ���س���
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
// ������:	H264Analysis::NextBnalu
// ����:	��ȡ��һ������B֡��Nalu, �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::NextBnalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete naluDataTmp
	 *		�ǿգ��� *outNaluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete naluDataTmp
	 * 5. ���س���
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

	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���


			if (slice_type == SLICE_TYPE_B1 || slice_type == SLICE_TYPE_B2)
			{
				// 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
				if (outNaluData) ///< �ǿգ��� *outNaluData = naluDataTmp
					*outNaluData = naluDataTmp;
				else	///< Ϊ�գ��� delete naluDataTmp
				{
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. ���س���
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
// ������:	H264Analysis::NextSiNalu
// ����:	��ȡ��һ������SI֡��Nalu, �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::NextSiNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete naluDataTmp
	 *		�ǿգ��� *outNaluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete naluDataTmp
	 * 5. ���س���
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

	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���


			if (slice_type == SLICE_TYPE_SI1 || slice_type == SLICE_TYPE_SI2)
			{
				// 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
				if (outNaluData) ///< �ǿգ��� *outNaluData = naluDataTmp
					*outNaluData = naluDataTmp;
				else	///< Ϊ�գ��� delete naluDataTmp
				{
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. ���س���
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
// ������:	H264Analysis::NextSpNalu
// ����:	��ȡ��һ������SP֡��Nalu, �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::NextSpNalu( char **outNaluData, UINT32 *outLength )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete naluDataTmp
	 *		�ǿգ��� *outNaluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete naluDataTmp
	 * 5. ���س���
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

	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw ;
		
		// 2. ��ȡ��һ�ֽڣ��ж�Nalu����
		nextByte = naluDataTmp[startCodeLen];	
		nal_unit_type = (NalUnitType)B8_VAL_BASE_R(nextByte, 3, 5);
		egcDataPos = startCodeLen + 1;
		egcDataLen = naluLen - egcDataPos;
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���


			if (slice_type == SLICE_TYPE_SP1 || slice_type == SLICE_TYPE_SP2)
			{
				// 3. ��ΪҪ�����ͣ����ж�outNaluData�Ƿ�Ϊ��:
				if (outNaluData) ///< �ǿգ��� *outNaluData = naluDataTmp
					*outNaluData = naluDataTmp;
				else	///< Ϊ�գ��� delete naluDataTmp
				{
					delete naluDataTmp;
					naluDataTmp = NULL;
				}

				// 4. ���س���
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
// ������:	H264Analysis::SeekDstToPos
// ����:	��ת��ָ��λ��(�ٷֱȱ�ʾ0.0~1.0)
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����: 	float inPersent(in: 0.0~1.0, ����Ҫ��ת�İٷֱ�)
// ����: 	2016/05/23
// ����: 	YJZ
// �޸ļ�¼:
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

	// ��ջ���
	ClearStreamBuf();

	// �����ڴ�
	m_pStreamBuf->tellgBase = m_fileStream.tellg();
	m_pStreamBuf->beg = new char[BUFSIZE];
	m_pStreamBuf->len = BUFSIZE;
	UINT32 outLengthRead = 0;
	if (Read(m_pStreamBuf->beg, &m_pStreamBuf->len, &outLengthRead) == Failed)
		m_pStreamBuf->len = outLengthRead;	///< ���һ�ζ�ȡ���ݲ���ʱ�����˳�
	m_pStreamBuf->end = m_pStreamBuf->beg + m_pStreamBuf->len;
	m_pStreamBuf->p = m_pStreamBuf->beg;

	// �ҵ���һ��I֡������������ָ�붨λ����I֡��ͷ��ǰ��������SPS,PPS����ָ��SPSǰ��
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
// ������:	H264Analysis::GetNaluType
// ����:	��ȡNalu������
// ����ֵ:	NalUnitType => Nalu����
// ����: 	char * inNaluData(in: ����Nalu����)
// ����: 	2016/05/27
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::NalUnitType H264Analysis::GetNaluType( char *inNaluData )
{
	UINT32 startCodeLength = 0;
	if (GetStartCodeLength(inNaluData,&startCodeLength) == Failed)	throw;
	return (NalUnitType)B8_VAL_BASE_R(inNaluData[startCodeLength], 3, 5);
}

//************************************
// ������:	H264Analysis::GetSliceType
// ����:	��ȡ֡����
// ����ֵ:	SliceType => ֡����
// ����: 	char * inNaluData(in: ����Nalu����)
// ����: 	UINT32 inLength(in: ����Nalu����)
// ����: 	2016/05/27
// ����: 	YJZ
// �޸ļ�¼:
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
		// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
		if (ParseUeExpGolombCode(&inNaluData[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
			ParseUeExpGolombCode(&inNaluData[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
			throw ;

		m_binPos = 0;	///< ������ָ��λ�ù���
		m_lastByte = 0;	///< �ֽڹ���

		return (SliceType)slice_type;
	}
	else
		return SLICE_TYPE_NONE;
}

//************************************
// ������:	H264Analysis::GetStartCodeLength
// ����:	����startCode����
// ����ֵ:	size_t => startCode����
// ����: 	char * inNaluData
// ����: 	UINT32 outLength(out: ����Nalu����)
// ����: 	2016/05/25
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::GetStartCodeLength(char *inNaluData, UINT32 *outLength)
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	UINT32 naluLength = 0;
	// ����ͷ��
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
// ������:	H264Analysis::ParseUeExpGolombCode
// ����:	����Exp-Golomb-Code(ָ�����ײ�����)
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *inUeExpGolombCodeData(in: ����Ҫ�����Exp-Golomb-Code����)
// ����: 	UINT32 inLength(in: ����Exp-Golomb-Code���ݵĳ���)
// ����: 	UINT32 * outResult(out: ����������ֵ)
// ����:	UINT32 * outResultLength(out: ���������õ��ֽ���)
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
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
		m_binPos++;	// ָ������
		if (m_binPos % 8 == 0)
			m_binPos = 0;
	}

	if (leadingZeroBits > 32)
	{
		throw ;
		return Failed;
	}

	// ָ�뻹��ʣ��λ��δ��
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
			if (leadingZeroBits <= leftBits)	// ʣ��λ���㹻������ײ�����
			{
				readBits = B8_VAL_BASE_R(m_lastByte, m_binPos, leadingZeroBits);
			}
			else	// ʣ��λ�ò���������ײ�����
			{
				INT32 leftLeadingZeroBits = leadingZeroBits - leftBits;
				INT32 leftLeadingZeroBytes = bits_get_byte_num(leftLeadingZeroBits);
				char *p = new char[leftLeadingZeroBytes];
			
				if (egcPtrPos + leftLeadingZeroBytes >= inLength)
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
				delete p;
			}
			m_binPos += leadingZeroBits;
			m_binPos %= 8;
			*outResult = pow(2.0, leadingZeroBits) - 1 + readBits;
		}
	}
	else	// ָ��û��ʣ��λ��δ��
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

			// ��ȡ������Exp-Golomb-Code
			readBits &= B32_VAL_MASK(0, leadingZeroBits);
			readBits = B32_VAL_BASE_R(readBits, 0, leadingZeroBits);

			// �������ֽ�
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

	*outResultLength = egcPtrPos; ///< ���ؽ���EGCʱ��ȡ���ֽ���
	return Success;
}


//************************************
// ������:	H264Analysis::CheckStreamBuf
// ����:	��黺�����������Ƿ񻹹������������ٴζ�ȡBUFSIZE�ֽڵ��ļ�����
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����: 	2016/05/24
// ����: 	YJZ
// �޸ļ�¼:
//************************************
inline H264Analysis::StatusCode H264Analysis::CheckStreamBuf()
{
	if ((m_pStreamBuf->p >= m_pStreamBuf->end) && (m_pStreamBuf->end != NULL))
	{
		ClearStreamBuf();

		m_pStreamBuf->tellgBase = m_fileStream.tellg();
		if (m_pStreamBuf->tellgBase >= m_fileLen)	///< �Ƿ񵽴��ļ���β
			return Failed;

		m_pStreamBuf->beg = new char[BUFSIZE];
		m_pStreamBuf->len = BUFSIZE;
		UINT32 outLengthRead = 0;
		if (Read(m_pStreamBuf->beg, &m_pStreamBuf->len, &outLengthRead) == Failed)
			m_pStreamBuf->len = outLengthRead;	///< ���һ�ζ�ȡ���ݲ���ʱ�����˳�
		m_pStreamBuf->end = m_pStreamBuf->beg + m_pStreamBuf->len;
		m_pStreamBuf->p = m_pStreamBuf->beg;
		return Success;
	}
	return Failed;
}


//************************************
// ������:	H264Analysis::ClearStreamBuf
// ����:	��ջ�����
// ����ֵ:	void
// ����: 	2016/06/15
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::SeekStreamBufPtrPos
// ����:		��������ָ���ƶ���ָ��λ��
// ����ֵ:	H264Analysis::StatusCode
// ����: 	UINT32 inNewPos (in:�����µ�λ�ã������ʼλ��)
// ����: 	2016/08/16
// ����: 	YJZ
// �޸ļ�¼:	
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
// ������:	H264Analysis::SeekStreamBufPtrPos
// ����:		��������ָ���ƶ���ָ��λ��
// ����ֵ:	H264Analysis::StatusCode
// ����: 	INT32 inOffset (in:���ƫ����)
// ����: 	StreamBufBase inWay (in:���ƫ������)
// ����: 	2016/08/16
// ����: 	YJZ
// �޸ļ�¼:	
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
// ������:	H264Analysis::Next_I_Nalu
// ����:	��ȡ��һ������I֡��Nalu(��ǰ����SPS,PPS��SEI��Ὣ������I֡һ����), �������ݷ������outNaluData��������ɺ󻺳���ָ��(m_pStreamBuf->p)ָ����һ��Nalu�Ŀ�ͷ
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����:	char *outNaluData	
//			1. Not NULL => ����Nalu, ����startCode
//			2. NULL		=> ����ȡ����
// ����:	UINT32 *outLength
//			1. Not NULL => ����Nalu�ĳ���
//			2. NULL		=> ����ȡ����
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
H264Analysis::StatusCode H264Analysis::Next_I_Nalu( char **outNaluData, UINT32 *outLength )
{
	#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	/**
	 * 1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	 * 2. ��ȡ��һ�ֽڣ��ж�Nalu����
	 * 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
	 *		Ϊ�գ��� delete naluDataTmp
	 *		�ǿգ��� *naluData = naluDataTmp
	 * 4. ����ΪҪ�ҵ����ͣ��� delete naluDataTmp
	 * 5. ���س���
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
	while (NextNalu(&naluDataTmp, &naluLen) == Success)	///<  1. ��ȡNalu�����ݴ浽 char *naluDataTmp�У����ȴ浽naluLen
	{
		if (GetStartCodeLength(naluDataTmp,&startCodeLen) == Failed)	throw;

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
			// EGC�����һ�λ��first_mb_in_slice, EGC����ڶ��λ��slice_type
			if (ParseUeExpGolombCode(&naluDataTmp[egcDataPos], egcDataLen, &first_mb_in_slice, &egcSize) == Failed || 
				ParseUeExpGolombCode(&naluDataTmp[egcDataPos + egcSize], egcDataLen, (UINT32 *)&slice_type, &egcSize) == Failed)
				return Failed;
			m_binPos = 0;	///< ������ָ��λ�ù���
			m_lastByte = 0;	///< �ֽڹ���

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				// 3. ��ΪҪ�����ͣ����ж�naluData�Ƿ�Ϊ��:
				if (outNaluData) ///< �ǿգ��� *naluData = naluDataTmp
				{
					if (flag && flagCount)
					{
						naluLen = m_pStreamBuf->tellgBase + m_pStreamBuf->p - m_pStreamBuf->beg - flagFilePtrPos;	///< Ҫ�����ĳ���
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
				else	///< Ϊ�գ��� delete naluDataTmp
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

				// 4. ���س���
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
// ������:	H264Analysis::Read
// ����:	��ȡ��������ioReadLength���ֽڣ����ɹ���ȡ���ֽ���(*outLengthRead)����(*inBufferLength)������᷵��Failed�������������Success
// ����ֵ:	StatusCode ״ֵ̬( Failed => 0, Success => 1 )
// ����: 	char * outBuffer(out: ������ȡ���ֽ����ݣ���Ҫ�����߷�����ͷ��ڴ�)
// ����: 	UINT32 *inBufferLength(in: ����Ҫ��ȡ���ݵ��ֽڳ���)
// ����: 	UINT32 *outLengthRead(out: ���سɹ���ȡ���ݵ��ֽڳ���)
// ע��:	һ������£�*inBufferLength == *outLengthRead��ֻ���ڿ�����ļ���βʱ�����ߵ�ֵ�Ż᲻ͬ *inBufferLength > *outLengthRead
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::GetNaluCount
// ����:	��ȡNALU����
// ����ֵ:	UINT32
// ����: 	2016/06/15
// ����: 	YJZ
// �޸ļ�¼:
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
	m_fileStream.seekg(0);	// ��ԭ�ļ�ָ��λ��
	return naluCount;
}
