#ifndef PUBLICDEF_H
#define PUBLICDEF_H

#include <bitset>

/**
 * nal_unit_type的值的枚举类型
 */
enum NalUnitType {
	NAL_SLICE			=	1,	///< 条带类型
	NAL_DPA				=	2,	///< slice_data_partition_a_layer_rbsp, 编码条带数据分区A
	NAL_DPB				=	3,	///< slice_data_partition_b_layer_rbsp, 编码条带数据分区B
	NAL_DPC				=	4,	///< slice_data_partition_c_layer_rbsp, 编码条带数据分区C
	NAL_IDR_SLICE		=	5,	///< slice_layer_without_partitioning_rbsp, IDR图的编码条带
	NAL_SEI				=	6,	///< sei_rbsp, SEI增强补充信息
	NAL_SPS				=	7,	///< seq_parameter_set_rbsp, 序列参数集
	NAL_PPS				=	8,	///< pic_parameter_set_rbsp, 图像参数集
	NAL_AUD				=	9,	///< access_unit_delimiter_rbsp, 访问单元界限符
	NAL_END_SEQUENCE	=	10,	///< end_of_seq_rbsp, 序列结尾
	NAL_END_STREAM		=	11,	///< end_of_stream_rbsp, 流结尾
	NAL_FILLER_DATA		=	12,	///< filler_data_rbsp, 填充数据
	NAL_SPS_EXT			=	13,	///< seq_parameter_set_extension_rbsp, 序列参数集扩展
	NAL_AUXILIARY_SLICE	=	19,	///< slice_layer_without_partitioning_rbsp, 未划分的辅助编码图的编码条带
	NAL_FF_IGNORE		=	0xff0f001
};

/**
 * 图(帧)的类型
 */
enum PictureType {
	PICTURE_TYPE_NONE	=	0,	///< 未定义的图片类型
	PICTURE_TYPE_I,				///< I帧(Intra)
	PICTURE_TYPE_P,				///< P帧(Predicted)
	PICTURE_TYPE_B,				///< B帧(Bi-dir predicted)
	PICTURE_TYPE_SI,			///< SI帧(Switching Intra)
	PICTURE_TYPE_SP				///< SP帧(Switching Predicted)
};


/**
 * 获取对应的比特位数(小端对齐)
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