#pragma once
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



//typedef struct
//{
//	unsigned char wight;
//	unsigned char height;
//	unsigned char len;
//	bool is_key; //是否是关键真
//}VIDEO_FRAME; //帧信息


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
