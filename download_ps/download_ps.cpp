#include <windows.h>
#include <wininet.h>
#include <atlconv.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <tchar.h>
#include "PsPacket.h"
#include <atlbase.h>
#include <time.h>
#include "Header.h"
#pragma comment( lib, "wininet.lib" )

// 帧类型定义
constexpr auto FM_FRAME_TYPE_VIDEO = 1;
constexpr auto FM_FRAME_TYPE_AUDIO = 2;

constexpr auto FM_ERROR_OK = 0;				// 成功;
constexpr auto FM_ERROR_UNKWON = -1;					// 失败;
constexpr auto FM_ERROR_MEM_OVERLOAD = -2;				// 内存出错;

// 视频算法类型
constexpr auto FM_VIDEO_ALG_H264 = 1;

// 音频算法类型
constexpr auto FM_AUDIO_ALG_G711A = 3;

constexpr auto MAXBLOCKSIZE = 1024 * 10 +100 ;			//缓存大小
#define EQUITPREFIX(arr_name,index) (arr_name[index] == 0 && arr_name[index + 1] == 0 && arr_name[index + 2] == 1)
#define EQUIT00(arr_name,index) (arr_name[index] == 0 && arr_name[index + 1] == 0 && arr_name[index + 2] == 0)
#define EQUITBIT(arr_name,index,t) arr_name[index] == 0 && arr_name[index + 1] == 0 && arr_name[index + 2] == 1 && arr_name[index + 3] == t
#define SUMLEN(arr_name,index)  arr_name[index] << 8 | arr_name[index + 1]
//#define GETBIT(arr_name,index,start,len) (arr_name[index] & (1 <<)) >> start

 
/**
* arr_name  为数组的名字
* index 或index_xxx 总是要解析的位置，并且是arr_name的绝对位置
* 
* 
*/



bool g_is_video = true;
void saveH264(const char* data , int len ,bool is_add) 
{
	if (!g_is_video)
	{
		return;
	}
	ofstream fd;
	fd.open("ps.h264", ios::out | ios::binary | ios::app);
	if (fd.is_open())
	{
		fd.write(data, len);
		fd.close();
	}
	//只保留一个PES包
	ofstream fd_frame;
	if (is_add)
	{
		fd_frame.open("pes.h264.data", ios::out | ios::binary | ios::app);
	}
	else
	{
		fd_frame.open("pes.h264.data", ios::out | ios::binary);
	}

	if (fd_frame.is_open())
	{
		fd_frame.write(data, len);
		fd_frame.close();
	}

}


int FM_MakeVideoFrame(unsigned int ts, int width, int height, int alg, int key_flag, char* data, int dataLen, char* buf, int bufLen)
{
	int videoDataLen = sizeof(X2Q_STORAGE_HEAD) + sizeof(X2Q_VIDEO_HEAD) + sizeof(X2Q_VIDEO_ALG_HEAD) + dataLen;
	if (videoDataLen > bufLen)   // buf空间不够大
	{
		return FM_ERROR_MEM_OVERLOAD;
	}

	memset(buf, 0, bufLen);  // 将buf置0

	videoDataLen = 0;    // buf指针的偏移量

	X2Q_STORAGE_HEAD* pStorageHead = (X2Q_STORAGE_HEAD*)buf; // 数据包头
	pStorageHead->utc = time(NULL);
	pStorageHead->timestamp = ts;
	pStorageHead->key_flag = key_flag;

	videoDataLen += sizeof(X2Q_STORAGE_HEAD);

	X2Q_VIDEO_HEAD* pVideoHead = (X2Q_VIDEO_HEAD*)(buf + videoDataLen);   // 视频头
	pVideoHead->frm_rate = 25;
	pVideoHead->width = width;
	pVideoHead->height = height;
	pVideoHead->producer_id = 1;

	videoDataLen += sizeof(X2Q_VIDEO_HEAD);

	X2Q_VIDEO_ALG_HEAD* pVideoAlgHead = (X2Q_VIDEO_ALG_HEAD*)(buf + videoDataLen);  // 视屏算法头
	pVideoAlgHead->alg = alg;           // 只填算法类型，其它数据置0
	pVideoAlgHead->ctl_flag = 0;

	videoDataLen += sizeof(X2Q_VIDEO_ALG_HEAD);

	memcpy(buf + videoDataLen, data, dataLen);

	return videoDataLen + dataLen;
}


