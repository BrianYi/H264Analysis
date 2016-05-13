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
	const ifstream& getOpenFile(const string &fileName);		///< ��ȡ�����ļ�
	const char* getSeqParameterSet(const char* nalBeg);	///< ��ȡSPS, �贫��NALU���׵�ַ
	const char* getPicParameterSet(const char* nalBeg);	///< ��ȡPPS, �贫��NALU���׵�ַ
	const char* get_I_frame(const char* nalBeg);		///< ��ȡI֡, �贫��NALU���׵�ַ
	const char* get_P_frame(const char* nalBeg);		///< ��ȡP֡, �贫��NALU���׵�ַ
	const char* get_B_frame(const char* nalBeg);		///< ��ȡB֡, �贫��NALU���׵�ַ
	const char* get_SI_frame(const char* nalBeg);		///< ��ȡSI֡, �贫��NALU���׵�ַ
	const char* get_SP_frame(const char* nalBeg);		///< ��ȡSP֡, �贫��NALU���׵�ַ
	const char* nextNALU();								///< ��ȡ��һNALU���׵�ַ
	int nextNALUSize();									///< ��ȡ��һNALU�Ĵ�С
	const char* next_I_frame();							///< ��ȡ��һI֡���׵�ַ
	const char* next_P_frame();							///< ��ȡ��һP֡���׵�ַ
	const char* next_B_frame();							///< ��ȡ��һB֡���׵�ַ
	const char* next_SI_frame();						///< ��ȡ��һSI֡���׵�ַ
	const char* next_SP_frame();						///< ��ȡ��һSP֡���׵�ַ
	UINT32 readNextBits(int n, BOOL* isFinished = NULL);///< ��ȡn��λ(0~32)
	void closeFile();	///< �ر��ļ�
protected:
	int ueDecode();		///< �����޷�������ָ�����ײ�����
	bool byteAligned();	///< �Ƿ����ֽڱ߽�
private:
	ifstream m_iFileStream;
	char m_binCurPos;	///< ������ָ���λ��, ʼ��ָ��Ҫ������һλ(8λ, ��0��ʼ, ֵΪ0~7)
	UINT8 m_lastByte;	///< ���һ�ζ����ֽ�����
};
#endif