/**
 * λ������
 */
/**
 * ������: 
 * ����: �ִ��ߵ�����(��С�˴���)
 * ����: 
 * ����ֵ: 
 * ����: 2016/05/13
 * ����: YJZ
 * �޸ļ�¼: 
 */
void bytes_updown(char* beg, int n)
{
	char tmp;
	for (int i = 0; i < n/2; i++)
	{
		tmp = beg[i];
		beg[i] = beg[n-i-1];
		beg[n-i-1] = tmp;
	}
}

/**
 * ���ֽڶ��뷵���ֽ���
 */
int bits_get_byte_num(int n)
{
	return (n % 8 ? n/8+1 : n/8);
}