int FM_MakeAudioFrame(unsigned int ts, int alg, char* data, int dataLen, FM_AUDIO_INFO* info, char* buf, int bufLen)
{
	memset(buf, 0, bufLen); // 把buf所有字节置0
	if ((sizeof(X2Q_STORAGE_HEAD) + sizeof(X2Q_AUDIO_HEAD)) > bufLen)   // buf空间不够大
	{
		return FM_ERROR_MEM_OVERLOAD;
	}
	int offset = 0;

	X2Q_STORAGE_HEAD* pStorageHead = (X2Q_STORAGE_HEAD*)(buf + offset);                                            // 数据包头
	pStorageHead->utc = time(NULL);
	pStorageHead->timestamp = ts;
	pStorageHead->key_flag = 1;

	offset += sizeof(X2Q_STORAGE_HEAD);

	X2Q_AUDIO_HEAD* pAudioHead = (X2Q_AUDIO_HEAD*)(buf + offset);
	pAudioHead->bits = info->bits;
	pAudioHead->channel = info->channel;
	pAudioHead->simple = info->simple / 100.0;
	pAudioHead->energy = 10;
	pAudioHead->producer_id = 1;
	switch (alg)
	{
	case FM_AUDIO_ALG_G711A:
		// 确定算法类型后判断传入的data长度是否合法
		if (dataLen < 0 || dataLen % (6 * 80) != 0)
		{
			return FM_ERROR_UNKWON;
		}

		pAudioHead->block_align = 4 + 4 + 6 * 80;
		pAudioHead->frm_num = dataLen / (6 * 80);
		pAudioHead->pcm_len = 6 * 160;
		break;
	default:
		return FM_ERROR_UNKWON;
	}
	offset += sizeof(X2Q_AUDIO_HEAD);

	// 判断buf空间是否足够
	if ((offset + pAudioHead->frm_num * pAudioHead->block_align) > bufLen)
	{
		// 传入的buf长度不足以执行内存拷贝
		return FM_ERROR_MEM_OVERLOAD;
	}

	// 多个音频包，添加到buf里面
	for (int i = 0; i < pAudioHead->frm_num; i++)
	{
		char* writePos = buf + offset;
		// 音频算法头
		X2Q_AUDIO_ALG_HEAD* pAudioAlgHead = (X2Q_AUDIO_ALG_HEAD*)(writePos);
		pAudioAlgHead->alg = alg;

		writePos += sizeof(X2Q_AUDIO_ALG_HEAD);

		switch (alg)
		{
		case FM_AUDIO_ALG_G711A:
		{
			// 添加海思帧头
			writePos[0] = 0x0;
			writePos[1] = 0x1;
			*((unsigned short*)&writePos[2]) = 480 / 2;
			writePos += 4;

			// 写音频数据
			char* dataPos = data + i * 480;
			memcpy(writePos, dataPos, 480);
		}
		break;
		default:
			return FM_ERROR_UNKWON;
		}
		offset += pAudioHead->block_align;
	}
	return  offset;
}



static void del_emulation_prevention(BYTE* data, UINT* dataSize)
{
	UINT dataSizeTemp = *dataSize;
	for (UINT i = 0, j = 0; i < (dataSizeTemp - 2); i++) {
		int val = (data[i] ^ 0x0) + (data[i + 1] ^ 0x0) + (data[i + 2] ^ 0x3);    //检测是否是竞争码
		if (val == 0) {
			for (j = i + 2; j < dataSizeTemp - 1; j++) {    //移除竞争码
				data[j] = data[j + 1];
			}

			(*dataSize)--;      //data size 减1
		}
	}
}

static void sps_bs_init(sps_bit_stream* bs, const BYTE* data, UINT size)
{
	if (bs) {
		bs->data = data;
		bs->size = size;
		bs->index = 0;
	}
}


/**
 是否已经到数据流最后

 @param bs sps_bit_stream数据
 @return 1：yes，0：no
 */

static INT eof(sps_bit_stream* bs)
{
	return (bs->index >= bs->size * 8);    //位偏移已经超出数据
}

/**
 读取从起始位开始的BitCount个位所表示的值

 @param bs sps_bit_stream数据
 @param bitCount bit位个数(从低到高)
 @return value
 */
static UINT u(sps_bit_stream* bs, BYTE bitCount)
{
	UINT val = 0;
	for (BYTE i = 0; i < bitCount; i++) {
		val <<= 1;
		if (eof(bs)) {
			val = 0;
			break;
		}
		else if (bs->data[bs->index / 8] & (0x80 >> (bs->index % 8))) {     //计算index所在的位是否为1
			val |= 1;
		}
		bs->index++;
	}

	return val;
}


