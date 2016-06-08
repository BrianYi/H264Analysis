#ifndef H264PUBLICDEF_H
#define H264PUBLICDEF_H

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

typedef int STATUS;	// ����״̬ ( 0 => Failed, 1 => Success)
const size_t BUFSIZE = 10 * 1024 * 1024;	// һ�ζ�ȡ���ֽ���

//
// ���ṹ
// 
typedef struct { 
	char *buf;			///< �����ݻ�����
	unsigned int len;	///< �����ݳ���
	unsigned int pos;	///< ������ָ�뵱ǰλ��
	unsigned int tellgBase;	///< �����������ļ����е�λ��
} DataStream, *PDataStream;	

//
// SPS�ṹ
// 
typedef struct {
	unsigned char profile_idc;
	bool constraint_set0_flag;
	/**
	 * δ���
	 */
}SPS, *PSPS;


//
// �ֽ�λ�ߵ�(��: 0011 0001 => 1000 1100)
// 
unsigned char bits_reverse(unsigned char data);

//
// ���ֽ�λ�ߵ�(��: 0011 0001 0111 1111 => 1111 1110 1000 1100)
// 
char* bits_reverse(char *data, int len);

//
// ���ֽڶ��뷵���ֽ���
//
int bits_get_byte_num(int len);

//
// ��c�Զ�������ʽ���, ��ʽΪ���ҵ���(���λ�����λ)
//
void bits_binary_printf(char c);

//
// ��data�Զ�������ʽ���, ��ʽΪ���ҵ���(���λ�����λ)
//
void bits_binary_printf(char *data, int len);

//
// ��data��ʮ��������ʽ���, С�˴洢�ķ�ʽ
//
void bytes_hex_printf(char *data, int len);

//
// ��ȡ��ȡ�ֽڵ�posλ��, ����Ϊlenλ���ֽ�����
//
const char B8_VAL_MASK(unsigned int pos, unsigned int len);

//
// ��ȡ��ȡ���ֽڵ�posλ��, ����Ϊnλ���ֽ�����
//
const UINT32 B32_VAL_MASK(unsigned int pos, unsigned int len);

//
// ��ȡ��ȡ�ֽڵ�posλ��ʼ���������ֽ�����
//
const char B8_VAL_MASK(unsigned int pos);

//
// ��ȡ��ȡ���ֽڵ�posλ��ʼ���������ֽ�����
//
const UINT32 B32_VAL_MASK(unsigned int pos);

//
// ��ȡ��ȡ�ֽ�val��posλ�𵽽�����ֵ�����������ұߵ�һλ��0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos);

//
// ��ȡ��ȡ���ֽ�val��posλ�𵽽�����ֵ�����������ұߵ�һλ��0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos);

//
// ��ȡ��ȡ�ֽ�val��posλ��, ����Ϊnλ���ֽڲ��������ұߵ�һλ��0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos, unsigned int len);

//
// ��ȡ��ȡ���ֽ�val��posλ��, ����Ϊnλ���ֽڲ��������ұߵ�һλ��0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos, unsigned int len);

//
// ��ȡ��ȡ�ֽ�val��posλ�𵽽�����ֵ������������ߵ�һλ��0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos);

//
// ��ȡ��ȡ���ֽ�val��posλ�𵽽�����ֵ������������ߵ�һλ��0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos);

//
// ��ȡ��ȡ�ֽ�val��posλ��, ����Ϊnλ���ֽڲ���������ߵ�һλ��0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos, unsigned int len);

//
// ��ȡ��ȡ���ֽ�val��posλ��, ����Ϊnλ���ֽڲ���������ߵ�һλ��0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos, unsigned int len);

//
// �ֽ�val��posλ��ʼ��lenλ����(����������)
//
const char B8_VAL_ZERO(unsigned char val, unsigned int pos, unsigned int len);

//
// ���ֽ�val��posλ��ʼ��lenλ����(����������)
//
const UINT32 B32_VAL_ZERO(UINT32 val, unsigned int pos, unsigned int len);

//
// �ֽ�val��posλ��ʼ��lenλ��һ(����������)
//
const char B8_VAL_ONE(unsigned char val, unsigned int pos, unsigned int len);

//
// ���ֽ�val��posλ��ʼ��lenλ��һ(����������)
//
const UINT32 B32_VAL_ONE(UINT32 val, unsigned int pos, unsigned int len);

//
// ���ֽ�srcVal�н�ȡsrcPos��ʼ��lenλ������䵽�ֽ�dstVal�д�dstPos��ʼ��lenλ(����������)
//
const char B8_VAL_FILL(unsigned char srcVal, unsigned int srcPos, unsigned char dstVal, unsigned int dstPos, unsigned int len);

//
// �����ֽ�srcVal�н�ȡsrcPos��ʼ��lenλ������䵽���ֽ�dstVal�д�dstPos��ʼ��lenλ(����������)
//
const UINT32 B32_VAL_FILL(UINT32 srcVal, unsigned int srcPos, UINT32 dstVal, unsigned int dstPos, unsigned int len);

//
// �ֽڵߵ�(��: 0011 0001 0111 1111 => 1111 0111 0001 0011)
//
char* bytes_reverse(char *data, int len);

#endif