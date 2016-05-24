#include "H264Analysis.h"

H264Analysis::H264Analysis(void)
{
	/// ��ʼ�����ṹ
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
// ������:	H264Analysis::getOpenFile
// ����:	��ȡ�����ļ�
// ����ֵ:	DataStream *
// ����: 	const string & fileName
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
//************************************
ifstream& H264Analysis::getOpenFile( const string &fileName )
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
// ������:	H264Analysis::clearStream
// ����:	���������
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
// ������:	H264Analysis::readNaluData
// ����:	��ȡNalu����ɺ�������ָ��ָ��Nalu�Ŀ�ͷ
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::readNaluData( char **naluData )
{
#ifdef TIME_TEST
	DWORD time_beg = GetTickCount();
#endif
	// ��ȡ��ǰ��Naluλ��(����startCode)
	int curNaluPos = m_fileStream.tellg();
	int len = 0;

	// ��λ������startCodeλ��
	int startCodeLen = skipNaluStartCode();

	// ��ȡNalu���ݲ��ֵ�λ��
	int curNaluDataPos = curNaluPos + startCodeLen;

	// ��ȡ��һNalu��λ��(����startCode)
	int nextNaluPos = 0;
	if (nextNalu(&nextNaluPos) == FileEnd)
	{
		// �ѵ��ļ���β
		nextNaluPos = m_len;
	}

	// ��ȡNalu�����ݳ���
	len = nextNaluPos - curNaluPos; // Nalu����

	// �ص�ԭ��Nalu��λ��
	m_fileStream.seekg(-len, ios::cur);
	
	// ��λ������ָ����
	if (naluData)
	{
		// ��ȡNalu����
		if (readNextBytes(naluData, len) == Failed)
			throw exception();
		m_binPos = 0;
		m_lastByte = (*naluData)[0];
		m_fileStream.seekg(-len, ios::cur);	// �ص�ԭ����Naluλ��
	}
#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "readNaluData " << time_diff << endl;
#endif
	return len;
}

//************************************
// ������:	H264Analysis::nextNalu
// ����:	�ҵ��¸�Nalu����ʼλ��
// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
// ����:	int * naluPos(out: ���ظ�Nalu����ʼλ��, ����startCode, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/16
// ����: 	YJZ
// �޸ļ�¼:
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

	// ���»ص�Nalu��ͷ��
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
// ������:	H264Analysis::next_SPS_Nalu
// ����:	��ȡ��һ������SPS��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�Ŀ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
		startCodeLen = skipNaluStartCode();	///< ����startCode
		//curNaluDataPos = m_fileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		//curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
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
// ������:	H264Analysis::next_PPS_Nalu
// ����:	��ȡ��һ������PPS��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�Ŀ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
		startCodeLen = skipNaluStartCode();	///< ����startCode
		//curNaluDataPos = m_fileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		//curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
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
// ������:	H264Analysis::next_I_Nalu
// ����:	��ȡ��һ������I֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�Ŀ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
		startCodeLen = skipNaluStartCode();	///< ����startCode
		//curNaluDataPos = m_fileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
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
// ������:	H264Analysis::next_P_Nalu
// ����:	��ȡ��һ������P֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�Ŀ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
		startCodeLen = skipNaluStartCode();	///< ����startCode
		//curNaluDataPos = m_fileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
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
// ������:	H264Analysis::next_B_Nalu
// ����:	��ȡ��һ������B֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�Ŀ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
		startCodeLen = skipNaluStartCode();	///< ����startCode
		//curNaluDataPos = m_fileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
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
// ������:	H264Analysis::next_SI_Nalu
// ����:	��ȡ��һ������SI֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�Ŀ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
		startCodeLen = skipNaluStartCode();	///< ����startCode
		//curNaluDataPos = m_fileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
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
// ������:	H264Analysis::next_SP_Nalu
// ����:	��ȡ��һ������SP֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�Ŀ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => Nalu���ȣ�����startCode��
// ����: 	char * * naluData(out: ����Nalu, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
		startCodeLen = skipNaluStartCode();	///< ����startCode
		//curNaluDataPos = m_fileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = (int)m_fileStream.tellg() - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
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

	size_t pos = m_len * persent * 0.01;
	m_fileStream.seekg(pos);

#ifdef TIME_TEST
	DWORD time_diff = GetTickCount() - time_beg;
	m_debugFileStream << "skipTo " << time_diff << endl;
#endif

	return Success;
}


//************************************
// ������:	H264Analysis::skipNaluStartCode
// ����:	����Nalu��startCode, ������������startCode���ȣ���ɺ�������ָ��ָ��Nalu�Ŀ�ͷ
// ����ֵ:	size_t => ������startCode����
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::readNextBytes
// ����:	��ȡ��������len���ֽ�
// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
// ����: 	char * * p(out: ������ȡ���ֽ����ݣ���Ҫ�������ͷ��ڴ�)
// ����: 	int len(in: ��ȡ�ֽڵĳ���)
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::readNextByte
// ����:	��ȡ��������1���ֽ�
// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
// ����: 	char * c(out: �����ֽ�����)
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::ueDecode
// ����:	����Exp-Golomb-Code(ָ�����ײ�����)
// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
// ����: 	UINT32 * codeNum(out: ����������ֵ)
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
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
		m_binPos++;	// ָ������
		if (m_binPos % 8 == 0)
			m_binPos = 0;
	}

	if (leadingZeroBits > 32)
	{
		throw exception();
		return -1;
	}

	// ָ�뻹��ʣ��λ��δ��
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
			if (leadingZeroBits <= leftBits)	// ʣ��λ���㹻������ײ�����
			{
				readBits = B8_VAL_BASE_R(m_lastByte, m_binPos, leadingZeroBits);
			}
			else	// ʣ��λ�ò���������ײ�����
			{
				int leftLeadingZeroBits = leadingZeroBits - leftBits;
				int leftLeadingZeroBytes = bits_get_byte_num(leftLeadingZeroBits);
				char *p = new char[leftLeadingZeroBytes];
				UINT32 golombCode = 0;
				if (readNextBytes(&p, leftLeadingZeroBytes) == Failed)
					return Failed;
				memcpy((void*)&golombCode, p, leftLeadingZeroBytes);
				bytes_reverse((char*)&golombCode, sizeof(golombCode));

				// ��ȡ������Exp-Golomb-Code
				golombCode &= B32_VAL_MASK(0, leftLeadingZeroBits);
				golombCode = (golombCode>>leftBits)|(B32_VAL_BASE_L((UINT32)B8_VAL_BASE_L(m_lastByte, m_binPos), 24));
				golombCode = B32_VAL_BASE_R(golombCode, 0, leadingZeroBits);
				*codeNum = golombCode;

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
			char *p = new char[leadingZeroBytes];
			UINT32 golombCode = 0;
			if (readNextBytes(&p, leadingZeroBytes) == Failed)
				return Failed;
			memcpy((void*)&golombCode, p, leadingZeroBytes);
			bytes_reverse((char*)&golombCode, sizeof(golombCode));

			// ��ȡ������Exp-Golomb-Code
			golombCode &= B32_VAL_MASK(0, leadingZeroBits);
			golombCode = B32_VAL_BASE_R(golombCode, 0, leadingZeroBits);
			*codeNum = golombCode;

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

	return Success;
}

