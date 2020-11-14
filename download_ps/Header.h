#pragma once

// 帧类型定义
#define FM_FRAME_TYPE_VIDEO  1
#define FM_FRAME_TYPE_AUDIO  2

#define FM_ERROR_OK  0				// 成功;
#define FM_ERROR_UNKWON  -1					// 失败;
#define FM_ERROR_MEM_OVERLOAD  -2				// 内存出错;

// 视频算法类型
#define FM_VIDEO_ALG_H264  1

// 音频算法类型

#define FM_AUDIO_ALG_G711A  3
typedef struct X2Q_STORAGE_HEAD_H
{
	unsigned int utc;                       // UTC时间
	unsigned int timestamp;                 // 时间戳
	unsigned char key_flag;                 // 关键帧标识
	unsigned char rsv[3];                   // 保留区域
}X2Q_STORAGE_HEAD;

// 视频帧格式的定义
typedef struct X2Q_VIDEO_HEAD_H
{
	unsigned short width;                   // 视频宽度
	unsigned short height;                  // 视频高度
	unsigned short producer_id;             // 厂商ID
	unsigned char frm_rate;                 // 帧率
	unsigned char rsv[9];                   // 保留区域
}X2Q_VIDEO_HEAD;

// 视频算法头
typedef struct X2Q_VIDEO_ALG_HEAD_H
{
	unsigned char alg;                       // 算法类型
	unsigned char ctl_flag;
	unsigned char rsv1[2];
	unsigned char pravite_data[8];
}X2Q_VIDEO_ALG_HEAD;

// 音频帧头
typedef struct X2Q_AUDIO_HEAD_H
{
	unsigned short block_align;             // 每算法帧字节数
	unsigned char channel;                  // 通道数
	unsigned char bits;                     // 采样精度
	unsigned short simple;                  // 采样率
	unsigned short frm_num;                 // 算法帧个数
	unsigned short producer_id;             // 厂商ID
	unsigned short pcm_len;
	unsigned char energy;
	unsigned char rsv[3];                   // 保留区域
}X2Q_AUDIO_HEAD;

// 音频算法头
typedef struct X2Q_AUDIO_ALG_HEAD_H
{
	unsigned char alg;						// 音频帧算法
	unsigned char rsv[3];                   // 保留区域
}X2Q_AUDIO_ALG_HEAD;

// 音频信息
typedef struct _FM_AUDIO_INFO
{
	int channel;                  // 通道数
	int bits;                     // 采样精度
	int simple;                   // 采样率
} FM_AUDIO_INFO;


typedef enum
{
	ps_packet_head_loading,
	ps_data_loading,
	ps_continue,
	ps_error,
}Header;

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


typedef struct
{
	const BYTE* data;   //sps数据
	unsigned int size;          //sps数据大小
	unsigned int index;         //当前计算位所在的位置标记
} sps_bit_stream;



typedef struct
{
	unsigned int profile_idc;
	unsigned int level_idc;

	unsigned int width;
	unsigned int height;
	unsigned int fps;       //SPS中可能不包含FPS信息
} sps_info_struct;

typedef struct
{
	unsigned int len;
	unsigned char buff_frame[1024 * 400];
	unsigned int stamp;
	unsigned long pts;
	bool is_key; //是否是关键帧
}VIDEO_FRAME; //帧信息




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
