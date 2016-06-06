#include "H264Analysis/H264Analysis.h"
#include <string>
const string g_fileNameStr = "../movie/500MTest.h264";

void H264AnalysisDebug();	// 测试函数



int main()
{
	H264AnalysisDebug();

	return 0;
}


void H264AnalysisDebug()
{
	H264Analysis h264Analysis;
	ifstream& fileStream = h264Analysis.getOpenFile(g_fileNameStr);
	int pCount = 0, bCount = 0, iCount = 0, siCount = 0, spCount = 0, spsCount = 0, ppsCount = 0, seiCount = 0, audCount = 0, slicCount = 0, idrCount = 0, auxCount = 0, dpaCount = 0, dpbCount = 0, dpcCount = 0;
	int pSize = 0, bSize = 0, iSize = 0, siSize = 0, spSize = 0, spsSize = 0, ppsSize = 0, seiSize = 0, audSize = 0, slicSize = 0, idrSize = 0, auxSize = 0, dpaSize = 0, dpbSize = 0, dpcSize = 0;
	int NaluCount = 0;
	int NaluSize = 0;
	int NaluTotalSize = 0;

	//h264Analysis.skipTo(99);
	DWORD timeTotal_beg = GetTickCount();

	char *naluData = NULL;
	while (NaluSize = h264Analysis.nextNalu(&naluData))
	{
		NaluCount++;
		NaluTotalSize += NaluSize;
		int startCodeLen = h264Analysis.scLen(naluData);
		unsigned char nextByte = naluData[startCodeLen];
		UINT32 forbidden_zero_bit = B8_VAL_BASE_R(nextByte, 0, 1);
		UINT32 nal_ref_idc = B8_VAL_BASE_R(nextByte, 1, 2);
		NalUnitType nal_unit_type = /*B8_VAL_BASE_R(nextByte, 3, 5);*/h264Analysis.getNaluType(naluData);
		UINT32 first_mb_in_slice = 0;
		SliceType slice_type = /*0;*/h264Analysis.getSliceType(naluData, NaluSize);
		UINT32 pic_parameter_set_id = 0;
		unsigned int egcDataPos = startCodeLen + 1;
		unsigned int egcDataLen = NaluSize - egcDataPos;
		unsigned int egcSize = 0;
		int len = 0;
		switch (nal_unit_type)
		{
		case NAL_SLICE:
		case NAL_IDR_SLICE:
		case NAL_AUXILIARY_SLICE:
			if (nal_unit_type == NAL_SLICE)
			{
				slicCount++;
				slicSize += NaluSize;
			}
			else if (nal_unit_type == NAL_IDR_SLICE)
			{
				idrCount++;
				idrSize += NaluSize;
			}
			else if (nal_unit_type == NAL_AUXILIARY_SLICE)
			{
				auxCount++;
				auxSize += NaluSize;
			}

			switch (slice_type)
			{
			case SLICE_TYPE_P1:
			case SLICE_TYPE_P2:
				pCount++;
				pSize += NaluSize;
				break;
			case SLICE_TYPE_B1:
			case SLICE_TYPE_B2:
				bCount++;
				bSize += NaluSize;
				break;
			case SLICE_TYPE_I1:
			case SLICE_TYPE_I2:
				iCount++;
				iSize += NaluSize;
				break;
			case SLICE_TYPE_SP1:
			case SLICE_TYPE_SP2:
				spCount++;
				spsSize += NaluSize;
				break;
			case SLICE_TYPE_SI1:
			case SLICE_TYPE_SI2:
				siCount++;
				siSize += NaluSize;
				break;
			default:
				break;
			}
			break;
		case NAL_DPA:
			dpaCount++;
			dpaSize += NaluSize;
			break;
		case NAL_DPB:
			dpbCount++;
			dpbSize += NaluSize;
			break;
		case NAL_DPC:
			dpcCount++;
			dpcSize += NaluSize;
			break;
		case NAL_SEI:
			seiCount++;
			seiSize += NaluSize;
			break;
		case NAL_SPS:
			spsCount++;
			spsSize += NaluSize;
			break;
		case NAL_PPS:
			ppsCount++;
			ppsSize += NaluSize;
			break;
		case NAL_AUD:
			audCount++;
			audSize += NaluSize;
			break;
		case NAL_END_SEQUENCE:
			break;
		case NAL_END_STREAM:
			break;
		case NAL_FILLER_DATA:
			break;
		case NAL_SPS_EXT:
			break;
		default:
			break;
		}

		delete [] naluData;
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

	
	pCount = 0, bCount = 0, iCount = 0, siCount = 0, spCount = 0, spsCount = 0, ppsCount = 0, seiCount = 0, audCount = 0, slicCount = 0, idrCount = 0, auxCount = 0, dpaCount = 0, dpbCount = 0, dpcCount = 0;;
	pSize = 0, bSize = 0, iSize = 0, siSize = 0, spSize = 0, spsSize = 0, ppsSize = 0, seiSize = 0, audSize = 0, slicSize = 0, idrSize = 0, auxSize = 0, dpaSize = 0, dpbSize = 0, dpcSize = 0;;
	NaluCount = 0;
	NaluSize = 0;
	NaluTotalSize = 0;
	
	cout << "------------------------------------------" << endl;
	cout << left << setprecision(6) <<  setiosflags(ios::fixed);
	cout << setw(10) << "Category" << setw(10) << "Number" << setw(12) << "Size(MB)" << setw(10) << "Time(ms)" << endl;
	cout << "------------------------------------------" << endl;

	fileStream.seekg(ios::beg);
	h264Analysis.m_binPos = 0;
	h264Analysis.m_lastByte = 0;
	DWORD time_beg = GetTickCount();
	timeTotal_beg = time_beg;
	while (NaluSize = h264Analysis.nextNalu())
	{
		NaluTotalSize += NaluSize;
		NaluCount++;
	}
	DWORD time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "NALU" << setw(10) << NaluCount << setw(12) << (float)NaluTotalSize/1024/1024 << setw(10) << time_diff << endl;
	
	fileStream.seekg(ios::beg);
	h264Analysis.m_binPos = 0;
	h264Analysis.m_lastByte = 0;
	time_beg = GetTickCount();
	while (NaluSize = h264Analysis.next_P_Nalu())
	{
		pSize += NaluSize;
		pCount++;
	}
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "P" << setw(10) << pCount << setw(12) << (float)pSize/1024/1024 << setw(10) << time_diff << endl;
	
	fileStream.seekg(ios::beg);
	h264Analysis.m_binPos = 0;
	h264Analysis.m_lastByte = 0;
	time_beg = GetTickCount();
	while (NaluSize = h264Analysis.next_B_Nalu())
	{
		bSize += NaluSize;
		bCount++;
	}
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "B" << setw(10) << bCount << setw(12) << (float)bSize/1024/1024 << setw(10) << time_diff << endl;
	
	fileStream.seekg(ios::beg);
	h264Analysis.m_binPos = 0;
	h264Analysis.m_lastByte = 0;
	time_beg = GetTickCount();
	while (NaluSize = h264Analysis.next_I_Nalu())
	{
		iSize += NaluSize;
		iCount++;
	}
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "I" << setw(10) << iCount << setw(12) << (float)iSize/1024/1024 << setw(10) << time_diff << endl;
	
	fileStream.seekg(ios::beg);
	h264Analysis.m_binPos = 0;
	h264Analysis.m_lastByte = 0;
	time_beg = GetTickCount();
	while (NaluSize = h264Analysis.next_SI_Nalu())
	{
		siSize += NaluSize;
		siCount++;
	}
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "SI" << setw(10) << siCount << setw(12) << (float)siSize/1024/1024 << setw(10) << time_diff << endl;
	
	fileStream.seekg(ios::beg);
	h264Analysis.m_binPos = 0;
	h264Analysis.m_lastByte = 0;
	time_beg = GetTickCount();
	while (NaluSize = h264Analysis.next_SP_Nalu())
	{
		spSize += NaluSize;
		spCount++;
	}
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "SP" << setw(10) << spCount << setw(12) << (float)spSize/1024/1024 << setw(10) << time_diff << endl;
	
	fileStream.seekg(ios::beg);	
	h264Analysis.m_binPos = 0;
	h264Analysis.m_lastByte = 0;
	time_beg = GetTickCount();
	while (NaluSize = h264Analysis.next_SPS_Nalu())
	{
		spsSize += NaluSize;
		spsCount++;
	}
	time_diff = GetTickCount() - time_beg;
	cout << setw(10) << "SPS" << setw(10) << spsCount << setw(12) << (float)spsSize/1024/1024 << setw(10) << time_diff << endl;
	
	fileStream.seekg(ios::beg);	
	h264Analysis.m_binPos = 0;
	h264Analysis.m_lastByte = 0;
	time_beg = GetTickCount();
	while (NaluSize = h264Analysis.next_PPS_Nalu())
	{
		ppsSize += NaluSize;
		ppsCount++;
	}
	time_diff = GetTickCount() - time_beg;
	timeTotal_diff = GetTickCount() - timeTotal_beg;
	cout << setw(10) << "PPS" << setw(10) << ppsCount << setw(12) << (float)ppsSize/1024/1024 << setw(10) << time_diff << endl;
	cout << "总耗时: " << timeTotal_diff << " ms" << endl;
// 	cout << setw(10) << "SEI" << setw(10) << seiCount << setw(10) << (float)seiSize/1024/1024 << endl;
// 	cout << setw(10) << "AUD" << setw(10) << audCount << setw(10) << (float)audSize/1024/1024 << endl;
	cout << "------------------------------------------" << endl;
	h264Analysis.closeFile();
	
}