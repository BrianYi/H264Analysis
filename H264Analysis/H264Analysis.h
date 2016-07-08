#ifndef H264ANALYSIS_H
#define H264ANALYSIS_H

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <Windows.h>
#include <iomanip>
#include "H264PublicDef.h"

//#define TIME_TEST // 统计每个函数的运行时间，测试用

class H264Analysis
{
public:
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

	enum { Success = 1, Failed = 0 };

	typedef int STATUS;	// 返回状态 ( 0 => Failed, 1 => Success)
	static const UINT32 BUFSIZE = 10 * 1024 * 1024;	// 一次读取的字节数

	//
	// 流结构
	// 
	typedef struct { 
		char *beg;			///< 流数据缓冲区头
		char *end;			///< 流数据缓冲区尾
		char *p;			///< 流数据缓冲区指针
		UINT32 len;	///< 流数据长度
		UINT32 tellgBase;	///< 缓冲区块在文件流中的位置
	} DataStream, *PDataStream;	
public:
	H264Analysis(void);
	~H264Analysis(void);
	// 描述:	获取文件流
	// 返回值:	ifstream& 
	// 参数: 	const char* fileName
	std::ifstream& GetOpenFile(const char* fileName);

	// 描述:	获取文件路径全名
	// 返回值:	char*
	char* GetFilePath()	{ return m_pFilePath; }

	// 描述:	关闭文件
	// 返回值:	void
	void CloseFile();


	// 描述:	获取下一个Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据
	STATUS NextNalu(char **outNaluData, UINT32 *outLength);

	// 描述:	获取下一个包含SPS的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据
	STATUS NextSpsNalu(char **outNaluData, UINT32 *outLength);

	// 描述:	获取下一个包含PPS的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据
	STATUS NextPpsNalu(char **outNaluData, UINT32 *outLength);

	// 描述:	获取下一个包含IDR帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据
	STATUS NextIdrNalu(char **outNaluData, UINT32 *outLength);

	// 描述:	描述:	获取下一个包含I帧的Nalu(若前面有SPS,PPS或SEI则会将他们与I帧一起打包), 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据
	// 参数: 	UINT16 inSpeed(in: 默认为1，代表正常速度找下一个I帧，inSpeed为2时代表每2个I帧只播第一个I帧，后面依次类推)
	STATUS NextInalu(char **outNaluData, UINT32 *outLength, UINT16 inSpeed = 1);

	// 描述:	获取下一个包含P帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据
	STATUS NextPnalu( char **outNaluData, UINT32 *outLength );

	// 描述:	获取下一个包含B帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据
	STATUS NextBnalu(char **outNaluData, UINT32 *outLength);

	// 描述:	获取下一个包含SI帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据

	STATUS NextSiNalu(char **outNaluData, UINT32 *outLength);

	// 描述:	获取下一个包含SP帧的Nalu, 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据
	STATUS NextSpNalu(char **outNaluData, UINT32 *outLength);	

	// 描述:	跳转到指定位置(百分比表示0.0~1.0)
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数: 	float inPersent(in: 0.0~1.0, 传入要跳转的百分比)
	STATUS SeekDstToPos(float inPersent);

	// 描述:	获取Nalu的类型
	// 返回值:	NalUnitType => Nalu类型
	// 参数: 	char * inNaluData(in: 传入Nalu数据)
	NalUnitType GetNaluType(char *inNaluData);

	// 描述:	获取帧类型
	// 返回值:	SliceType => 帧类型
	// 参数: 	char * inNaluData(in: 传入Nalu数据)
	// 参数: 	UINT32 inLength(in: 传入Nalu长度)
	SliceType GetSliceType(char *inNaluData, UINT32 inLength);

	// 描述:	返回startCode长度
	// 返回值:	size_t => startCode长度
	// 参数: 	char * inNaluData
	// 参数: 	UINT32 outLength(out: 传出Nalu长度)
	STATUS GetStartCodeLength(char *inNaluData, UINT32 *outLength);

