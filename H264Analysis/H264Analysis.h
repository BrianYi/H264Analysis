#ifndef H264ANALYSIS_H
#define H264ANALYSIS_H

#include <fstream>


typedef unsigned char       UInt8;
typedef signed char         SInt8;
typedef unsigned short      UInt16;
typedef signed short        SInt16;
typedef unsigned int        UInt32;
typedef signed int          SInt32;
typedef signed long long	SInt64;
typedef unsigned long long	UInt64;
typedef float               Float32;
typedef double              Float64;
typedef UInt16              Bool16;
typedef UInt8               Bool8;

//#define TIME_TEST // ͳ��ÿ������������ʱ�䣬������

class H264Analysis
{
public:
	//
	// nal_unit_type��ֵ��ö������
	//
	enum NalUnitType {
		NAL_SLICE			=	1,	///< ��������
		NAL_DPA				=	2,	///< slice_data_partition_a_layer_rbsp, �����������ݷ���A
		NAL_DPB				=	3,	///< slice_data_partition_b_layer_rbsp, �����������ݷ���B
		NAL_DPC				=	4,	///< slice_data_partition_c_layer_rbsp, �����������ݷ���C
		NAL_IDR_SLICE		=	5,	///< slice_layer_without_partitioning_rbsp, IDRͼ�ı�������
		NAL_SEI				=	6,	///< sei_rbsp, SEI��ǿ������Ϣ
		NAL_SPS				=	7,	///< seq_parameter_set_rbsp, ���в�����
		NAL_PPS				=	8,	///< pic_parameter_set_rbsp, ͼ�������
		NAL_AUD				=	9,	///< access_unit_delimiter_rbsp, ���ʵ�Ԫ���޷�
		NAL_END_SEQUENCE	=	10,	///< end_of_seq_rbsp, ���н�β
		NAL_END_STREAM		=	11,	///< end_of_stream_rbsp, ����β
		NAL_FILLER_DATA		=	12,	///< filler_data_rbsp, �������
		NAL_SPS_EXT			=	13,	///< seq_parameter_set_extension_rbsp, ���в�������չ
		NAL_AUXILIARY_SLICE	=	19,	///< slice_layer_without_partitioning_rbsp, δ���ֵĸ�������ͼ�ı�������
		NAL_FF_IGNORE		=	0xff0f001	///< ����
	};

	//
	// slice_type������
	//
	enum SliceType {
		SLICE_TYPE_P1	=	0,
		SLICE_TYPE_B1	=	1,
		SLICE_TYPE_I1	=	2,
		SLICE_TYPE_SP1	=	3,
		SLICE_TYPE_SI1	=	4,
		SLICE_TYPE_P2	=	5,
		SLICE_TYPE_B2	=	6,
		SLICE_TYPE_I2	=	7,
		SLICE_TYPE_SP2	=	8,
		SLICE_TYPE_SI2	=	9,
		SLICE_TYPE_NONE =	10
	};

	enum { Success = 1, Failed = 0 };

	typedef int STATUS;	// ����״̬ ( 0 => Failed, 1 => Success)
	static const UInt32 BUFSIZE = 10 * 1024 * 1024;	// һ�ζ�ȡ���ֽ���

	//
	// ���ṹ
	// 
	typedef struct { 
		char *beg;			///< �����ݻ�����ͷ
		char *end;			///< �����ݻ�����β
		char *p;			///< �����ݻ�����ָ��
		UInt32 len;	///< �����ݳ���
		UInt32 tellgBase;	///< �����������ļ����е�λ��
	} DataStream, *PDataStream;	

public:
	H264Analysis(void);
	~H264Analysis(void);

	// ����:	��ȡ�ļ���
	// ����ֵ:	std::ifstream& 
	// ����: 	const char* fileName
	std::ifstream& GetOpenFile(const char* fileName);

	// ����:	��ȡ�ļ�·��ȫ��
	// ����ֵ:	char*
	char* GetFilePath()	{ return m_pFilePath; }

	// ����:	�ر��ļ�
	// ����ֵ:	void
	void CloseFile();


	// ����:	��ȡ��һ��Nalu, �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����
	STATUS NextNalu(char **outNaluData, UInt32 *outLength);

	// ����:	��ȡ��һ������SPS��Nalu, �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����
	STATUS NextSpsNalu(char **outNaluData, UInt32 *outLength);

	// ����:	��ȡ��һ������PPS��Nalu, �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����
	STATUS NextPpsNalu(char **outNaluData, UInt32 *outLength);

	// ����:	��ȡ��һ������IDR֡��Nalu, �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����
	STATUS NextIdrNalu(char **outNaluData, UInt32 *outLength);

	// ����:	����:	��ȡ��һ������I֡��Nalu(��ǰ����SPS,PPS��SEI��Ὣ������I֡һ����), �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����
	// ����: 	UInt16 inSpeed(in: Ĭ��Ϊ1�����������ٶ�����һ��I֡��inSpeedΪ2ʱ����ÿ2��Iֻ֡����һ��I֡��������������)
	STATUS NextInalu(char **outNaluData, UInt32 *outLength, UInt16 inSpeed = 1);

	// ����:	��ȡ��һ������P֡��Nalu, �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����
	STATUS NextPnalu( char **outNaluData, UInt32 *outLength );

	// ����:	��ȡ��һ������B֡��Nalu, �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����
	STATUS NextBnalu(char **outNaluData, UInt32 *outLength);

