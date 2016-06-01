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

//#define TIME_TEST // 统计每个函数的运行时间，测试用

class H264Analysis
{
public:
	enum { Success = 1, Failed = 0, FileEnd = 0 };
public:
	H264Analysis(void);
	~H264Analysis(void);
	/**
	 * 描述:	获取数据流
	 * 返回值:	ifstream& 
	 * 参数: 	const string & fileName
	 */
	ifstream& getOpenFile(const string &fileName);

	/**
	 * 函数名:	H264Analysis::clearStream
	 * 描述:	关闭文件
	 * 返回值:	void
	 */
	void closeFile();

	/**
	 * 描述:	获取下一个Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
	 * 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	 * 参数:	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
	 */
	size_t nextNalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含SPS的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => Nalu长度（包含startCode）
	 * 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
	 */
	size_t next_SPS_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含PPS的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => Nalu长度（包含startCode）
	 * 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
	 */
	size_t next_PPS_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含IDR帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => Nalu长度（包含startCode）
	 * 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
	 */
	size_t next_IDR_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含I帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => Nalu长度（包含startCode）
	 * 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
	 */
	size_t next_I_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含P帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => Nalu长度（包含startCode）
	 * 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
	 */
	size_t next_P_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含B帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => Nalu长度（包含startCode）
	 * 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
	 */
	size_t next_B_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含SI帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => Nalu长度（包含startCode）
	 * 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
	 */
	size_t next_SI_Nalu(char **naluData = NULL);

	/**
	 * 描述:	获取下一个包含SP帧的Nalu, 并将数据放入参数naluData传出，完成后文件指针指向下一个Nalu的开头(naluData为空时，不获取数据，只返回长度)
	 * 返回值:	size_t => Nalu长度（包含startCode）
	 * 参数: 	char * * naluData(out: 返回Nalu, 包含startCode, 为空时不获取数据)
	 */
	size_t next_SP_Nalu(char **naluData = NULL);	

	/**
	 * 描述:	跳转到指定位置(0~100)
	 * 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	 * 参数: 	short int persent(in: 0~100, 传入要跳转的百分比)
	 */
	STATUS skipTo(short int persent);
	
	/**
	 * 描述:	获取Nalu的类型
	 * 返回值:	NalUnitType => Nalu类型
	 * 参数: 	char * naluData(in: 传入Nalu数据)
	 */
	NalUnitType getNaluType(char *naluData);

	/**
	 * 描述:	获取帧类型
	 * 返回值:	SliceType => 帧类型
	 * 参数: 	char * naluData(in: 传入Nalu数据)
	 * 参数: 	size_t naluLen(in: 传入Nalu长度)
	 */
	SliceType getSliceType(char *naluData, size_t naluLen);

	/**
	 * 描述:	返回startCode长度
	 * 返回值:	size_t => startCode长度
	 * 参数: 	char * p
	 */
	size_t scLen(char *p);

	/**
	 * 描述:	解码Exp-Golomb-Code(指数哥伦布编码)
	 * 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	 * 参数:	char *egcData(in: 传入要解码的Exp-Golomb-Code数据)
	 * 参数: 	size_t len(in: 传入Exp-Golomb-Code数据的长度)
	 * 参数: 	UINT32 * codeNum(out: 传出解码后的值)
	 * 参数:	unsigned int * egcSize(out: 传出解码用的字节数)
	 */
	STATUS ueDecode(char *egcData, size_t len, UINT32 *codeNum, unsigned int *egcSize);		///< 解码无符号整型指数哥伦布编码

public: // 当测试完毕后，需改为protected
	/**
	 * 描述:	读取接下来的len个字节
	 * 返回值:	size_t => 成功读取的字节数
	 * 参数: 	char * p(out: 传出读取的字节数据，需要调用者分配和释放内存)
	 * 参数: 	int len(in: 读取字节的长度)
	 */
	size_t readNextBytes(char *p, int len);///< 读取len个字节

	/**
	 * 描述:	检查缓冲区的数据是否还够，若不够则再次读取BUFSIZE字节的文件内容
	 * 返回值:	STATUS 状态值( Failed => 0, Success => 1 )
	 */
	STATUS checkStreamBuf();
public: // 当测试完毕后，需改为private
	ifstream m_fileStream;	///< 文件流
	size_t m_len;	///< 流数据长度
	char m_binPos;		///< 二进制指针的位置, 始终指向要读的下一位(8位, 从0开始, 值为0~7)
	UINT8 m_lastByte;		///< 最后一次读的字节内容
	PDataStream m_pStreamBuf;	///< 
private:
	ofstream m_debugFileStream; ///< 测试用
};
#endif