#ifndef H264ANALYSIS_H
#define H264ANALYSIS_H

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <Windows.h>
#include <iomanip>
#include "H264PublicDef.h"
using namespace std;

class H264Analysis
{
public:
	enum { Success = 1, Failed = 0, FileEnd = 0 };
public:
	H264Analysis(void);
	~H264Analysis(void);
	/**
	 * 描述:	获取数据流
	 * 返回值:	DataStream*
	 * 参数: 	const string & fileName
	 */
	DataStream* getStream(const string &fileName);

	/**
	 * 函数名:	H264Analysis::clearStream
	 * 描述:	清空流数据
	 * 返回值:	void
	 */
	void clearStream();

	/**
	 * 描述:	跳过startCode并读取Nalu数据部分（不包括startCode），完成后数据流指针指向Nalu的数据部分开头
	 * 返回值:	size_t => Nalu数据部分长度
	 * 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
	 */
	size_t readNaluData(char **naluData = NULL);

	/**
	 * 描述:	找到下个Nalu的起始位置
	 * 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	 * 参数:	int * naluPos(out: 返回该Nalu的起始位置, 包含startCode, 为空时不获取数据)
	 */
	STATUS nextNalu(int *naluPos = NULL);

	/**
	 * 描述:	获取下一个包含SPS的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => Nalu数据部分长度
	 * 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
	 */
	size_t next_SPS_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含PPS的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => NALU数据部分长度
	 * 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
	 */
	size_t next_PPS_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含I帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => NALU数据部分长度
	 * 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
	 */
	size_t next_I_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含P帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => NALU数据部分长度
	 * 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
	 */
	size_t next_P_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含B帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => NALU数据部分长度
	 * 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
	 */
	size_t next_B_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含SI帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => NALU数据部分长度
	 * 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
	 */
	size_t next_SI_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含SP帧的Nalu, 并将数据放入参数Nalu传出，完成后数据流指针指向Nalu的数据部分开头(Nalu为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => NALU数据部分长度
	 * 参数: 	char * * naluData(out: 返回Nalu数据部分, 为空时不获取数据)
	 */
	size_t next_SP_Nalu(char **naluData = NULL);	

public:
	/**
	 * 描述:	跳过Nalu的startCode, 并返回跳过的startCode长度，完成后数据流指针指向Nalu的数据部分开头
	 * 返回值:	size_t => 跳过的startCode长度
	 */
	size_t skipNaluStartCode();

	/**
	 * 描述:	读取接下来的len个字节
	 * 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	 * 参数: 	char * * p(out: 传出读取的字节数据，需要调用者释放内存)
	 * 参数: 	int len(in: 读取字节的长度)
	 */
	STATUS readNextBytes(char **p, int len);///< 读取len个字节

	/**
	 * 描述:	读取接下来的1个字节
	 * 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	 * 参数: 	char * c(out: 传出字节数据)
	 */
	STATUS readNextByte(char *c);	///< 读取下一个字节

	/**
	 * 描述:	解码Exp-Golomb-Code(指数哥伦布编码)
	 * 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	 * 参数: 	UINT32 * codeNum(out: 传出解码后的值)
	 */
	STATUS ueDecode(UINT32 *codeNum);		///< 解码无符号整型指数哥伦布编码
private:
	DataStream *m_stream;	///< 数据流
};
#endif