#include "H264Analysis/H264Analysis.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <iomanip>
#include <Windows.h>
using namespace std;
const string g_fileNameStr = "../movie/500MTest.h264";

void H264AnalysisDebug();	// 测试函数

int main()
{
	H264AnalysisDebug();

#if 0	// test time of running when using buffer
	H264Analysis h264Analysis;
	ifstream& fileStream = h264Analysis.getOpenFile(g_fileNameStr);
	char *naluData = NULL;
	int naluSize = 0;
	int naluCount = 0;
	DWORD timeTotal_beg = GetTickCount();
	naluCount = h264Analysis.get_NALU_count();
	DWORD timeTotal_diff = GetTickCount() - timeTotal_beg;
	cout << naluCount << " time: " << timeTotal_diff << endl;
	cout << "total time: " << naluCount / 25 << " s " << endl;
	h264Analysis.closeFile();
#endif

#if 0 // test time of running when all data in mem
	size_t len = 0;
	ifstream is;
	is.open(g_fileNameStr.c_str(), std::ios_base::binary);
	is.seekg(0, std::ios_base::end);
	len = is.tellg();
	is.seekg(std::ios_base::beg);
	PDataStream pDataStream = new DataStream;
	pDataStream->buf = new char[len];
	pDataStream->len = len;
	pDataStream->pos = 0;
	pDataStream->tellgBase = 0;
	is.read(pDataStream->buf, len);
	is.close();
	char *p = pDataStream->buf;
	int naluCount = 0;
	DWORD timeTotal_beg = GetTickCount();
	for (int i = 0; i < len; )
	{
		if (p[i] == 0x00 && 
			p[i+1] == 0x00)
		{
			if (p[i+2] == 0x01)
			{
				i += 3;
				naluCount++;
			}
			else if (p[i+2] == 0x00 &&
				p[i+3] == 0x01)
			{
				i += 4;
				naluCount++;
			}
			else
				i+=2;
		}
		else
			i++;
	}
	DWORD timeTotal_diff = GetTickCount() - timeTotal_beg;
	cout << naluCount << " time: " << timeTotal_diff << endl;
	delete pDataStream->buf;
	delete pDataStream;
#endif
	
	return 0;
}