/**
 读取无符号哥伦布编码值(UE)
 #2^LeadingZeroBits - 1 + (xxx)
 @param bs sps_bit_stream数据
 @return value
 */
static UINT ue(sps_bit_stream* bs)
{
	UINT zeroNum = 0;
	while (u(bs, 1) == 0 && !eof(bs) && zeroNum < 32) {
		zeroNum++;
	}

	return (UINT)((1 << zeroNum) - 1 + u(bs, zeroNum));
}


/**
 读取有符号哥伦布编码值(SE)
 #(-1)^(k+1) * Ceil(k/2)

 @param bs sps_bit_stream数据
 @return value
 */
static INT se(sps_bit_stream* bs)
{
	INT ueVal = (INT)ue(bs);
	double k = ueVal;

	INT seVal = (INT)ceil(k / 2);     //ceil:返回大于或者等于指定表达式的最小整数
	if (ueVal % 2 == 0) {       //偶数取反，即(-1)^(k+1)
		seVal = -seVal;
	}

	return seVal;
}


/**
 视频可用性信息(Video usability information)解析
 @param bs sps_bit_stream数据
 @param info sps解析之后的信息数据及结构体
 @see E.1.1 VUI parameters syntax
 */
void vui_para_parse(sps_bit_stream* bs, sps_info_struct* info)
{
	UINT aspect_ratio_info_present_flag = u(bs, 1);
	if (aspect_ratio_info_present_flag) {
		UINT aspect_ratio_idc = u(bs, 8);
		if (aspect_ratio_idc == 255) {      //Extended_SAR
			u(bs, 16);      //sar_width
			u(bs, 16);      //sar_height
		}
	}

	UINT overscan_info_present_flag = u(bs, 1);
	if (overscan_info_present_flag) {
		u(bs, 1);       //overscan_appropriate_flag
	}

	UINT video_signal_type_present_flag = u(bs, 1);
	if (video_signal_type_present_flag) {
		u(bs, 3);       //video_format
		u(bs, 1);       //video_full_range_flag
		UINT colour_description_present_flag = u(bs, 1);
		if (colour_description_present_flag) {
			u(bs, 8);       //colour_primaries
			u(bs, 8);       //transfer_characteristics
			u(bs, 8);       //matrix_coefficients
		}
	}

	UINT chroma_loc_info_present_flag = u(bs, 1);
	if (chroma_loc_info_present_flag) {
		ue(bs);     //chroma_sample_loc_type_top_field
		ue(bs);     //chroma_sample_loc_type_bottom_field
	}

	UINT timing_info_present_flag = u(bs, 1);
	if (timing_info_present_flag) {
		UINT num_units_in_tick = u(bs, 32);
		UINT time_scale = u(bs, 32);
		UINT fixed_frame_rate_flag = u(bs, 1);

		info->fps = (UINT)((float)time_scale / (float)num_units_in_tick);
		if (fixed_frame_rate_flag) {
			info->fps = info->fps / 2;
		}
	}

	UINT nal_hrd_parameters_present_flag = u(bs, 1);
	if (nal_hrd_parameters_present_flag) {
		//hrd_parameters()  //see E.1.2 HRD parameters syntax
	}

	//后面代码需要hrd_parameters()函数接口实现才有用
	UINT vcl_hrd_parameters_present_flag = u(bs, 1);
	if (vcl_hrd_parameters_present_flag) {
		//hrd_parameters()
	}
	if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
		u(bs, 1);   //low_delay_hrd_flag
	}

	u(bs, 1);       //pic_struct_present_flag
	UINT bitstream_restriction_flag = u(bs, 1);
	if (bitstream_restriction_flag) {
		u(bs, 1);   //motion_vectors_over_pic_boundaries_flag
		ue(bs);     //max_bytes_per_pic_denom
		ue(bs);     //max_bits_per_mb_denom
		ue(bs);     //log2_max_mv_length_horizontal
		ue(bs);     //log2_max_mv_length_vertical
		ue(bs);     //max_num_reorder_frames
		ue(bs);     //max_dec_frame_buffering
	}
}


