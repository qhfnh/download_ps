#include <windows.h>
#include <wininet.h>
#include <atlconv.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <tchar.h>
#include "PsPacket.h"
#include <atlbase.h>
#define _CRT_SECURE_NO_WARNINGS

constexpr auto MAXBLOCKSIZE = 1024 * 10 +100;
#define EQUITPREFIX(arr_name,index) (arr_name[index] == 0 && arr_name[index + 1] == 0 && arr_name[index + 2] == 1)
#define EQUIT00(arr_name,index) (arr_name[index] == 0 && arr_name[index + 1] == 0 && arr_name[index + 2] == 0)
#define EQUITBIT(arr_name,index,t) arr_name[index] == 0 && arr_name[index + 1] == 0 && arr_name[index + 2] == 1 && arr_name[index + 3] == t
#define SUMLEN(arr_name,index)  arr_name[index] << 8 | arr_name[index + 1]
//#define GETBIT(arr_name,index,start,len) (arr_name[index] & (1 <<)) >> start
#pragma comment( lib, "wininet.lib" )
 
/**
* arr_name  为数组的名字
* index 或index_xxx 总是要解析的位置，并且是arr_name的绝对位置
* 
* 
*/

//struct AppParam
//{
//	char  srcFile[256];
//	char  dstFile[256];
//};
//
//AppParam g_appParam;
//
//
//void processParams(int argc, char* argv[], AppParam* param)
//{
//	int paramNums = 0;
//
//	memset(param, 0, sizeof(AppParam));
//
//	for (int i = 1; i < argc; i++)
//	{
//
//		if ((_stricmp(argv[i], "-s") == 0) && (i < argc - 1))
//		{
//			//paramNums = _stscanf(argv[++i], _T("%s"), param->srcFile);
//			if (paramNums != 1)
//				continue;
//		}
//
//		else if ((_stricmp(argv[i], "-d") == 0) && (i < argc - 1))
//		{
//			//paramNums = _stscanf(argv[++i], _T("%s"), param->dstFile);
//			if (paramNums != 1)
//				continue;
//
//		}
//	}
//}
//
//
//int GetH246FromPs(const char* szPsFile, const char* szEsFile, int* h264length)
//{
//	PSPacket psDemux;
//	//psDemux.SetRealTime(true); //如果输入的是实时流，需要设置这个开关
//
//	psDemux.m_ProgramMap.clear();
//
//	FILE* poutfile = NULL;
//	FILE* pinfile = NULL;
//
//	if (pinfile == NULL)
//	{
//		if (NULL == (pinfile = fopen(szPsFile, "rb")))
//		{
//			printf("Error: Open inputfilename file error\n");
//			return -1;
//		}
//	}
//
//	if (poutfile == NULL)
//	{
//		if (NULL == (poutfile = fopen(szEsFile, "wb")))
//		{
//			printf("Error: Open outputfilename file error\n");
//			return -1;
//		}
//	}
//
//	fseek(pinfile, 0, SEEK_END);
//	int  file_len = ftell(pinfile);
//
//	fseek(pinfile, 0, SEEK_SET);
//
//	char* buffer = NULL;
//	int length = 0;
//
//	const int read_chunk_size = 64 * 1024;
//
//	buffer = new char[read_chunk_size];
//	memset(buffer, 0, read_chunk_size);
//
//	int nLeftLen = file_len;
//
//	int nReadLen = 0;
//	int nVFrameNum = 0;
//	int nAFrameNum = 0;
//
//	*h264length = 0;
//
//	DWORD dwStartTick = GetTickCount();
//
//	while (nLeftLen > 0)
//	{
//		nReadLen = min(read_chunk_size, nLeftLen); //每次从文件读64K
//
//		fread(buffer, 1, nReadLen, pinfile); //将文件数据读进Buffer
//
//		psDemux.PSWrite(buffer, nReadLen);
//
//		int nOutputVideoEsSize = 0;
//
//		while (1)
//		{
//
//			naked_tuple es_packet = psDemux.naked_payload();
//			if (!es_packet.bContainData || es_packet.nDataLen == 0)
//				break;
//
//			if (es_packet.nStreamID == 0xE0)
//			{
//				int fwrite_number = fwrite(es_packet.pDataPtr, 1, es_packet.nDataLen, poutfile);
//				*h264length += es_packet.nDataLen;
//				nOutputVideoEsSize += es_packet.nDataLen;
//
//				printf("[%x]VFrameNum: %d, EsLen: %d, pts: %lld \n", es_packet.nStreamID, nVFrameNum++, es_packet.nDataLen, es_packet.ptsTime);
//			}
//			else
//			{
//				//printf("[%x]AFrameNum: %d, EsLen: %d \n", es_packet.nStreamID, nAFrameNum++, es_packet.nDataLen);
//			}
//		}
//
//		//ATLTRACE("WritePos: %d, ReadPos: %d, OutputEsSize: %d \n",  psDemux.GetWritePos(), psDemux.GetReadPos(), nOutputVideoEsSize);
//
//		nLeftLen -= nReadLen;
//	}
//	///
//
//	int nSize = psDemux.m_ProgramMap.size();
//	for (int i = 0; i < nSize; i++)
//	{
//		program_map_info& map_info = psDemux.m_ProgramMap[i];
//		printf("stream_id: %x, stream_type: %x \n", map_info.elementary_stream_id, map_info.stream_type);
//	}
//	printf("Time cost %ld secs \n", (GetTickCount() - dwStartTick) / 1000);
//
//	fclose(pinfile);
//	fclose(poutfile);
//
//	delete buffer;
//
//	return *h264length;
//}
//
// 
typedef enum 
{
	ps_packet_head_loading,
	ps_data_loading,
	ps_continue,
	ps_error, 
}header;