void H264AnalysisDebug()
{
	H264Analysis h264Analysis;
	if (h264Analysis.GetOpenFile(g_fileNameStr.c_str()) == NULL)
		cout << "Cannot open file " + g_fileNameStr + " !" << endl;
	UINT32 pCount = 0, bCount = 0, iCount = 0, siCount = 0, spCount = 0, spsCount = 0, ppsCount = 0, seiCount = 0, audCount = 0, slicCount = 0, idrCount = 0, auxCount = 0, dpaCount = 0, dpbCount = 0, dpcCount = 0;
	UINT32 pSize = 0, bSize = 0, iSize = 0, siSize = 0, spSize = 0, spsSize = 0, ppsSize = 0, seiSize = 0, audSize = 0, slicSize = 0, idrSize = 0, auxSize = 0, dpaSize = 0, dpbSize = 0, dpcSize = 0;
	UINT32 NaluCount = 0;
	UINT32 NaluSize = 0;
	UINT32 NaluTotalSize = 0;

	//h264Analysis.skipTo(99);
	DWORD timeTotal_beg = GetTickCount();

	char *naluData = NULL;
	while (h264Analysis.NextNalu(&naluData, &NaluSize) == H264Analysis::Success)
	{
		NaluCount++;
		NaluTotalSize += NaluSize;
		UINT32 startCodeLen = 0;
		if (h264Analysis.GetStartCodeLength(naluData,&startCodeLen) == H264Analysis::Failed)	throw;
		unsigned char nextByte = naluData[startCodeLen];
		UINT32 forbidden_zero_bit = nextByte>>7;
		UINT32 nal_ref_idc = (nextByte>>5)&0x3;
		H264Analysis::NalUnitType nal_unit_type = /*B8_VAL_BASE_R(nextByte, 3, 5);*/h264Analysis.GetNaluType(naluData);
		UINT32 first_mb_in_slice = 0;
		H264Analysis::SliceType slice_type = /*0;*/h264Analysis.GetSliceType(naluData, NaluSize);
		UINT32 pic_parameter_set_id = 0;
		unsigned int egcDataPos = startCodeLen + 1;
		unsigned int egcDataLen = NaluSize - egcDataPos;
		unsigned int egcSize = 0;
		int len = 0;
		switch (nal_unit_type)
		{
		case H264Analysis::NAL_SLICE:
		case H264Analysis::NAL_IDR_SLICE:
		case H264Analysis::NAL_AUXILIARY_SLICE:
			if (nal_unit_type == H264Analysis::NAL_SLICE)
			{
				slicCount++;
				slicSize += NaluSize;
			}
			else if (nal_unit_type == H264Analysis::NAL_IDR_SLICE)
			{
				idrCount++;
				idrSize += NaluSize;
			}
			else if (nal_unit_type == H264Analysis::NAL_AUXILIARY_SLICE)
			{
				auxCount++;
				auxSize += NaluSize;
			}

			switch (slice_type)
			{
			case H264Analysis::SLICE_TYPE_P1:
			case H264Analysis::SLICE_TYPE_P2:
				pCount++;
				pSize += NaluSize;
				break;
			case H264Analysis::SLICE_TYPE_B1:
			case H264Analysis::SLICE_TYPE_B2:
				bCount++;
				bSize += NaluSize;
				break;
			case H264Analysis::SLICE_TYPE_I1:
			case H264Analysis::SLICE_TYPE_I2:
				iCount++;
				iSize += NaluSize;
				break;
			case H264Analysis::SLICE_TYPE_SP1:
			case H264Analysis::SLICE_TYPE_SP2:
				spCount++;
				spsSize += NaluSize;
				break;
			case H264Analysis::SLICE_TYPE_SI1:
			case H264Analysis::SLICE_TYPE_SI2:
				siCount++;
				siSize += NaluSize;
				break;
			default:
				break;
			}
			break;
		case H264Analysis::NAL_DPA:
			dpaCount++;
			dpaSize += NaluSize;
			break;
		case H264Analysis::NAL_DPB:
			dpbCount++;
			dpbSize += NaluSize;
			break;
		case H264Analysis::NAL_DPC:
			dpcCount++;
			dpcSize += NaluSize;
			break;
		case H264Analysis::NAL_SEI:
			seiCount++;
			seiSize += NaluSize;
			break;
		case H264Analysis::NAL_SPS:
			spsCount++;
			spsSize += NaluSize;
			break;
		case H264Analysis::NAL_PPS:
			ppsCount++;
			ppsSize += NaluSize;
			break;
		case H264Analysis::NAL_AUD:
			audCount++;
			audSize += NaluSize;
			break;
		case H264Analysis::NAL_END_SEQUENCE:
			break;
		case H264Analysis::NAL_END_STREAM:
			break;
		case H264Analysis::NAL_FILLER_DATA:
			break;
		case H264Analysis::NAL_SPS_EXT:
			break;
		default:
			break;
		}

		delete naluData;
		naluData = NULL;
	}
	DWORD timeTotal_diff = GetTickCount() - timeTotal_beg;
	cout << "------------------------------" << endl;
	cout << left << setprecision(6) <<  setiosflags(ios::fixed);
	cout << setw(10) << "Category" << setw(10) << "Number" << setw(10) << "Size(MB)" << endl;
	cout << "------------------------------" << endl;
	cout << setw(10) << "NALU" << setw(10) << NaluCount << setw(10) << (float)NaluTotalSize/1024/1024 << endl;
	cout << setw(10) << "P" << setw(10) << pCount << setw(10) << (float)pSize/1024/1024 << endl;
	cout << setw(10) << "B" << setw(10) << bCount << setw(10) << (float)bSize/1024/1024 << endl;
	cout << setw(10) << "I" << setw(10) << iCount << setw(10) << (float)iSize/1024/1024 << endl;
	cout << setw(10) << "SI" << setw(10) << siCount << setw(10) << (float)siSize/1024/1024 << endl;
	cout << setw(10) << "SP" << setw(10) << spCount << setw(10) << (float)spSize/1024/1024 << endl;
	cout << setw(10) << "SPS" << setw(10) << spsCount << setw(10) << (float)spsSize/1024/1024 << endl;
	cout << setw(10) << "PPS" << setw(10) << ppsCount << setw(10) << (float)ppsSize/1024/1024 << endl;
	cout << setw(10) << "SEI" << setw(10) << seiCount << setw(10) << (float)seiSize/1024/1024 << endl;
	cout << setw(10) << "AUD" << setw(10) << audCount << setw(10) << (float)audSize/1024/1024 << endl;
	cout << setw(10) << "SLICE" << setw(10) << slicCount << setw(10) << (float)slicSize/1024/1024 << endl;
	cout << setw(10) << "IDR" << setw(10) << idrCount << setw(10) << (float)idrSize/1024/1024 << endl;
	cout << setw(10) << "AUX" << setw(10) << auxCount << setw(10) << (float)auxSize/1024/1024 << endl;
	cout << setw(10) << "DPA" << setw(10) << dpaCount << setw(10) << (float)dpaSize/1024/1024 << endl;
	cout << setw(10) << "DPB" << setw(10) << dpbCount << setw(10) << (float)dpbSize/1024/1024 << endl;
	cout << setw(10) << "DPC" << setw(10) << dpcCount << setw(10) << (float)dpcSize/1024/1024 << endl;
	cout << "总耗时: " << timeTotal_diff << " ms" << endl;
	cout << "------------------------------" << endl;
	h264Analysis.CloseFile();
	
	pCount = 0, bCount = 0, iCount = 0, siCount = 0, spCount = 0, spsCount = 0, ppsCount = 0, seiCount = 0, audCount = 0, slicCount = 0, idrCount = 0, auxCount = 0, dpaCount = 0, dpbCount = 0, dpcCount = 0;;
	pSize = 0, bSize = 0, iSize = 0, siSize = 0, spSize = 0, spsSize = 0, ppsSize = 0, seiSize = 0, audSize = 0, slicSize = 0, idrSize = 0, auxSize = 0, dpaSize = 0, dpbSize = 0, dpcSize = 0;;
	NaluCount = 0;
	NaluSize = 0;
	NaluTotalSize = 0;
	
	cout << "------------------------------------------" << endl;
	cout << left << setprecision(6) <<  setiosflags(ios::fixed);
	cout << setw(10) << "Category" << setw(10) << "Number" << setw(12) << "Size(MB)" << setw(10) << "Time(ms)" << endl;
	cout << "------------------------------------------" << endl;

	DWORD time_beg = GetTickCount();
	timeTotal_beg = time_beg;
	if (h264Analysis.GetOpenFile(g_fileNameStr.c_str()) == NULL)
		cout << "Cannot open file " + g_fileNameStr + " !" << endl;
	while (h264Analysis.NextNalu(NULL, &NaluSize) == H264Analysis::Success)
	{
		NaluTotalSize += NaluSize;
		NaluCount++;
	}
	h264Analysis.CloseFile();
	DWORD time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "NALU" << setw(10) << NaluCount << setw(12) << (float)NaluTotalSize/1024/1024 << setw(10) << time_diff << endl;
	
	time_beg = GetTickCount();
	if (h264Analysis.GetOpenFile(g_fileNameStr.c_str()) == NULL)
		cout << "Cannot open file " + g_fileNameStr + " !" << endl;
	while (h264Analysis.NextPnalu(NULL,&NaluSize) == H264Analysis::Success)
	{
		pSize += NaluSize;
		pCount++;
	}
	h264Analysis.CloseFile();
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "P" << setw(10) << pCount << setw(12) << (float)pSize/1024/1024 << setw(10) << time_diff << endl;
	
	time_beg = GetTickCount();
	if (h264Analysis.GetOpenFile(g_fileNameStr.c_str()) == NULL)
		cout << "Cannot open file " + g_fileNameStr + " !" << endl;
	while (h264Analysis.NextBnalu(NULL,&NaluSize) == H264Analysis::Success)
	{
		bSize += NaluSize;
		bCount++;
	}
	h264Analysis.CloseFile();
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "B" << setw(10) << bCount << setw(12) << (float)bSize/1024/1024 << setw(10) << time_diff << endl;
	
	time_beg = GetTickCount();
	if (h264Analysis.GetOpenFile(g_fileNameStr.c_str()) == NULL)
		cout << "Cannot open file " + g_fileNameStr + " !" << endl;
	while (h264Analysis.NextInalu(NULL,&NaluSize) == H264Analysis::Success)
	{
		iSize += NaluSize;
		iCount++;
	}
	h264Analysis.CloseFile();
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "I" << setw(10) << iCount << setw(12) << (float)iSize/1024/1024 << setw(10) << time_diff << endl;
	
	time_beg = GetTickCount();
	if (h264Analysis.GetOpenFile(g_fileNameStr.c_str()) == NULL)
		cout << "Cannot open file " + g_fileNameStr + " !" << endl;
	while (h264Analysis.NextSiNalu(NULL,&NaluSize) == H264Analysis::Success)
	{
		siSize += NaluSize;
		siCount++;
	}
	h264Analysis.CloseFile();
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "SI" << setw(10) << siCount << setw(12) << (float)siSize/1024/1024 << setw(10) << time_diff << endl;
	
	time_beg = GetTickCount();
	if (h264Analysis.GetOpenFile(g_fileNameStr.c_str()) == NULL)
		cout << "Cannot open file " + g_fileNameStr + " !" << endl;
	while (h264Analysis.NextSpNalu(NULL,&NaluSize) == H264Analysis::Success)
	{
		spSize += NaluSize;
		spCount++;
	}
	h264Analysis.CloseFile();
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "SP" << setw(10) << spCount << setw(12) << (float)spSize/1024/1024 << setw(10) << time_diff << endl;
	
	time_beg = GetTickCount();
	if (h264Analysis.GetOpenFile(g_fileNameStr.c_str()) == NULL)
		cout << "Cannot open file " + g_fileNameStr + " !" << endl;
	while (h264Analysis.NextSpsNalu(NULL,&NaluSize) == H264Analysis::Success)
	{
		spsSize += NaluSize;
		spsCount++;
	}
	h264Analysis.CloseFile();
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "SPS" << setw(10) << spsCount << setw(12) << (float)spsSize/1024/1024 << setw(10) << time_diff << endl;
	
	time_beg = GetTickCount();
	if (h264Analysis.GetOpenFile(g_fileNameStr.c_str()) == NULL)
		cout << "Cannot open file " + g_fileNameStr + " !" << endl;
	while (h264Analysis.NextPpsNalu(NULL,&NaluSize) == H264Analysis::Success)
	{
		ppsSize += NaluSize;
		ppsCount++;
	}
	h264Analysis.CloseFile();
	time_diff = GetTickCount() - time_beg;
	timeTotal_diff = GetTickCount() - timeTotal_beg;
	cout << setw(10) << "PPS" << setw(10) << ppsCount << setw(12) << (float)ppsSize/1024/1024 << setw(10) << time_diff << endl;
	cout << "总耗时: " << timeTotal_diff << " ms" << endl;
// 	cout << setw(10) << "SEI" << setw(10) << seiCount << setw(10) << (float)seiSize/1024/1024 << endl;
// 	cout << setw(10) << "AUD" << setw(10) << audCount << setw(10) << (float)audSize/1024/1024 << endl;
	cout << "------------------------------------------" << endl;	
}