INT h264_parse_sps(const BYTE* data, UINT dataSize, sps_info_struct* info)
{
	if (!data || dataSize <= 0 || !info) return 0;
	INT ret = 0;

	BYTE* dataBuf = (BYTE*)malloc(dataSize);
	memcpy(dataBuf, data, dataSize);        //重新拷贝一份数据，防止移除竞争码时对原数据造成影响
	del_emulation_prevention(dataBuf, &dataSize);

	sps_bit_stream bs = { 0 };
	sps_bs_init(&bs, dataBuf, dataSize);   //初始化SPS数据流结构体

	u(&bs, 1);      //forbidden_zero_bit
	u(&bs, 2);      //nal_ref_idc
	UINT nal_unit_type = u(&bs, 5);

	if (nal_unit_type == 0x7) {     //Nal SPS Flag
		info->profile_idc = u(&bs, 8);
		u(&bs, 1);      //constraint_set0_flag
		u(&bs, 1);      //constraint_set1_flag
		u(&bs, 1);      //constraint_set2_flag
		u(&bs, 1);      //constraint_set3_flag
		u(&bs, 1);      //constraint_set4_flag
		u(&bs, 1);      //constraint_set4_flag
		u(&bs, 2);      //reserved_zero_2bits
		info->level_idc = u(&bs, 8);

		ue(&bs);    //seq_parameter_set_id

		UINT chroma_format_idc = 1;     //摄像机出图大部分格式是4:2:0
		if (info->profile_idc == 100 || info->profile_idc == 110 || info->profile_idc == 122 ||
			info->profile_idc == 244 || info->profile_idc == 44 || info->profile_idc == 83 ||
			info->profile_idc == 86 || info->profile_idc == 118 || info->profile_idc == 128 ||
			info->profile_idc == 138 || info->profile_idc == 139 || info->profile_idc == 134 || info->profile_idc == 135) {
			chroma_format_idc = ue(&bs);
			if (chroma_format_idc == 3) {
				u(&bs, 1);      //separate_colour_plane_flag
			}

			ue(&bs);        //bit_depth_luma_minus8
			ue(&bs);        //bit_depth_chroma_minus8
			u(&bs, 1);      //qpprime_y_zero_transform_bypass_flag
			UINT seq_scaling_matrix_present_flag = u(&bs, 1);
			if (seq_scaling_matrix_present_flag) {
				UINT seq_scaling_list_present_flag[8] = { 0 };
				for (INT i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) {
					seq_scaling_list_present_flag[i] = u(&bs, 1);
					if (seq_scaling_list_present_flag[i]) {
						if (i < 6) {    //scaling_list(ScalingList4x4[i], 16, UseDefaultScalingMatrix4x4Flag[i])
						}
						else {    //scaling_list(ScalingList8x8[i − 6], 64, UseDefaultScalingMatrix8x8Flag[i − 6] )
						}
					}
				}
			}
		}

		ue(&bs);        //log2_max_frame_num_minus4
		UINT pic_order_cnt_type = ue(&bs);
		if (pic_order_cnt_type == 0) {
			ue(&bs);        //log2_max_pic_order_cnt_lsb_minus4
		}
		else if (pic_order_cnt_type == 1) {
			u(&bs, 1);      //delta_pic_order_always_zero_flag
			se(&bs);        //offset_for_non_ref_pic
			se(&bs);        //offset_for_top_to_bottom_field

			UINT num_ref_frames_in_pic_order_cnt_cycle = ue(&bs);
			INT* offset_for_ref_frame = (INT*)malloc((UINT)num_ref_frames_in_pic_order_cnt_cycle * sizeof(INT));
			for (INT i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
				offset_for_ref_frame[i] = se(&bs);
			}
			free(offset_for_ref_frame);
		}

		ue(&bs);      //max_num_ref_frames
		u(&bs, 1);      //gaps_in_frame_num_value_allowed_flag

		UINT pic_width_in_mbs_minus1 = ue(&bs);     //第36位开始
		UINT pic_height_in_map_units_minus1 = ue(&bs);      //47
		UINT frame_mbs_only_flag = u(&bs, 1);

		info->width = (INT)(pic_width_in_mbs_minus1 + 1) * 16;
		info->height = (INT)(2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;

		if (!frame_mbs_only_flag) {
			u(&bs, 1);      //mb_adaptive_frame_field_flag
		}

		u(&bs, 1);     //direct_8x8_inference_flag
		UINT frame_cropping_flag = u(&bs, 1);
		if (frame_cropping_flag) {
			UINT frame_crop_left_offset = ue(&bs);
			UINT frame_crop_right_offset = ue(&bs);
			UINT frame_crop_top_offset = ue(&bs);
			UINT frame_crop_bottom_offset = ue(&bs);

			//See 6.2 Source, decoded, and output picture formats
			INT crop_unit_x = 1;
			INT crop_unit_y = 2 - frame_mbs_only_flag;      //monochrome or 4:4:4
			if (chroma_format_idc == 1) {   //4:2:0
				crop_unit_x = 2;
				crop_unit_y = 2 * (2 - frame_mbs_only_flag);
			}
			else if (chroma_format_idc == 2) {    //4:2:2
				crop_unit_x = 2;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}

			info->width -= crop_unit_x * (frame_crop_left_offset + frame_crop_right_offset);
			info->height -= crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);
		}

		UINT vui_parameters_present_flag = u(&bs, 1);
		if (vui_parameters_present_flag) {
			vui_para_parse(&bs, info);
		}

		ret = 1;
	}
	free(dataBuf);

	return ret;
}

