#ifndef PUBLICDEF_H
#define PUBLICDEF_H

#include <bitset>

/**
 * nal_unit_type��ֵ��ö������
 */
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
	NAL_FF_IGNORE		=	0xff0f001
};

/**
 * ͼ(֡)������
 */
enum PictureType {
	PICTURE_TYPE_NONE	=	0,	///< δ�����ͼƬ����
	PICTURE_TYPE_I,				///< I֡(Intra)
	PICTURE_TYPE_P,				///< P֡(Predicted)
	PICTURE_TYPE_B,				///< B֡(Bi-dir predicted)
	PICTURE_TYPE_SI,			///< SI֡(Switching Intra)
	PICTURE_TYPE_SP				///< SP֡(Switching Predicted)
};


/**
 * ��ȡ��Ӧ�ı���λ��(С�˶���)
 */
enum {
	B_1_BIT	=	0x01,
	B_2_BIT	=	0x03,
	B_3_BIT	=	0x07,
	B_4_BIT	=	0x0f,
	B_5_BIT	=	0x1f,
	B_6_BIT	=	0x3f,
	B_7_BIT	=	0x7f,
	B_8_BIT	=	0xff
};


extern void bytes_updown(char* beg, int n);
extern int bits_get_byte_num(int n);

#endif