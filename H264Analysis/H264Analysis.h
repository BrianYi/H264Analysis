#ifndef H264ANALYSIS_H
#define H264ANALYSIS_H

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <Windows.h>
#include "PublicDef.h"
using namespace std;

class H264Analysis
{
public:
	H264Analysis(void);
	~H264Analysis(void);
	const ifstream& getOpenFile(const string &fileName);		///< 获取并打开文件
	const char* getSeqParameterSet(const char* nalBeg);	///< 获取SPS, 需传入NALU的首地址
	const char* getPicParameterSet(const char* nalBeg);	///< 获取PPS, 需传入NALU的首地址
	const char* get_I_frame(const char* nalBeg);		///< 获取I帧, 需传入NALU的首地址
	const char* get_P_frame(const char* nalBeg);		///< 获取P帧, 需传入NALU的首地址
	const char* get_B_frame(const char* nalBeg);		///< 获取B帧, 需传入NALU的首地址
	const char* get_SI_frame(const char* nalBeg);		///< 获取SI帧, 需传入NALU的首地址
	const char* get_SP_frame(const char* nalBeg);		///< 获取SP帧, 需传入NALU的首地址
	const char* nextNALU();								///< 获取下一NALU的首地址
	int nextNALUSize();									///< 获取下一NALU的大小
	const char* next_I_frame();							///< 获取下一I帧的首地址
	const char* next_P_frame();							///< 获取下一P帧的首地址
	const char* next_B_frame();							///< 获取下一B帧的首地址
	const char* next_SI_frame();						///< 获取下一SI帧的首地址
	const char* next_SP_frame();						///< 获取下一SP帧的首地址
	UINT32 readNextBits(int n, BOOL* isFinished = NULL);///< 读取n个位(0~32)
	void closeFile();	///< 关闭文件
protected:
	int ueDecode();		///< 解码无符号整型指数哥伦布编码
	bool byteAligned();	///< 是否处于字节边界
private:
	ifstream m_iFileStream;
	char m_binCurPos;	///< 二进制指针的位置, 始终指向要读的下一位(8位, 从0开始, 值为0~7)
	UINT8 m_lastByte;	///< 最后一次读的字节内容
};
#endif