Header parase_h264(int len)
{
	ifstream fd_frame;
	fd_frame.open("pes.h264.data", ios::in | ios::binary);
	if (!fd_frame.is_open())
	{
		return ps_error;
	}
	int index = 0;
	std::cout << "len" << len << std::endl;
	std::stringstream strstream;
	strstream << fd_frame.rdbuf();
	fd_frame.close();
	std::string str_name = strstream.str();
	unsigned int len1 = str_name.length();
	unsigned char* arr_name = (unsigned char*)str_name.data();

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
			sps_info_struct sps;
			h264_parse_sps(arr_name + index - 1, len1 - index + 1, &sps);
			//int profile_ide = arr_name[index ++];
			//int constraint_set0_flag = (arr_name[index]  & 0x80) >> 7;
			//int constraint_set1_flag = (arr_name[index] & 0x40) >> 6;
			//int constraint_set2_flag = (arr_name[index] & 0x20) >> 5;
			//int constraint_set3_flag = (arr_name[index] & 0x10) >> 4;
			//int constraint_set4_flag = (arr_name[index] & 0x08) >> 3;
			//int constraint_set5_flag = (arr_name[index] & 0x04) >> 2;
			//int reserved_zero_2bit	=  arr_name[index] & 0x03;
			//int level_idc = arr_name[++index];


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

Header parase_pes(unsigned char* arr_name, int& index, int len ,unsigned int& reserve, int& remain_len)
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
			return Header::ps_packet_head_loading;
		}




		if (index > len)//解析头时 缓存数据不够
		{
			std::cout << "::index:" << index <<":index > len" <<std::endl;
			remain_len = len - start_index + 4;
			return Header::ps_packet_head_loading;
		}
		else if (packet_end >= len || index == len) //解析数据  数据时不够
		{
			std::cout << "::index:" << index << ":index == len" << std::endl;
			remain_len = 0;
			reserve = packet_end - len;
			//index_globe += packet_end - 1;
			saveH264(reinterpret_cast<const char*>(arr_name + index), len - index, false);
			return Header::ps_data_loading;
		}
		else //正常解析
		{
			remain_len = 0;

			saveH264(reinterpret_cast<const char*>(arr_name + index), PES_packet_length - PES_header_data_length - 3, false);
			parase_h264(PES_packet_length - PES_header_data_length - 3);
			index = packet_end;
			std::cout << (int)arr_name[index] << ":" << (int)arr_name[index + 1] << ":" << (int)arr_name[index + 2] << ":" << (int)arr_name[index + 3] << std::endl;
			return Header::ps_continue;
		}

	}
	else 
	{
		remain_len = len - start_index + 4;
		return Header::ps_packet_head_loading;
	}

}

Header ps_parase(unsigned char* arr_name, int len ,int& remain_len)
{
	static unsigned int reserve = 0; //还有多少字节没有保存
	int index = 0;//当前文件索引
	if (reserve >= len) //处理数据中断
	{
		saveH264(reinterpret_cast<const char*>(arr_name + index), len, true);
		reserve -= len;
		return ps_data_loading;
	}
	else if (reserve > 0 && reserve < len)
	{
		saveH264(reinterpret_cast<const char*>(arr_name + index), reserve, true);
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
			g_is_video = true;
			index += 4;

			Header sta = parase_pes(arr_name, index, len, reserve, remain_len);
			if (sta == ps_packet_head_loading || sta == ps_data_loading)
			{
				return sta;
			}

			break;
		}
		case 0xc0: //PSE
		{
			g_is_video = false;
			index += 4;
			Header sta = parase_pes(arr_name, index, len, reserve, remain_len);
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
	//L"RookIE/1.0"
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
					Header sta=ps_parase(Temp,Number,remain);
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
	//http://172.22.72.1:9580/test.ps
	download("http://172.22.172.1:9580/876221_13660360807_3ZWCA333447FELB.ps", "c:/q.ps");/*调用示例，下载百度的首页到c:\index.html文件*/
	return 0;
}