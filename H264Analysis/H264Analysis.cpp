#include "H264Analysis.h"

H264Analysis::H264Analysis(void)
{
	/// ��ʼ��������ָ��
	m_lastByte = 0;
	m_binCurPos = 0;
}


H264Analysis::~H264Analysis(void)
{
	if (m_iFileStream.is_open())
		m_iFileStream.close();
}


//************************************
// ������:	H264Analysis::getOpenFile
// ����:	��ȡ�����ļ�
// ����ֵ:	std::ifstream&
// ����: 	const string & fileName
// ����: 	2016/05/18
// ����: 	YJZ
// �޸ļ�¼:
//************************************
std::ifstream& H264Analysis::getOpenFile( const string &fileName )
{
	/// ��ʼ��������ָ��
	m_lastByte = 0;
	m_binCurPos = 0;

	if (!m_iFileStream.is_open())
		m_iFileStream.open(fileName.c_str(), ios_base::binary);

	return m_iFileStream;
}


//************************************
// ������:	H264Analysis::closeFile
// ����:	�ر��ļ�
// ����ֵ:	void
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
// ������:	H264Analysis::readNaluData
// ����:	��ȡNalu���ݲ��֣�������startCode��������startCode����ɺ��ļ�ָ��ָ��Nalu�����ݲ��ֿ�ͷ
// ����ֵ:	size_t => Nalu���ݲ��ֳ���
// ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
//************************************
size_t H264Analysis::readNaluData( char **naluData )
{
	// ��ȡ��ǰ��Naluλ��(����startCode)
	int curNaluPos = m_iFileStream.tellg();
	int len = 0;
	
	// ��λ������startCodeλ��
	int startCodeLen = skipNaluStartCode();

	// ��ȡNalu���ݲ��ֵ�λ��
	int curNaluDataPos = curNaluPos + startCodeLen;
	
	// ��ȡ��һNalu��λ��(����startCode)
	int nextNaluPos = 0;
	if (nextNalu(&nextNaluPos) == Failed)
		return Failed;

	// �ص�ԭ��Nalu���ݲ��ֵ�λ��
	m_iFileStream.seekg(curNaluDataPos);

	// ��ȡNalu�����ݲ���
	len = nextNaluPos - curNaluDataPos; // Nalu����
	if (naluData && readNextBytes(naluData, len) == Failed)
		return Failed;
	m_iFileStream.seekg(curNaluDataPos);	// �ص�ԭ����Naluλ��(����startCode)

	// ��λ������ָ����
	if (naluData)
	{
		m_binCurPos = 0;
		m_lastByte = (*naluData)[0];
	}
	
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

	int curNaluDataPos = m_iFileStream.tellg();
	int curNaluPos = curNaluDataPos - i;
	m_iFileStream.seekg(curNaluPos);
	if (naluPos)
		*naluPos = curNaluPos;
	
	return Success;
}


//************************************
// ������:	H264Analysis::next_SPS_Nalu
// ����:	��ȡ��һ������SPS��Nalu, �������ݷ������Nalu��������ɺ��ļ�ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => NALU���ݲ��ֳ���
// ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
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
		curNaluDataPos = m_iFileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
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
// ������:	H264Analysis::next_PPS_Nalu
// ����:	��ȡ��һ������PPS��Nalu, �������ݷ������Nalu��������ɺ��ļ�ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => NALU���ݲ��ֳ���
// ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
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
		curNaluDataPos = m_iFileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
		curNaluPos = curNaluDataPos - startCodeLen;	///< ��ȡ��ǰNalu����ʼλ��(����startCode)
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
// ������:	H264Analysis::next_I_Nalu
// ����:	��ȡ��һ������I֡��Nalu, �������ݷ������Nalu��������ɺ��ļ�ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => NALU���ݲ��ֳ���
// ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
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
		curNaluDataPos = m_iFileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
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
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// ������:	H264Analysis::next_P_Nalu
// ����:	��ȡ��һ������P֡��Nalu, �������ݷ������Nalu��������ɺ��ļ�ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => NALU���ݲ��ֳ���
// ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
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
		curNaluDataPos = m_iFileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
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
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// ������:	H264Analysis::next_B_Nalu
// ����:	��ȡ��һ������B֡��Nalu, �������ݷ������Nalu��������ɺ��ļ�ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => NALU���ݲ��ֳ���
// ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
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
		curNaluDataPos = m_iFileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
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
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// ������:	H264Analysis::next_SI_Nalu
// ����:	��ȡ��һ������SI֡��Nalu, �������ݷ������Nalu��������ɺ��ļ�ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => NALU���ݲ��ֳ���
// ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
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
		curNaluDataPos = m_iFileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
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
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// ������:	H264Analysis::next_SP_Nalu
// ����:	��ȡ��һ������SP֡��Nalu, �������ݷ������Nalu��������ɺ��ļ�ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
// ����ֵ:	size_t => NALU���ݲ��ֳ���
// ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
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
		curNaluDataPos = m_iFileStream.tellg();	///< ��ȡ��ǰNalu���ݲ��ֵ���ʼλ��(������startCode)
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
				m_iFileStream.seekg(curNaluPos);
				return readNaluData(naluData);
			}
		}
	}
	return Failed;
}

//************************************
// ������:	H264Analysis::skipNaluStartCode
// ����:	����Nalu��startCode, ������������startCode���ȣ���ɺ��ļ�ָ��ָ��Nalu�����ݲ��ֿ�ͷ
// ����ֵ:	size_t => ������startCode����
// ����: 	2016/05/17
// ����: 	YJZ
// �޸ļ�¼:
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
	if (len <= 0 || m_iFileStream.eof())
	{
		return Failed;
	}

	*p = new char[len];
	m_iFileStream.read(*p, len);
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
	if (m_iFileStream.eof())
	{
		return Failed;
	}

	m_iFileStream.read(c, 1);
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
		if (m_binCurPos == 0)
		{
			if (readNextByte((char*)&m_lastByte) == Failed)
				return Failed;
		}
		b = B8_VAL_BASE_R(m_lastByte, m_binCurPos, 1);	// read next bit
		m_binCurPos++;	// ָ������
		if (m_binCurPos % 8 == 0)
			m_binCurPos = 0;
	}

	if (leadingZeroBits > 32)
	{
		throw exception("");
		return -1;
	}

	// ָ�뻹��ʣ��λ��δ��
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
			if (leadingZeroBits <= leftBits)	// ʣ��λ���㹻������ײ�����
			{
				readBits = B8_VAL_BASE_R(m_lastByte, m_binCurPos, leadingZeroBits);
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
				golombCode = (golombCode>>leftBits)|(B32_VAL_BASE_L((UINT32)B8_VAL_BASE_L(m_lastByte, m_binCurPos), 24));
				golombCode = B32_VAL_BASE_R(golombCode, 0, leadingZeroBits);
				*codeNum = golombCode;
				
				// �������ֽ�
				m_lastByte = p[leftLeadingZeroBytes-1];
				delete [] p;
			}
			m_binCurPos += leadingZeroBits;
			m_binCurPos %= 8;
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

			m_binCurPos += leadingZeroBits;
			m_binCurPos %= 8;
			*codeNum = pow(2.0, leadingZeroBits) - 1 + readBits;
		}
	}
	return Success;
}

