/**
 * 位串处理
 */
/**
 * 函数名: 
 * 描述: 字串颠倒处理(大小端处理)
 * 参数: 
 * 返回值: 
 * 日期: 2016/05/13
 * 作者: YJZ
 * 修改记录: 
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
 * 按字节对齐返回字节数
 */
int bits_get_byte_num(int n)
{
	return (n % 8 ? n/8+1 : n/8);
}