	// ����:	��ȡ��һ������SI֡��Nalu, �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����

	STATUS NextSiNalu(char **outNaluData, UInt32 *outLength);

	// ����:	��ȡ��һ������SP֡��Nalu, �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����
	STATUS NextSpNalu(char **outNaluData, UInt32 *outLength);	

	// ����:	��ת��ָ��λ��(�ٷֱȱ�ʾ0.0~1.0)
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����: 	Float32 inPersent(in: 0.0~1.0, ����Ҫ��ת�İٷֱ�)
	STATUS SeekDstToPos(Float32 inPersent);

	// ����:	��ȡNalu������
	// ����ֵ:	NalUnitType => Nalu����
	// ����: 	char * inNaluData(in: ����Nalu����)
	NalUnitType GetNaluType(char *inNaluData);

	// ����:	��ȡ֡����
	// ����ֵ:	SliceType => ֡����
	// ����: 	char * inNaluData(in: ����Nalu����)
	// ����: 	UInt32 inLength(in: ����Nalu����)
	SliceType GetSliceType(char *inNaluData, UInt32 inLength);

	// ����:	����startCode����
	// ����ֵ:	size_t => startCode����
	// ����: 	char * inNaluData
	// ����: 	UInt32 outLength(out: ����Nalu����)
	STATUS GetStartCodeLength(char *inNaluData, UInt32 *outLength);

	// ����:	����Exp-Golomb-Code(ָ�����ײ�����)
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *inUeExpGolombCodeData(in: ����Ҫ�����Exp-Golomb-Code����)
	// ����: 	UInt32 inLength(in: ����Exp-Golomb-Code���ݵĳ���)
	// ����: 	UInt32 * outResult(out: ����������ֵ)
	// ����:	UInt32 * outResultLength(out: ���������õ��ֽ���)
	STATUS ParseUeExpGolombCode(char *inUeExpGolombCodeData, UInt32 inLength, UInt32 *outResult, UInt32 *outResultLength);		///< �����޷�������ָ�����ײ�����

	// ����:	��ȡ���ݻ�����
	// ����ֵ:	PDataSteam
	PDataStream GetStreamBuf() { return m_pStreamBuf; }

	// ����:	��ȡNALU����
	// ����ֵ:	UInt32
	UInt32 GetNaluCount() { return m_naluCount; }

	// ����:	��ȡ֡��(Ĭ��25֡/s)
	// ����ֵ:	UInt32
	UInt32 GetFrameRate() { return m_frameRate; };

	// ����:	��ȡ�ļ���ʱ��
	// ����ֵ:	Float64
	Float64 GetFileFrameTotalTime() { return m_frameTotalTime; }

protected:
	// ����:	��ȡ��һ������I֡��Nalu(��ǰ����SPS,PPS��SEI��Ὣ������I֡һ����), �������ݷ������outNaluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����:	char *outNaluData	
	//			1. Not NULL => ����Nalu, ����startCode
	//			2. NULL		=> ����ȡ����
	// ����:	UInt32 *outLength
	//			1. Not NULL => ����Nalu�ĳ���
	//			2. NULL		=> ����ȡ����
	STATUS Next_I_Nalu(char **outNaluData, UInt32 *outLength);

	// ����:	��ȡ��������ioReadLength���ֽڣ����ɹ���ȡ���ֽ���(*outLengthRead)����(*inBufferLength)������᷵��Failed�������������Success
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	// ����: 	char * outBuffer(out: ������ȡ���ֽ����ݣ���Ҫ�����߷�����ͷ��ڴ�)
	// ����: 	UInt32 *inBufferLength(in: ����Ҫ��ȡ���ݵ��ֽڳ���)
	// ����: 	UInt32 *outLengthRead(out: ���سɹ���ȡ���ݵ��ֽڳ���)
	// ע��:	һ������£�*inBufferLength == *outLengthRead��ֻ���ڿ�����ļ���βʱ�����ߵ�ֵ�Ż᲻ͬ *inBufferLength > *outLengthRead
	inline STATUS Read( char *outBuffer, UInt32 *inBufferLength, UInt32 *outLengthRead);///< ��ȡlen���ֽ�

	// ����:	��黺�����������Ƿ񻹹������������ٴζ�ȡBUFSIZE�ֽڵ��ļ�����
	// ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	inline STATUS CheckStreamBuf();

	// ����:	��ջ�����
	// ����ֵ:	void
	inline void ClearStreamBuf();

	// ����:	��ȡNALU����
	// ����ֵ:	UInt32
	UInt32 NaluCount();
public:
	std::ifstream m_fileStream;	///< �ļ���
	char *m_pFilePath;			///< �ļ�����
	UInt32 m_fileLen;			///< �����ݳ���
	char m_binPos;				///< ������ָ���λ��, ʼ��ָ��Ҫ������һλ(8λ, ��0��ʼ, ֵΪ0~7)
	UInt8 m_lastByte;			///< ���һ�ζ����ֽ�����
	PDataStream m_pStreamBuf;	///< ������
	UInt32 m_frameRate;			///< ֡��(Ĭ��25֡/s)
	UInt32 m_naluCount;			///< Nalu����
	Float64 m_frameTotalTime;	///< ��ʱ��
#ifdef TIME_TEST
private:
	std::ofstream m_debugFileStream; ///< ������
#endif
};
#endif