typedef enum {
	NALU_TYPE_SLICE = 1,
	NALU_TYPE_DPA = 2,
	NALU_TYPE_DPB = 3,
	NALU_TYPE_DPC = 4,
	NALU_TYPE_IDR = 5,
	NALU_TYPE_SEI = 6,
	NALU_TYPE_SPS = 7,
	NALU_TYPE_PPS = 8,
	NALU_TYPE_AUD = 9,
	NALU_TYPE_EOSEQ = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL = 12,
} NaluType;

void saveH264(const char *arr_name,int index,int arr_len,int len) // 防止越界
{
	if (index >= arr_len)
	{
		return ;
	}
	ofstream fd;
	fd.open("ps.h264", ios::out | ios::binary | ios::app);
	if (fd.is_open())
	{
		fd.write(arr_name + index, len);
		fd.close();
	}
}

header parase_h264(unsigned char* arr_name, int index,int len)
{
	if (EQUITPREFIX(arr_name,index))
	{
		index += 3;
	}
	else if(EQUIT00(arr_name,index) && (arr_name[index +3] == 1))
	{
		index += 4;
	}
	else 
	{
		return ps_error;
	}
	int nalu_type = arr_name[index] & 0x1f;
	index++;

	switch (nalu_type)
	{
		case NALU_TYPE_SLICE:
		{
			break;
		}
		case NALU_TYPE_DPA:
		{
			break;
		}
		case NALU_TYPE_DPB:
		{
			break;
		}
		case NALU_TYPE_DPC:
		{
			break;
		}
		case NALU_TYPE_IDR:
		{
			break;
		}
		case NALU_TYPE_SEI:
		{
			break;
		}
		case NALU_TYPE_SPS:
		{
			int profile_ide = arr_name[index ++];
			int constraint_set0_flag = (arr_name[index]  & 0x80) >> 7;
			int constraint_set1_flag = (arr_name[index] & 0x40) >> 6;
			int constraint_set2_flag = (arr_name[index] & 0x20) >> 5;
			int constraint_set3_flag = (arr_name[index] & 0x10) >> 4;
			int constraint_set4_flag = (arr_name[index] & 0x08) >> 3;
			int constraint_set5_flag = (arr_name[index] & 0x04) >> 2;
			int reserved_zero_2bit	=  arr_name[index] & 0x03;
			int level_idc = arr_name[++index];


			break;
		}
		case NALU_TYPE_PPS:
		{
			break;
		}
		case NALU_TYPE_AUD:
		{
			break;
		}
		case NALU_TYPE_EOSEQ:
		{
			break;
		}
		case NALU_TYPE_EOSTREAM:
		{
			break;
		}
		case NALU_TYPE_FILL: 
		{
			break;
		}
	}
}


