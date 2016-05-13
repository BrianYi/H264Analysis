#include "H264Analysis.h"

const string g_fileNameStr = "../movie/264.h264";

int main()
{
	H264Analysis h264Analysis;
	h264Analysis.getOpenFile(g_fileNameStr);
	int n;
	while (cin >> n)
	{
		UINT32 tmp = h264Analysis.readNextBits(n);
		cout << tmp << endl;
	}
	h264Analysis.closeFile();
	return 0;
}