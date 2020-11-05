#pragma once
typedef struct X2Q_STORAGE_HEAD_H
{
	unsigned int utc;                       // UTCʱ��
	unsigned int timestamp;                 // ʱ���
	unsigned char key_flag;                 // �ؼ�֡��ʶ
	unsigned char rsv[3];                   // ��������
}X2Q_STORAGE_HEAD;

// ��Ƶ֡��ʽ�Ķ���
typedef struct X2Q_VIDEO_HEAD_H
{
	unsigned short width;                   // ��Ƶ���
	unsigned short height;                  // ��Ƶ�߶�
	unsigned short producer_id;             // ����ID
	unsigned char frm_rate;                 // ֡��
	unsigned char rsv[9];                   // ��������
}X2Q_VIDEO_HEAD;

// ��Ƶ�㷨ͷ
typedef struct X2Q_VIDEO_ALG_HEAD_H
{
	unsigned char alg;                       // �㷨����
	unsigned char ctl_flag;
	unsigned char rsv1[2];
	unsigned char pravite_data[8];
}X2Q_VIDEO_ALG_HEAD;

// ��Ƶ֡ͷ
typedef struct X2Q_AUDIO_HEAD_H
{
	unsigned short block_align;             // ÿ�㷨֡�ֽ���
	unsigned char channel;                  // ͨ����
	unsigned char bits;                     // ��������
	unsigned short simple;                  // ������
	unsigned short frm_num;                 // �㷨֡����
	unsigned short producer_id;             // ����ID
	unsigned short pcm_len;
	unsigned char energy;
	unsigned char rsv[3];                   // ��������
}X2Q_AUDIO_HEAD;

// ��Ƶ�㷨ͷ
typedef struct X2Q_AUDIO_ALG_HEAD_H
{
	unsigned char alg;						// ��Ƶ֡�㷨
	unsigned char rsv[3];                   // ��������
}X2Q_AUDIO_ALG_HEAD;

// ��Ƶ��Ϣ
typedef struct _FM_AUDIO_INFO
{
	int channel;                  // ͨ����
	int bits;                     // ��������
	int simple;                   // ������
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
//	bool is_key; //�Ƿ��ǹؼ���
//}VIDEO_FRAME; //֡��Ϣ


typedef struct
{
	const BYTE* data;   //sps����
	unsigned int size;          //sps���ݴ�С
	unsigned int index;         //��ǰ����λ���ڵ�λ�ñ��
} sps_bit_stream;

typedef struct
{
	unsigned int profile_idc;
	unsigned int level_idc;

	unsigned int width;
	unsigned int height;
	unsigned int fps;       //SPS�п��ܲ�����FPS��Ϣ
} sps_info_struct;
