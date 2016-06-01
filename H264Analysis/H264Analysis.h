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

//#define TIME_TEST // ͳ��ÿ������������ʱ�䣬������

class H264Analysis
{
public:
	enum { Success = 1, Failed = 0, FileEnd = 0 };
public:
	H264Analysis(void);
	~H264Analysis(void);
	/**
	 * ����:	��ȡ������
	 * ����ֵ:	ifstream& 
	 * ����: 	const string & fileName
	 */
	ifstream& getOpenFile(const string &fileName);

	/**
	 * ������:	H264Analysis::clearStream
	 * ����:	�ر��ļ�
	 * ����ֵ:	void
	 */
	void closeFile();

	/**
	 * ����:	��ȡ��һ��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	 * ����:	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	size_t nextNalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������SPS��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => Nalu���ȣ�����startCode��
	 * ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	size_t next_SPS_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������PPS��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => Nalu���ȣ�����startCode��
	 * ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	size_t next_PPS_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������IDR֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => Nalu���ȣ�����startCode��
	 * ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	size_t next_IDR_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������I֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => Nalu���ȣ�����startCode��
	 * ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	size_t next_I_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������P֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => Nalu���ȣ�����startCode��
	 * ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	size_t next_P_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������B֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => Nalu���ȣ�����startCode��
	 * ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	size_t next_B_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������SI֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => Nalu���ȣ�����startCode��
	 * ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	size_t next_SI_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������SP֡��Nalu, �������ݷ������naluData��������ɺ��ļ�ָ��ָ����һ��Nalu�Ŀ�ͷ(naluDataΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => Nalu���ȣ�����startCode��
	 * ����: 	char * * naluData(out: ����Nalu, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	size_t next_SP_Nalu(char **naluData = NULL);	

	/**
	 * ����:	��ת��ָ��λ��(0~100)
	 * ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	 * ����: 	short int persent(in: 0~100, ����Ҫ��ת�İٷֱ�)
	 */
	STATUS skipTo(short int persent);
	
	/**
	 * ����:	��ȡNalu������
	 * ����ֵ:	NalUnitType => Nalu����
	 * ����: 	char * naluData(in: ����Nalu����)
	 */
	NalUnitType getNaluType(char *naluData);

	/**
	 * ����:	��ȡ֡����
	 * ����ֵ:	SliceType => ֡����
	 * ����: 	char * naluData(in: ����Nalu����)
	 * ����: 	size_t naluLen(in: ����Nalu����)
	 */
	SliceType getSliceType(char *naluData, size_t naluLen);

	/**
	 * ����:	����startCode����
	 * ����ֵ:	size_t => startCode����
	 * ����: 	char * p
	 */
	size_t scLen(char *p);

	/**
	 * ����:	����Exp-Golomb-Code(ָ�����ײ�����)
	 * ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	 * ����:	char *egcData(in: ����Ҫ�����Exp-Golomb-Code����)
	 * ����: 	size_t len(in: ����Exp-Golomb-Code���ݵĳ���)
	 * ����: 	UINT32 * codeNum(out: ����������ֵ)
	 * ����:	unsigned int * egcSize(out: ���������õ��ֽ���)
	 */
	STATUS ueDecode(char *egcData, size_t len, UINT32 *codeNum, unsigned int *egcSize);		///< �����޷�������ָ�����ײ�����

public: // ��������Ϻ����Ϊprotected
	/**
	 * ����:	��ȡ��������len���ֽ�
	 * ����ֵ:	size_t => �ɹ���ȡ���ֽ���
	 * ����: 	char * p(out: ������ȡ���ֽ����ݣ���Ҫ�����߷�����ͷ��ڴ�)
	 * ����: 	int len(in: ��ȡ�ֽڵĳ���)
	 */
	size_t readNextBytes(char *p, int len);///< ��ȡlen���ֽ�

	/**
	 * ����:	��黺�����������Ƿ񻹹������������ٴζ�ȡBUFSIZE�ֽڵ��ļ�����
	 * ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	 */
	STATUS checkStreamBuf();
public: // ��������Ϻ����Ϊprivate
	ifstream m_fileStream;	///< �ļ���
	size_t m_len;	///< �����ݳ���
	char m_binPos;		///< ������ָ���λ��, ʼ��ָ��Ҫ������һλ(8λ, ��0��ʼ, ֵΪ0~7)
	UINT8 m_lastByte;		///< ���һ�ζ����ֽ�����
	PDataStream m_pStreamBuf;	///< 
private:
	ofstream m_debugFileStream; ///< ������
};
#endif