header parase_pes(unsigned char* arr_name, int& index, int len, unsigned int& reserve, int& remain_len)
{
	int start_index = index;
	int PES_packet_length = SUMLEN(arr_name, index);
	int packet_end = index + PES_packet_length + 2;//这个包的结尾

	//int PES_header_data_length = arr_name[index + 4];

	//.........
	index += 2;
	int fix_biat_10 = (arr_name[index] & 0xc0) >> 6;
	int packet_inde = index;
	std::cout << fix_biat_10 << std::endl;
	if (2 == (arr_name[index] & 0xc0) >> 6) // '10' fix
	{
		int PES_scrambing_control = (arr_name[index] & 0x30) >> 4;
		int PES_priority = (arr_name[index] & 0x08) >> 3;
		int data_alignment_indicator = (arr_name[index] & 0x04) >> 2;
		int copyright = (arr_name[index] & 0x02) >> 1;
		int original_or_copy = arr_name[index] & 0x01;

		int PTS_DES_flags = (arr_name[index + 1] & 0xc0) >> 6;
		int ESCR_flag = (arr_name[index + 1] & 0x20) >> 5;
		int ES_rate_flag = (arr_name[index + 1] & 0x10) >> 4;
		int DSM_trick_mode_flag = (arr_name[index + 1] & 0x08) >> 3;
		int additional_copy_info_flag = (arr_name[index + 1] & 0x04) >> 2;
		int PES_CRC_flag = (arr_name[index + 1] & 0x02) >> 1;
		int PES_extrnsion_flag = arr_name[index + 1] & 0x01;
		int PES_header_data_length = arr_name[index + 2];
		index += 3;

		if (PTS_DES_flags == 3)//PTS_DES_flags == '11'
		{
			std::cout << PTS_DES_flags << std::endl;
			//PTS
			int fix_0011 = (arr_name[index] & 0xf0) >> 4;
			unsigned int PTS32_30 = (arr_name[index] & 0x0e) >> 1;
			int marker_bit = arr_name[index] & 0x01;
			unsigned int PTS29_15 = (arr_name[index + 1] << 7) | ((arr_name[index + 2] & 0xfe) >> 1);
			int marter_bit1 = arr_name[index + 2] & 0x01;
			unsigned int PTS14_0 = (arr_name[index + 3] << 7) | ((arr_name[index + 4] & 0xfe) >> 1);
			int marter_bit2 = arr_name[index + 4] & 0x01;

			unsigned long long pts = (PTS32_30 << 30) + (PTS29_15 << 15) + PTS14_0;
			index += 5;
			//DTS
			int fix_0001 = (arr_name[index] & 0xf0) >> 4;
			int DTS32_30 = (arr_name[index] & 0x0e) >> 1;
			int marker_bit_d = arr_name[index] & 0x01;
			int DTS29_15 = (arr_name[index + 1] << 7) | ((arr_name[index + 2] & 0xfe) >> 1);
			int marter_bit1_d = arr_name[index + 2] & 0x01;
			int DTS14_0 = (arr_name[index + 3] << 7) | ((arr_name[index + 4] & 0xfe) >> 1);
			int marter_bit2_d = arr_name[index + 4] & 0x01;
			index += 5;
			std::cout <<"index:" << index << ":0xe0:" ;

		}
		else if (PTS_DES_flags == 2)
		{

			std::cout << PTS_DES_flags << std::endl;
			int fix_0011 = (arr_name[index] & 0xf0) >> 4;
			int PTS32_30 = (arr_name[index] & 0x0e) >> 1;
			int marker_bit = arr_name[index] & 0x01;
			int PTS29_15 = (arr_name[index + 1] << 7) | ((arr_name[index + 2] & 0xfe) >> 1);
			int marter_bit1 = arr_name[index + 2] & 0x01;
			int PTS14_0 = (arr_name[index + 3] << 7) | ((arr_name[index + 4] & 0xfe) >> 1);
			int marter_bit2 = arr_name[index + 4] & 0x01;
			index += 5;
			std::cout<< "index:" << index << ":0xc0:";

		}
		else 
		{
			remain_len = len - start_index + 4;
			return header::ps_packet_head_loading;
		}




		if (index > len)//解析头时 缓存数据不够
		{
			std::cout << "::index:" << index <<":index > len" <<std::endl;
			remain_len = len - start_index + 4;
			return header::ps_packet_head_loading;
		}
		else if (packet_end >= len || index == len) //解析数据  数据时不够
		{
			std::cout << "::index:" << index << ":index == len" << std::endl;
			remain_len = 0;
			reserve = packet_end - len;
			//index_globe += packet_end - 1;
			saveH264(reinterpret_cast<const char*>(arr_name), index, len, len - index);
			return header::ps_data_loading;
		}
		else //正常解析
		{
			remain_len = 0;
			saveH264(reinterpret_cast<const char*>(arr_name), index, len, PES_packet_length - PES_header_data_length - 3);
			index = packet_end;
			std::cout << (int)arr_name[index] << ":" << (int)arr_name[index + 1] << ":" << (int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << std::endl;
			return header::ps_continue;
		}

	}
	else 
	{
		remain_len = len - start_index + 4;
		return header::ps_packet_head_loading;
	}

}

header ps_parase(unsigned char* arr_name, int len ,int& remain_len)
{
	int a = (int)arr_name[len];
	cout << "a:" << a << endl;
	//static unsigned long long index_globe = 0; //全局文件索引  
	static unsigned int reserve = 0; //还有多少字节没有保存
	int index = 0;//当前文件索引
	if (reserve >= len) //处理数据中断
	{
		saveH264(reinterpret_cast<const char*>(arr_name), index, len, len);
		reserve -= len;
		return ps_data_loading;
	}
	else if (reserve > 0 && reserve < len)
	{
		saveH264(reinterpret_cast<const char*>(arr_name), index, len, reserve);
		index += reserve;
		reserve = 0;
	}


	while (EQUITPREFIX(arr_name, index)|| EQUIT00(arr_name,index)) //ps 包头
	{
		switch (arr_name[index + 3])
		{
		case 0xba:  // ps
		{
			int start_index = index;
			int stuff_byte = arr_name[index + 13] & 0x07;
			index += stuff_byte + 14;
			if (index > len)
			{
				remain_len = len - start_index; //包含 +4 了
				std::cout << "0xba:" <<"index:"<<index <<": index > len "<<std::endl;
				return ps_packet_head_loading;
			}
			else if (index == len)
			{
				remain_len = 0;
				std::cout << "0xba:" << "index:" << index << ": index == len " << std::endl;
				return ps_continue;
			}
			cout << "0xba:" << (int)arr_name[index] << ":" << (int)arr_name[index + 1] << ":" << (int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << endl;
			break;
		}
		case 0xbb: //sys
		{
			int start_index = index;
			int sys_header_length = SUMLEN(arr_name, index + 4);
			index = index + 6 + sys_header_length;
			if (index > len)
			{
				std::cout << "0xbb:" << "index:" << index << ": index > len " << std::endl;
				remain_len = len - start_index ;
				return ps_packet_head_loading;
			}
			else if (index == len)
			{
				std::cout << "0xbb:" << "index:" << index << ": index == len " << std::endl;
				remain_len = 0;
				return ps_continue;
			}
			cout << "0xbb:" << (int)arr_name[index] << ":" << (int)arr_name[index + 1] << ":" << (int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << endl;
			break;
		}
		case 0xbc: //psm
		{
			int start_index1 = index;
			int program_stream_map_length = SUMLEN(arr_name, index + 4);
			int start_index = index + 8;
			//直接跳过 解析里面的内容时 在break上边 index =start_index;
			index += program_stream_map_length + 6;

			//解析里面内容
			{

				int elementary_stream_info_length = SUMLEN(arr_name, start_index);
				start_index += 2;
				int elementary_stream_map_length = SUMLEN(arr_name, start_index);
				start_index = start_index + 2;
				for (int i = 0; i < elementary_stream_map_length / 4; i++)
				{
					int stream_type = arr_name[index];
					int elementary_stream_id = arr_name[index + 1];
					int elementary_stream_info_length = SUMLEN(arr_name, start_index + 2);
					start_index = start_index + elementary_stream_info_length + 4;
				}
				start_index += 4; //CRC
			}
			if (index > len)
			{
				std::cout << "0xbc:" << "index:" << index << ":index > len " << std::endl;
				remain_len = len - start_index1;
				return ps_packet_head_loading;
			}
			else if (index == len)
			{
				std::cout << "0xbc:" << "index:" << index << ":index == len " << std::endl;
				remain_len = 0;
				return ps_continue;
			}
			cout << "0xbc" << (int)arr_name[index] << ":" << (int)arr_name[index + 1] << ":" << (int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << endl;
			break;
		}
		case 0xe0: //pse
		{
			index += 4;
			//int PES_packet_length = SUMLEN(arr_name, index);
			//int packet_end = index + PES_packet_length + 2;
			////.........
			//index += 2;
			//int fix_biat_10 = (arr_name[index] & 0xc0) >> 6;
			//int packet_inde = index;
			//cout << fix_biat_10 << endl;
			//if (2 == (arr_name[index] & 0xc0) >> 6) // '10' fix
			//{
			//	int PES_scrambing_control = (arr_name[index] & 0x30) >> 4;
			//	int PES_priority = (arr_name[index] & 0x08) >> 3;
			//	int data_alignment_indicator = (arr_name[index] & 0x04) >> 2;
			//	int copyright = (arr_name[index] & 0x02) >> 1;
			//	int original_or_copy = arr_name[index] & 0x01;
			//	int PTS_DES_flags = (arr_name[index + 1] & 0xc0) >> 6;
			//	int ESCR_flag = (arr_name[index + 1] & 0x20) >> 5;
			//	int ES_rate_flag = (arr_name[index + 1] & 0x10) >> 4;
			//	int DSM_trick_mode_flag = (arr_name[index + 1] & 0x08) >> 3;
			//	int additional_copy_info_flag = (arr_name[index + 1] & 0x04) >> 2;
			//	int PES_CRC_flag = (arr_name[index + 1] & 0x02) >> 1;
			//	int PES_extrnsion_flag = arr_name[index + 1] & 0x01;
			//	int PES_header_data_length = arr_name[index + 2];
			//	index += 3;
			//	if (PTS_DES_flags == 3)//PTS_DES_flags == '11'
			//	{
			//		cout << PTS_DES_flags << endl;
			//		//PTS
			//		int fix_0011 = (arr_name[index] & 0xf0) >> 4;
			//		int PTS32_30 = (arr_name[index] & 0x0e) >> 1;
			//		int marker_bit = arr_name[index] & 0x01;
			//		int PTS29_15 = (arr_name[index + 1] << 7) | ((arr_name[index + 2] & 0xfe) >> 1);
			//		int marter_bit1 = arr_name[index + 2] & 0x01;
			//		int PTS14_0 = (arr_name[index + 3] << 7) | ((arr_name[index + 4] & 0xfe) >> 1);
			//		int marter_bit2 = arr_name[index + 4] & 0x01;
			//		index += 5;
			//		//DTS
			//		int fix_0001 = (arr_name[index] & 0xf0) >> 4;
			//		int DTS32_30 = (arr_name[index] & 0x0e) >> 1;
			//		int marker_bit_d = arr_name[index] & 0x01;
			//		int DTS29_15 = (arr_name[index + 1] << 7) | ((arr_name[index + 2] & 0xfe) >> 1);
			//		int marter_bit1_d = arr_name[index + 2] & 0x01;
			//		int DTS14_0 = (arr_name[index + 3] << 7) | ((arr_name[index + 4] & 0xfe) >> 1);
			//		int marter_bit2_d = arr_name[index + 4] & 0x01;
			//		index += 5;
			//	}
			//	else if (PTS_DES_flags == 2)
			//	{
			//		cout << PTS_DES_flags << endl;
			//		int fix_0011 = (arr_name[index] & 0xf0) >> 4;
			//		int PTS32_30 = (arr_name[index] & 0x0e) >> 1;
			//		int marker_bit = arr_name[index] & 0x01;
			//		int PTS29_15 = (arr_name[index + 1] << 7) | ((arr_name[index + 2] & 0xfe) >> 1);
			//		int marter_bit1 = arr_name[index + 2] & 0x01;
			//		int PTS14_0 = (arr_name[index + 3] << 7) | ((arr_name[index + 4] & 0xfe) >> 1);
			//		int marter_bit2 = arr_name[index + 4] & 0x01;
			//		index += 5;
			//	}
			//	if (packet_end >= len)
			//	{
			//		reserve = packet_end - len;
			//		//index_globe += packet_end - 1;
			//		saveH264(reinterpret_cast<const char*>(arr_name), index, len, len - index);
			//		return pe_loading;
			//	}
			//	saveH264(reinterpret_cast<const char*>(arr_name), index, len, PES_packet_length - PES_header_data_length - 3);
			//	index = packet_end;
			//	cout << "0xe0:" << (int)arr_name[index] << ":" << (int)arr_name[index + 1] << ":" << (int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << endl;
			//	break;
			//}
			header sta = parase_pes(arr_name, index, len, reserve, remain_len);
			if (sta == ps_packet_head_loading || sta == ps_data_loading)
			{
				return sta;
			}

			break;
		}
		case 0xc0: //PSE
		{
			index += 4;
			//int PES_packet_length = SUMLEN(arr_name, index);
			//int packet_end = index + PES_packet_length +2;
			////int PES_header_data_length = arr_name[index + 4];
			////.........
			//index += 2;
			//int fix_biat_10 = (arr_name[index] & 0xc0) >> 6;
			//int packet_inde = index;
			//cout << fix_biat_10 << endl;
			//if (2 == (arr_name[index] & 0xc0) >> 6) // '10' fix
			//{
			//	int PES_scrambing_control = (arr_name[index] & 0x30) >> 4;
			//	int PES_priority = (arr_name[index] & 0x08) >> 3;
			//	int data_alignment_indicator = (arr_name[index] & 0x04) >> 2;
			//	int copyright = (arr_name[index] & 0x02) >> 1;
			//	int original_or_copy = arr_name[index] & 0x01;
			//	int PTS_DES_flags = (arr_name[index + 1] & 0xc0) >> 6;
			//	int ESCR_flag = (arr_name[index + 1] & 0x20) >> 5;
			//	int ES_rate_flag = (arr_name[index + 1] & 0x10) >> 4;
			//	int DSM_trick_mode_flag = (arr_name[index + 1] & 0x08) >> 3;
			//	int additional_copy_info_flag = (arr_name[index + 1] & 0x04) >> 2;
			//	int PES_CRC_flag = (arr_name[index + 1] & 0x02) >> 1;
			//	int PES_extrnsion_flag = arr_name[index + 1] & 0x01;
			//	int PES_header_data_length = arr_name[index + 2];
			//	index += 3;
			//	if (PTS_DES_flags == 3)//PTS_DES_flags == '11'
			//	{
			//		cout << PTS_DES_flags << endl;
			//		//PTS
			//		int fix_0011 = (arr_name[index] & 0xf0) >> 4;
			//		int PTS32_30 = (arr_name[index] & 0x0e) >> 1;
			//		int marker_bit = arr_name[index] & 0x01;
			//		int PTS29_15 = (arr_name[index + 1] << 7) | ((arr_name[index + 2] & 0xfe) >> 1);
			//		int marter_bit1 = arr_name[index + 2] & 0x01;
			//		int PTS14_0 = (arr_name[index + 3] << 7) | ((arr_name[index + 4] & 0xfe) >> 1);
			//		int marter_bit2 = arr_name[index + 4] & 0x01;
			//		index += 5;
			//		//DTS
			//		int fix_0001 = (arr_name[index] & 0xf0) >> 4;
			//		int DTS32_30 = (arr_name[index] & 0x0e) >> 1;
			//		int marker_bit_d = arr_name[index] & 0x01;
			//		int DTS29_15 = (arr_name[index + 1] << 7) | ((arr_name[index + 2] & 0xfe) >> 1);
			//		int marter_bit1_d = arr_name[index + 2] & 0x01;
			//		int DTS14_0 = (arr_name[index + 3] << 7) | ((arr_name[index + 4] & 0xfe) >> 1);
			//		int marter_bit2_d = arr_name[index + 4] & 0x01;
			//		index += 5;
			//	}
			//	else if (PTS_DES_flags == 2)
			//	{
			//		cout << PTS_DES_flags << endl;
			//		int fix_0011 = (arr_name[index] & 0xf0) >> 4;
			//		int PTS32_30 = (arr_name[index] & 0x0e) >> 1;
			//		int marker_bit = arr_name[index] & 0x01;
			//		int PTS29_15 = (arr_name[index + 1] << 7) | ((arr_name[index + 2] & 0xfe) >> 1);
			//		int marter_bit1 = arr_name[index + 2] & 0x01;
			//		int PTS14_0 = (arr_name[index + 3] << 7) | ((arr_name[index + 4] & 0xfe) >> 1);
			//		int marter_bit2 = arr_name[index + 4] & 0x01;
			//		index += 5;
			//	}
			//	if (packet_end >= len)
			//	{
			//		reserve = packet_end - len;
			//		saveH264(reinterpret_cast<const char*>(arr_name),index, len, len - index);
			//			return pe_loading;
			//		//index_globe += packet_end - 1;
			//	}
			//	saveH264(reinterpret_cast<const char*>(arr_name), index, len, PES_packet_length - PES_header_data_length - 3);
			//	index = packet_end;
			//	cout << "0xc0:" << (int)arr_name[index] << ":"<<(int)arr_name[index + 1]  << ":"<<(int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << endl;
			//	break;
			//}
			header sta = parase_pes(arr_name, index, len, reserve, remain_len);
			if (sta == ps_packet_head_loading || sta == ps_data_loading)
			{
				return sta;
			}
			break;
		}
		case 0x00:
		{
			int count = 0;
			std::cout << "index:"<< index << std::endl;
			std::cout << "0x00:" << (int)arr_name[index] << ":" << (int)arr_name[index + 1] << ":" << (int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << std::endl;
			while (index < len)
			{
				index++;
				count++;
			}
			remain_len = count;
			if (count > 4)
			{
				std::cout << "0xbc" << (int)arr_name[index] << ":" << (int)arr_name[index + 1] << ":" << (int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << std::endl;
				std::cout << "0x00: error" << std::endl;
				system("pause");
			}

			remain_len = count;
			return ps_packet_head_loading;

		}
		default:
			cout << "default:" << (int)arr_name[index] << ":" << (int)arr_name[index + 1] << ":" << (int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << endl;
			std::cout << "index:" << index << std::endl;
			std::cout << "123444444444" << std::endl;

		}
	
	}
	return ps_error;
}
//header ps_parase1(unsigned char *ps) 
//{
//	int index = 0;
//	int PES_end = 0;//PES结束的下一字节
//	if (EQUITBIT(ps,index,0xba) ) //ps 包头
//	{
//		int stuff_byte = ps[13] & 0x07;
//		index = stuff_byte + 14;
//		if (EQUITBIT(ps, index, 0xbb)) 
//		{
//			int sys_header_length = SUMLEN(ps,index + 4);
//			index = index + 6 + sys_header_length;
//			if (EQUITBIT(ps, index, 0xbc))
//			{
//				index = index + 8;
//				//int program_stream_map_length = ps[index] * 256 + ps[index + 1];
//				//index = index + prograam_stream_info_length + 2;
//				int elementary_stream_info_length = SUMLEN(ps, index);
//				index += 2;
//				int elementary_stream_map_length =  SUMLEN(ps, index);
//				index = index + 2;
//				for (int i = 0; i < elementary_stream_map_length / 4; i++)
//				{
//					int stream_type = ps[index];
//					int elementary_stream_id = ps[index + 1];
//					int elementary_stream_info_length = SUMLEN(ps, index + 2);
//					index = index + elementary_stream_info_length + 4;
//				}
//				index += 4;
//				//PES 视频 
//				if (EQUITBIT(ps, index, 0xe0) || EQUITBIT(ps, index, 0xc0))
//				{
//					index += 4;
//					int PES_packet_length = SUMLEN(ps,  index);
//					index += 2;
//					PES_end = index + PES_packet_length; //PES结束的下一字节
//					int fix_biat_10 = (ps[index] & 0xc0) >> 6;
//					int packet_inde = index;
//					cout << fix_biat_10 << endl;
//					if (  2 == (ps[index] & 0xc0) >> 6 ) // '10' fix
//					{
//						int PES_scrambing_control = (ps[index] & 0x30) >> 4;
//						int PES_priority = (ps[index] & 0x08) >> 3;
//						int data_alignment_indicator = (ps[index] & 0x04) >> 2;
//						int copyright = (ps[index] & 0x02) >> 1;
//						int original_or_copy = ps[index] & 0x01;
//
//						int PTS_DES_flags = (ps[index + 1] & 0xc0) >> 6;
//						int ESCR_flag = (ps[index + 1] & 0x20) >> 5;
//						int ES_rate_flag = (ps[index + 1] & 0x10) >> 4;
//						int DSM_trick_mode_flag = (ps[index + 1] & 0x08) >> 3;
//						int additional_copy_info_flag = (ps[index + 1] & 0x04) >> 2;
//						int PES_CRC_flag = (ps[index + 1] & 0x02) >> 1;
//						int PES_extrnsion_flag = ps[index + 1] & 0x01;
//						int PES_header_data_length = ps[index + 2];
//						index += 3;
//						if (PTS_DES_flags == 3)//PTS_DES_flags == '11'
//						{
//							cout << PTS_DES_flags << endl;
//							//PTS
//							int fix_0011 = (ps[index] & 0xf0) >> 4;
//							int PTS32_30 = (ps[index] & 0x0e) >> 1;
//							int marker_bit = ps[index] & 0x01;
//							int PTS29_15 = (ps[index + 1] << 7) | ((ps[index + 2] & 0xfe) >> 1);
//							int marter_bit1 = ps[index + 2] & 0x01;
//							int PTS14_0 =  (ps[index + 3] << 7) | ((ps[index + 4] & 0xfe) >> 1);
//							int marter_bit2 = ps[index + 4] & 0x01;
//							index += 5;
//							//DTS
//							int fix_0001 = (ps[index] & 0xf0) >> 4;
//							int DTS32_30 = (ps[index] & 0x0e) >> 1;
//							int marker_bit_d = ps[index] & 0x01;
//							int DTS29_15 = (ps[index + 1] << 7) | ((ps[index + 2] & 0xfe) >> 1);
//							int marter_bit1_d = ps[index + 2] & 0x01;
//							int DTS14_0 = (ps[index + 3] << 7) | ((ps[index + 4] & 0xfe) >> 1);
//							int marter_bit2_d = ps[index + 4] & 0x01;
//							index += 5;
//						}
//						else if (PTS_DES_flags == 2)
//						{
//							cout << PTS_DES_flags << endl;
//						}
//
//						saveH264(reinterpret_cast<const char* >(ps + index) , PES_packet_length - PES_header_data_length - 3);
//
//					} 
//
//				}
//			}
//			return header::sys_header;
//		}
//
//	}
//}


void download(const char* Url, const char* save_as)	/*将Url指向的地址的文件bai下载到save_as指向的本地文件*/
{
	byte Temp[MAXBLOCKSIZE];
	//byte save_data[100];
	memset(Temp,0, MAXBLOCKSIZE);
	//memset(save_data, 0, 100);
	ULONG Number = 1;
	int remain = 0;
	FILE* stream;
	USES_CONVERSION;
	HINTERNET hSession = InternetOpen(L"RookIE/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hSession != NULL)
	{
		HINTERNET handle2 = InternetOpenUrl(hSession, A2W(Url), NULL, 0, INTERNET_FLAG_DONT_CACHE, 0);
		if (handle2 != NULL)
		{
			if ((stream = fopen(save_as, "wb")) != NULL)
			{
				while (Number > 0)
				{
					memset(Temp + remain, 0, MAXBLOCKSIZE - remain);
					InternetReadFile(handle2, Temp + remain, MAXBLOCKSIZE - 100, &Number);
					Number += remain;
					header sta=ps_parase(Temp,Number,remain);
					if (sta == ps_packet_head_loading)
					{
						//基本上不可能不满足
						if (remain <= 100)
						{
							for (int i = 0; i < remain; i++)
							{
								Temp[i] = Temp[Number - remain + i];								
							}
							memset(Temp + remain, 0, MAXBLOCKSIZE - remain);
						}
						else
						{
							std::cout << " downloading  error" << std::endl;
						}

					}
					else
					{
						remain = 0;
					}

					fwrite(Temp, sizeof(char), Number, stream);
				}
				fclose(stream);
			}
			InternetCloseHandle(handle2);
			handle2 = NULL;
		}
		InternetCloseHandle(hSession);
		hSession = NULL;
	}
}

int main(int argc, char* argv[]) {
	download("http://172.22.72.1:9580/test.ps", "c:\\q.ps");/*调用示例，下载百度的首页到c:\index.html文件*/
	return 0;
}