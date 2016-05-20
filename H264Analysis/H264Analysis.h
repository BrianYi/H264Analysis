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
	 * ����:	��ȡ������
	 * ����ֵ:	DataStream*
	 * ����: 	const string & fileName
	 */
	DataStream* getStream(const string &fileName);

	/**
	 * ������:	H264Analysis::clearStream
	 * ����:	���������
	 * ����ֵ:	void
	 */
	void clearStream();

	/**
	 * ����:	����startCode����ȡNalu���ݲ��֣�������startCode������ɺ�������ָ��ָ��Nalu�����ݲ��ֿ�ͷ
	 * ����ֵ:	size_t => Nalu���ݲ��ֳ���
	 * ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
	 */
	size_t readNaluData(char **naluData = NULL);

	/**
	 * ����:	�ҵ��¸�Nalu����ʼλ��
	 * ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	 * ����:	int * naluPos(out: ���ظ�Nalu����ʼλ��, ����startCode, Ϊ��ʱ����ȡ����)
	 */
	STATUS nextNalu(int *naluPos = NULL);

	/**
	 * ����:	��ȡ��һ������SPS��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => Nalu���ݲ��ֳ���
	 * ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
	 */
	size_t next_SPS_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������PPS��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => NALU���ݲ��ֳ���
	 * ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
	 */
	size_t next_PPS_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������I֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => NALU���ݲ��ֳ���
	 * ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
	 */
	size_t next_I_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������P֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => NALU���ݲ��ֳ���
	 * ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
	 */
	size_t next_P_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������B֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => NALU���ݲ��ֳ���
	 * ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
	 */
	size_t next_B_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������SI֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => NALU���ݲ��ֳ���
	 * ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
	 */
	size_t next_SI_Nalu(char **naluData = NULL);

	/**
	 * ����:	��ȡ��һ������SP֡��Nalu, �������ݷ������Nalu��������ɺ�������ָ��ָ��Nalu�����ݲ��ֿ�ͷ(NaluΪ��ʱ������ȡ���ݣ�ֻ���س���)
	 * ����ֵ:	size_t => NALU���ݲ��ֳ���
	 * ����: 	char * * naluData(out: ����Nalu���ݲ���, Ϊ��ʱ����ȡ����)
	 */
	size_t next_SP_Nalu(char **naluData = NULL);	

public:
	/**
	 * ����:	����Nalu��startCode, ������������startCode���ȣ���ɺ�������ָ��ָ��Nalu�����ݲ��ֿ�ͷ
	 * ����ֵ:	size_t => ������startCode����
	 */
	size_t skipNaluStartCode();

	/**
	 * ����:	��ȡ��������len���ֽ�
	 * ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	 * ����: 	char * * p(out: ������ȡ���ֽ����ݣ���Ҫ�������ͷ��ڴ�)
	 * ����: 	int len(in: ��ȡ�ֽڵĳ���)
	 */
	STATUS readNextBytes(char **p, int len);///< ��ȡlen���ֽ�

	/**
	 * ����:	��ȡ��������1���ֽ�
	 * ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	 * ����: 	char * c(out: �����ֽ�����)
	 */
	STATUS readNextByte(char *c);	///< ��ȡ��һ���ֽ�

	/**
	 * ����:	����Exp-Golomb-Code(ָ�����ײ�����)
	 * ����ֵ:	STATUS ״ֵ̬( Failed => 0, Success => 1 )
	 * ����: 	UINT32 * codeNum(out: ����������ֵ)
	 */
	STATUS ueDecode(UINT32 *codeNum);		///< �����޷�������ָ�����ײ�����
private:
	DataStream *m_stream;	///< ������
};
#endif