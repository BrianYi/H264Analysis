#include "H264Analysis.h"

H264Analysis::H264Analysis(void)
{
	/// ��ʼ�����ṹ
	m_stream = new DataStream();
	m_stream->buf = NULL;
	m_stream->len = 0;
	m_stream->ptr = NULL;
	m_stream->lastByte = 0;
	m_stream->binPos = 0;
}


H264Analysis::~H264Analysis(void)
{
	clearStream();
	delete m_stream;
	m_stream = NULL;
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
DataStream* H264Analysis::getStream( const string &fileName )
{
	/// ��ʼ�����ṹ
	if (m_stream->buf)
		delete [] m_stream->buf;
	m_stream->buf = NULL;
	m_stream->len = 0;
	m_stream->ptr = NULL;
	m_stream->lastByte = 0;
	m_stream->binPos = 0;

	/// �ļ������ݴ�ŵ����ṹ��
	ifstream fileStream(fileName.c_str(), ios_base::binary);
	fileStream.seekg(0, ios_base::end);
	m_stream->len = fileStream.tellg();
	m_stream->buf = new char [m_stream->len];
	fileStream.seekg(ios_base::beg);
	streambuf *sb = fileStream.rdbuf();
	sb->sgetn(m_stream->buf, m_stream->len);
	m_stream->ptr = m_stream->buf;
	fileStream.close();

	return m_stream;
}



//************************************
// ������:	H264Analysis::clearStream
// ����:	���������
// ����ֵ:	void
// ����: 	2016/05/19
// ����: 	YJZ
// �޸ļ�¼:
//************************************
void H264Analysis::clearStream()
{
	if (m_stream)
	{
		if (m_stream->buf)
		{
			delete [] m_stream->buf;
			m_stream->buf = NULL;
		}

		m_stream->buf = NULL;
		m_stream->len = 0;
		m_stream->ptr = NULL;
		m_stream->binPos = 0;
		m_stream->lastByte = 0;
	}
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
	// ��ȡ��ǰ��Naluλ��(����startCode)
	int curNaluPos = m_stream->ptr - m_stream->buf;
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
		nextNaluPos = m_stream->len;
	}

	// �ص�ԭ��Nalu��λ��
	m_stream->ptr = m_stream->buf + curNaluPos;

	// ��ȡNalu�����ݳ���
	len = nextNaluPos - curNaluPos; // Nalu����
	
	// ��λ������ָ����
	if (naluData)
	{
		// ��ȡNalu����
		if (readNextBytes(naluData, len) == Failed)
			throw exception();
		m_stream->binPos = 0;
		m_stream->lastByte = (*naluData)[0];
	}

	m_stream->ptr = m_stream->buf + curNaluPos; // �ص�ԭ����Naluλ��

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

	int curNaluDataPos = m_stream->ptr - m_stream->buf;
	int curNaluPos = curNaluDataPos - i;
	m_stream->ptr = m_stream->buf + curNaluPos;
	if (naluPos)
		*naluPos = curNaluPos;

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
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< ����startCode
		curNaluDataPos = m_stream->ptr - m_stream->buf;	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SPS)
		{
			m_stream->ptr = m_stream->buf + curNaluPos;
			return readNaluData(naluData);
		}
	}
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
	int startCodeLen = 0;
	int curNaluPos = 0;
	int curNaluDataPos = 0;
	unsigned char nextByte = 0;
	UINT32 nal_unit_type = 0;
	while (nextNalu())
	{
		startCodeLen = skipNaluStartCode();	///< ����startCode
		curNaluDataPos = m_stream->ptr - m_stream->buf;	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_PPS)
		{
			m_stream->ptr = m_stream->buf + curNaluPos;
			return readNaluData(naluData);
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
		curNaluDataPos = m_stream->ptr - m_stream->buf;	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_I1 || slice_type == SLICE_TYPE_I2)
			{
				m_stream->ptr = m_stream->buf + curNaluPos;
				return readNaluData(naluData);
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
		curNaluDataPos = m_stream->ptr - m_stream->buf;	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_P1 || slice_type == SLICE_TYPE_P2)
			{
				m_stream->ptr = m_stream->buf + curNaluPos;
				return readNaluData(naluData);
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
		curNaluDataPos = m_stream->ptr - m_stream->buf;	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_B1 || slice_type == SLICE_TYPE_B2)
			{
				m_stream->ptr = m_stream->buf + curNaluPos;
				return readNaluData(naluData);
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
		curNaluDataPos = m_stream->ptr - m_stream->buf;	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_SI1 || slice_type == SLICE_TYPE_SI2)
			{
				m_stream->ptr = m_stream->buf + curNaluPos;
				return readNaluData(naluData);
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
		curNaluDataPos = m_stream->ptr - m_stream->buf;	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
		if (readNextByte((char*)&nextByte) == Failed)
			return Failed;
		nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		if (nal_unit_type == NAL_SLICE || nal_unit_type == NAL_IDR_SLICE || nal_unit_type == NAL_AUXILIARY_SLICE)
		{
			if (ueDecode(&first_mb_in_slice) == Failed || ueDecode(&slice_type) == Failed)
				return Failed;

			if (slice_type == SLICE_TYPE_SP1 || slice_type == SLICE_TYPE_SP2)
			{
				m_stream->ptr = m_stream->buf + curNaluPos;
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
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
	int len = 0;
	char *p = new char[4];
	int curNaluPos = m_stream->ptr - m_stream->buf;
	if (readNextBytes(&p, 4) == Failed)
		return Failed;
	if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01)
	{
		len = 3;
		m_stream->ptr = m_stream->buf + curNaluPos + 3;
	}
	else if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
	{
		len = 4;
	}
	delete [] p;

	if (len == 0)
	{
		m_stream->ptr = m_stream->buf + curNaluPos;
	}

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
	if (len <= 0 || m_stream->ptr - m_stream->buf + len > m_stream->len)
	{
		return Failed;
	}

	*p = new char[len];
	char *c = *p;
	while(len--)
		*c++ = *m_stream->ptr++;
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
	if (m_stream->ptr - m_stream->buf >= m_stream->len)
	{
		return Failed;
	}

	*c = *m_stream->ptr++;

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
	int leadingZeroBits = -1;
	for (char b = 0; !b; leadingZeroBits++)
	{
		if (m_stream->binPos == 0)
		{
			if (readNextByte((char*)&m_stream->lastByte) == Failed)
				return Failed;
		}
		b = B8_VAL_BASE_R(m_stream->lastByte, m_stream->binPos, 1);	// read next bit
		m_stream->binPos++;	// ָ������
		if (m_stream->binPos % 8 == 0)
			m_stream->binPos = 0;
	}

	if (leadingZeroBits > 32)
	{
		throw exception();
		return -1;
	}

	// ָ�뻹��ʣ��λ��δ��
	int readBits = 0;
	int leftBits = 8 - m_stream->binPos;
	int leadingZeroBytes = 0;
	if (m_stream->binPos != 0)
	{
		if (leadingZeroBits == 0)
		{
			*codeNum = 0;
		}
		else
		{
			if (leadingZeroBits <= leftBits)	// ʣ��λ���㹻������ײ�����
			{
				readBits = B8_VAL_BASE_R(m_stream->lastByte, m_stream->binPos, leadingZeroBits);
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
				golombCode = (golombCode>>leftBits)|(B32_VAL_BASE_L((UINT32)B8_VAL_BASE_L(m_stream->lastByte, m_stream->binPos), 24));
				golombCode = B32_VAL_BASE_R(golombCode, 0, leadingZeroBits);
				*codeNum = golombCode;

				// �������ֽ�
				m_stream->lastByte = p[leftLeadingZeroBytes-1];
				delete [] p;
			}
			m_stream->binPos += leadingZeroBits;
			m_stream->binPos %= 8;
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
			m_stream->lastByte = p[leadingZeroBytes-1];
			delete [] p;

			m_stream->binPos += leadingZeroBits;
			m_stream->binPos %= 8;
			*codeNum = pow(2.0, leadingZeroBits) - 1 + readBits;
		}
	}
	return Success;
}