	// 描述:	解码Exp-Golomb-Code(指数哥伦布编码)
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *inUeExpGolombCodeData(in: 传入要解码的Exp-Golomb-Code数据)
	// 参数: 	UINT32 inLength(in: 传入Exp-Golomb-Code数据的长度)
	// 参数: 	UINT32 * outResult(out: 传出解码后的值)
	// 参数:	UINT32 * outResultLength(out: 传出解码用的字节数)
	STATUS ParseUeExpGolombCode(char *inUeExpGolombCodeData, UINT32 inLength, UINT32 *outResult, UINT32 *outResultLength);		///< 解码无符号整型指数哥伦布编码

	// 描述:	获取数据缓冲区
	// 返回值:	PDataSteam
	PDataStream GetStreamBuf() { return m_pStreamBuf; }

	// 描述:	获取NALU总数
	// 返回值:	UINT32
	UINT32 GetNaluCount() { return m_naluCount; }

	// 描述:	获取帧率(默认25帧/s)
	// 返回值:	UINT32
	UINT32 GetFrameRate() { return m_frameRate; };

	// 描述:	获取文件总时间
	// 返回值:	float
	float GetFileFrameTotalTime() { return m_frameTotalTime; }

	// 描述:	检查缓冲区的数据是否还够，若不够则再次读取BUFSIZE字节的文件内容
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	inline STATUS CheckStreamBuf();

	// 描述:	清空缓冲区
	// 返回值:	void
	inline void ClearStreamBuf();
public: // 当测试完毕后，需改为protected
	// 描述:	获取下一个包含I帧的Nalu(若前面有SPS,PPS或SEI则会将他们与I帧一起打包), 并将数据放入参数outNaluData传出，完成后缓冲区指针(m_pStreamBuf->p)指向下一个Nalu的开头
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数:	char *outNaluData	
	//			1. Not NULL => 返回Nalu, 包含startCode
	//			2. NULL		=> 不获取数据
	// 参数:	UINT32 *outLength
	//			1. Not NULL => 传出Nalu的长度
	//			2. NULL		=> 不获取数据
	STATUS Next_I_Nalu(char **outNaluData, UINT32 *outLength);

	// 描述:	读取接下来的ioReadLength个字节，若成功读取的字节数(*outLengthRead)不足(*inBufferLength)个，则会返回Failed，其他情况返回Success
	// 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	// 参数: 	char * outBuffer(out: 传出读取的字节数据，需要调用者分配和释放内存)
	// 参数: 	UINT32 *inBufferLength(in: 传入要读取数据的字节长度)
	// 参数: 	UINT32 *outLengthRead(out: 返回成功读取数据的字节长度)
	// 注意:	一般情况下，*inBufferLength == *outLengthRead，只有在快读到文件结尾时，两者的值才会不同 *inBufferLength > *outLengthRead
	inline STATUS Read( char *outBuffer, UINT32 *inBufferLength, UINT32 *outLengthRead);///< 读取len个字节

	// 描述:	获取NALU总数
	// 返回值:	UINT32
	UINT32 NaluCount();
public: // 当测试完毕后，需改为private
	std::ifstream m_fileStream;	///< 文件流
	char *m_pFilePath;			///< 文件名字
	UINT32 m_fileLen;			///< 流数据长度
	char m_binPos;				///< 二进制指针的位置, 始终指向要读的下一位(8位, 从0开始, 值为0~7)
	UINT8 m_lastByte;			///< 最后一次读的字节内容
	PDataStream m_pStreamBuf;	///< 缓冲区
	UINT32 m_frameRate;			///< 帧率(默认25帧/s)
	UINT32 m_naluCount;			///< Nalu总数
	float m_frameTotalTime;	///< 总时间
private:
#ifdef TIME_TEST
	std::ofstream m_debugFileStream; ///< 测试用
#endif
};
#endif