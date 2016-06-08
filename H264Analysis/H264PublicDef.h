#ifndef H264PUBLICDEF_H
#define H264PUBLICDEF_H

//
// nal_unit_type的值的枚举类型
//
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
	NAL_FF_IGNORE		=	0xff0f001	///< 保留
};

//
// slice_type的类型
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

typedef int STATUS;	// 返回状态 ( 0 => Failed, 1 => Success)
const size_t BUFSIZE = 10 * 1024 * 1024;	// 一次读取的字节数

//
// 流结构
// 
typedef struct { 
	char *buf;			///< 流数据缓存区
	unsigned int len;	///< 流数据长度
	unsigned int pos;	///< 流数据指针当前位置
	unsigned int tellgBase;	///< 缓冲区块在文件流中的位置
} DataStream, *PDataStream;	

//
// SPS结构
// 
typedef struct {
	unsigned char profile_idc;
	bool constraint_set0_flag;
	/**
	 * 未完成
	 */
}SPS, *PSPS;


//
// 字节位颠倒(如: 0011 0001 => 1000 1100)
// 
unsigned char bits_reverse(unsigned char data);

//
// 多字节位颠倒(如: 0011 0001 0111 1111 => 1111 1110 1000 1100)
// 
char* bits_reverse(char *data, int len);

//
// 按字节对齐返回字节数
//
int bits_get_byte_num(int len);

//
// 将c以二进制形式输出, 格式为从右到左(最低位到最高位)
//
void bits_binary_printf(char c);

//
// 将data以二进制形式输出, 格式为从右到左(最低位到最高位)
//
void bits_binary_printf(char *data, int len);

//
// 将data以十六进制形式输出, 小端存储的方式
//
void bytes_hex_printf(char *data, int len);

//
// 获取截取字节第pos位起, 长度为len位的字节掩码
//
const char B8_VAL_MASK(unsigned int pos, unsigned int len);

//
// 获取截取四字节第pos位起, 长度为n位的字节掩码
//
const UINT32 B32_VAL_MASK(unsigned int pos, unsigned int len);

//
// 获取截取字节第pos位开始到结束的字节掩码
//
const char B8_VAL_MASK(unsigned int pos);

//
// 获取截取四字节第pos位开始到结束的字节掩码
//
const UINT32 B32_VAL_MASK(unsigned int pos);

//
// 获取截取字节val第pos位起到结束的值，并右移至右边第一位非0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos);

//
// 获取截取四字节val第pos位起到结束的值，并右移至右边第一位非0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos);

//
// 获取截取字节val第pos位起, 长度为n位的字节并右移至右边第一位非0
//
const char B8_VAL_BASE_R(unsigned char val, unsigned int pos, unsigned int len);

//
// 获取截取四字节val第pos位起, 长度为n位的字节并右移至右边第一位非0
//
const UINT32 B32_VAL_BASE_R(UINT32 val, unsigned int pos, unsigned int len);

//
// 获取截取字节val第pos位起到结束的值，并左移至左边第一位非0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos);

//
// 获取截取四字节val第pos位起到结束的值，并左移至左边第一位非0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos);

//
// 获取截取字节val第pos位起, 长度为n位的字节并左移至左边第一位非0
//
const char B8_VAL_BASE_L(unsigned char val, unsigned int pos, unsigned int len);

//
// 获取截取四字节val第pos位起, 长度为n位的字节并左移至左边第一位非0
//
const UINT32 B32_VAL_BASE_L(UINT32 val, unsigned int pos, unsigned int len);

//
// 字节val从pos位开始的len位清零(从左往右数)
//
const char B8_VAL_ZERO(unsigned char val, unsigned int pos, unsigned int len);

//
// 四字节val从pos位开始的len位清零(从左往右数)
//
const UINT32 B32_VAL_ZERO(UINT32 val, unsigned int pos, unsigned int len);

//
// 字节val从pos位开始的len位置一(从左往右数)
//
const char B8_VAL_ONE(unsigned char val, unsigned int pos, unsigned int len);

//
// 四字节val从pos位开始的len位置一(从左往右数)
//
const UINT32 B32_VAL_ONE(UINT32 val, unsigned int pos, unsigned int len);

//
// 从字节srcVal中截取srcPos开始的len位数据填充到字节dstVal中从dstPos开始的len位(从左往右数)
//
const char B8_VAL_FILL(unsigned char srcVal, unsigned int srcPos, unsigned char dstVal, unsigned int dstPos, unsigned int len);

//
// 从四字节srcVal中截取srcPos开始的len位数据填充到四字节dstVal中从dstPos开始的len位(从左往右数)
//
const UINT32 B32_VAL_FILL(UINT32 srcVal, unsigned int srcPos, UINT32 dstVal, unsigned int dstPos, unsigned int len);

//
// 字节颠倒(如: 0011 0001 0111 1111 => 1111 0111 0001 0011)
//
char* bytes_reverse(char *data, int len);

#endif