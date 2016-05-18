#include "H264Analysis.h"

const string g_fileNameStr = "../movie/264.h264";

void H264AnalysisDebug();	// ²âÊÔº¯Êý

int main()
{
	return 0;
}


void H264AnalysisDebug()
{
	H264Analysis h264Analysis;
	h264Analysis.getOpenFile(g_fileNameStr);
	int pCount = 0, bCount = 0, iCount = 0, siCount = 0, spCount = 0, spsCount = 0, ppsCount = 0, seiCount = 0, audCount = 0;
	int pSize = 0, bSize = 0, iSize = 0, siSize = 0, spSize = 0, spsSize = 0, ppsSize = 0, seiSize = 0, audSize = 0;
	int NaluCount = 0;
	int NaluSize = 0;
	int NaluTotalSize = 0;
	while (h264Analysis.nextNalu())
	{
		unsigned char nextByte = 0;
		if (!(NaluSize = h264Analysis.readNaluData())|| 
			!h264Analysis.readNextByte((char*)&nextByte))
			break;
		NaluCount++;
		NaluTotalSize += NaluSize;
		UINT32 forbidden_zero_bit = B8_VAL_BASE_R(nextByte, 0, 1);
		UINT32 nal_ref_idc = B8_VAL_BASE_R(nextByte, 1, 2);
		UINT32 nal_unit_type = B8_VAL_BASE_R(nextByte, 3, 5);
		UINT32 first_mb_in_slice = 0;
		UINT32 slice_type = 0;
		UINT32 pic_parameter_set_id = 0;
		int len = 0;
		switch (nal_unit_type)
		{
		case NAL_SLICE:
		case NAL_IDR_SLICE:
		case NAL_AUXILIARY_SLICE:
			if (!h264Analysis.ueDecode(&first_mb_in_slice) || 
				!h264Analysis.ueDecode(&slice_type))
				break;

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
			break;
		case NAL_DPB:
			break;
		case NAL_DPC:
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
	}
	cout << "------------------------------" << endl;
	cout << left << setprecision(4);
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
	cout << "------------------------------" << endl;
	h264Analysis.closeFile();
}