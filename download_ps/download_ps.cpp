//windows
#ifdef WIN32

#include <windows.h>
#include <wininet.h>
#include <tchar.h>
#include <atlbase.h>
#pragma comment( lib, "wininet.lib" )

#endif // WIN32

#include <atlconv.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include<thread>
#include<mutex>
#include <cstring>
#include <time.h>
#include "PsPacket.h"
#include "Header.h"
#include <queue>

//// ffmpeg
extern "C" 
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/pixfmt.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/parseutils.h>
	#include <libavutil/opt.h>
	#include <libswscale/swscale.h>
	#include <libavutil/time.h>

};
#pragma comment (lib, "avcodec.lib")
#pragma comment (lib, "avdevice.lib")
#pragma comment (lib, "avfilter.lib")
#pragma comment (lib, "avformat.lib")
#pragma comment (lib, "avutil.lib")
#pragma comment (lib, "swresample.lib")
#pragma comment (lib, "swscale.lib")

constexpr auto MAXBLOCKSIZE = 1024 * 10 + 100 ;			//缓存大小

#define IO_BUFFER_SIZE 1024 * 32

#define EQUITPREFIX(arr_name,index) (arr_name[index] == 0 && arr_name[index + 1] == 0 && arr_name[index + 2] == 1)
#define EQUIT00(arr_name,index) (arr_name[index] == 0 && arr_name[index + 1] == 0 && arr_name[index + 2] == 0)
#define EQUITBIT(arr_name,index,t) arr_name[index] == 0 && arr_name[index + 1] == 0 && arr_name[index + 2] == 1 && arr_name[index + 3] == t
#define SUMLEN(arr_name,index)  arr_name[index] << 8 | arr_name[index + 1]
//#define GETBIT(arr_name,index,start,len) (arr_name[index] & (1 <<)) >> start
bool g_is_video = true;
VIDEO_FRAME video_frame;
sps_info_struct sps;


//FILE* fp_open;

mutex m;
queue <int> products;
condition_variable cond;
bool notify = false;





int fill_iobuffer(void* opaque, uint8_t* buf, int buf_size)
{
	//if (!feof(fp_open)) {
	//	int true_size = fread(buf, 1, buf_size, fp_open);
	//	return true_size;
	//}
	//else {
	//	return -1;
	//}
		static int  sum = 0;
		//上锁保护共享资源,unique_lock一次实现上锁和解锁
		unique_lock<mutex>lk(m);
		//等待生产者者通知有资源
		while (!notify) {

			cond.wait(lk);
		}

		//要是队列不为空的话
		if (!products.empty()) {

			if (sum + buf_size <= video_frame.len)
			{
				memcpy_s(buf, buf_size, video_frame.buff_frame + sum, buf_size);
				sum += buf_size;
				return buf_size;
			}
			else if (sum <= video_frame.len)
			{
				
				int len1 = video_frame.len - sum;
				memcpy_s(buf, buf_size, video_frame.buff_frame + sum, len1);
				sum = 0;
				products.pop();
				//通知生产者仓库容量不足,生产产品
				notify = false;
				cond.notify_one();
				return len1;
			}
			else
			{
				std::cout << "buf_size:" << buf_size << "  video_frame.len:" << video_frame.len << std::endl;
				system("pause");
				return -1;
			}

		}
	}


int pushStream( const char* url ,const unsigned char* data = NULL, int len = 0)
{

	AVInputFormat* ifmt_v = NULL;
	AVOutputFormat* ofmt = NULL;
	AVFormatContext* ifmt_ctx_v = NULL, * ofmt_ctx = NULL;
	AVPacket pkt;
	int ret, i;
	int videoindex_v = -1, videoindex_out = -1;
	int frame_index = 0;
	int64_t cur_pts_v = 0;
	AVStream* out_stream = NULL;
	AVStream* in_stream = NULL;
	//fp_open = fopen(in_filename_v, "rb+");

	ifmt_ctx_v = avformat_alloc_context();


	unsigned char* iobuffer = (unsigned char*)av_malloc(IO_BUFFER_SIZE);
	AVIOContext* avio = avio_alloc_context(iobuffer, IO_BUFFER_SIZE, 0, NULL, fill_iobuffer, NULL, NULL);
	ifmt_ctx_v->pb = avio;
	ifmt_v = av_find_input_format("h264");
	if ((ret = avformat_open_input(&ifmt_ctx_v, NULL, ifmt_v, NULL)) < 0) {
		printf("Could not open input file.");
		goto end;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		goto end;
	}

	printf("===========Input Information==========\n");
	//av_dump_format(ifmt_ctx_v, 0, in_filename_v, 0);
	printf("======================================\n");
	avformat_alloc_output_context2(&ofmt_ctx, NULL, "rtsp", url);
	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	av_opt_set(ofmt_ctx->priv_data, "rtsp_transport", "tcp", 0);

	ofmt = ofmt_ctx->oformat;
	in_stream = ifmt_ctx_v->streams[0];

	out_stream = avformat_new_stream(ofmt_ctx, NULL);
	videoindex_v = 0;
	if (!out_stream) {
		printf("Failed allocating output stream\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	videoindex_out = out_stream->index;
	//Copy the settings of AVCodecContext
	if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
		printf("Failed to copy context from input to output stream codec context\n");
		goto end;
	}
	out_stream->codec->codec_tag = 0;
	/* Some formats want stream headers to be separate. */
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		std::cout << "ppppp" << std::endl;

	printf("==========Output Information==========\n");
	av_dump_format(ofmt_ctx, 0, url, 1);
	printf("======================================\n");
	//Open output file
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx->pb, url , AVIO_FLAG_WRITE) < 0) {
			printf("Could not open output file '%s'", url);
			goto end;
		}
	}


	if (avformat_write_header(ofmt_ctx, NULL) < 0) {
		printf("Error occurred when opening output file\n");
		goto end;
	}
	while (1) {
		AVFormatContext* ifmt_ctx;
		int stream_index = 0;
		//AVStream* in_stream, * out_stream;

		//Get an AVPacket
		ifmt_ctx = ifmt_ctx_v;
		stream_index = videoindex_out;

		if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
			do {
				in_stream = ifmt_ctx->streams[pkt.stream_index];
				out_stream = ofmt_ctx->streams[stream_index];

				if (pkt.stream_index == videoindex_v) {
					//FIX：No PTS (Example: Raw H.264)
					//Simple Write PTS
					if (pkt.pts == AV_NOPTS_VALUE) {
						//Write PTS
						AVRational time_base1 = in_stream->time_base;
						//Duration between 2 frames (μs)
						int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
						//Parameters
						pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
						pkt.dts = pkt.pts;
						pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
						frame_index++;
					}
					cur_pts_v = pkt.pts;
					break;
				}
			} while (av_read_frame(ifmt_ctx, &pkt) >= 0);
		}
		else 
		{
			break;
		}

		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index = stream_index;

		av_usleep(pkt.duration);
		printf("Write 1 Packet. size:%5d\tpts:%lld\n", pkt.size, pkt.pts);
		//Write
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf("Error muxing packet\n");
			break;
		}
		av_free_packet(&pkt);

	}


	//Write file trailer
	av_write_trailer(ofmt_ctx);
end:
	avformat_close_input(&ifmt_ctx_v);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	//fclose(fp_open);
	if (ret < 0 && ret != AVERROR_EOF) 
{
		printf("Error occurred.\n");
		return -1;
	}
	//return 0;
	//avformat_network_init();//需要反初始化 avformat_network_deinit(void);
	
	return 0;
}
/**
* arr_name  为数组的名字
* index 或index_xxx 总是要解析的位置，并且是arr_name的绝对位置
* 
* 
*/




void saveH264(const char* data, int len, bool is_add)
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
	
			h264_parse_sps(arr_name + index - 1, len1 - index + 1, &sps);
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
			video_frame.pts = video_frame.pts / 90;
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

			video_frame.pts = (PTS32_30 << 30) + (PTS29_15 << 15) + PTS14_0;
			video_frame.stamp = video_frame.pts / 90;
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
			if (g_is_video) //只添加视频 包含ps包中的所用pes（oxe0）
			{
				memcpy(video_frame.buff_frame + video_frame.len, arr_name + index, len - index);
				video_frame.len += len - index;
			}

			std::cout << "::index:" << index << ":index == len" << std::endl;
			remain_len = 0;
			reserve = packet_end - len;
			saveH264(reinterpret_cast<const char*>(arr_name + index), len - index, false);
			return Header::ps_data_loading;
		}
		else //正常解析
		{
			if (g_is_video) ////只添加视频
			{
				memcpy(video_frame.buff_frame + video_frame.len, arr_name + index, PES_packet_length - PES_header_data_length - 3);
				video_frame.len += PES_packet_length - PES_header_data_length - 3;
			}
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

//少了最后一帧
Header ps_parase(unsigned char* arr_name, int len ,int& remain_len)
{
	static unsigned int reserve = 0; //还有多少字节没有保存
	int index = 0;//当前文件索引
	if (reserve >= len) //处理数据中断
	{
		if (g_is_video) 
		{
			memcpy(video_frame.buff_frame + video_frame.len, arr_name + index, len);
			video_frame.len += len;
		}

		saveH264(reinterpret_cast<const char*>(arr_name + index), len, true);
		reserve -= len;
		return ps_data_loading;
	}
	else if (reserve > 0 && reserve < len)
	{
		if (g_is_video)
		{
			memcpy(video_frame.buff_frame + video_frame.len, arr_name + index, reserve);
			video_frame.len += reserve;
		}

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
			
			if (video_frame.len > 0) 
			{
				
				{
					unique_lock<mutex>lk(m);
					products.push(1);
					notify = true;
					cond.notify_one();
					while (notify || !products.empty()) {

						cond.wait(lk);
					}					
					
					
				}
				//pushStream("rtsp://localhost:554/tes",video_frame.buff_frame, video_frame.len);
				char buff[1024 * 600] = {0};
				int len1 = FM_MakeVideoFrame(video_frame.stamp, sps.width, sps.height, FM_VIDEO_ALG_H264, video_frame.is_key, (char *)video_frame.buff_frame, video_frame.len, buff, 1024 * 600);
				ofstream fd1("add1.h264", ios::out | ios::binary | ios::app);
				unsigned int frametype = 1;
				if (fd1.is_open())
				{
					//fd1.write((char*)video_frame.buff_frame, video_frame.len);
					fd1.write((char *)&frametype,sizeof frametype);
					fd1.write((char *)&len1, sizeof len1);
					fd1.write(buff, len1);
					fd1.close();
				}
			}

			video_frame.is_key = false;
			video_frame.len = 0;
			video_frame.stamp = 0;
			memset(video_frame.buff_frame, 0, 1024 * 400);

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
			video_frame.is_key = true;

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

		}
	
	}
	return ps_error;
}

void download(const char* Url, const char* save_as)	/*将Url指向的地址的文件bai下载到save_as指向的本地文件*/
{

	video_frame.len = 0;
	video_frame.stamp = 0;
	video_frame.is_key = false;
	memset(video_frame.buff_frame, 0, 1024 * 400);
	sps.fps = 0;
	sps.height = 0;
	sps.width = 0;
	sps.profile_idc = 0;
	sps.level_idc = 0;

	byte Temp[MAXBLOCKSIZE];
	//byte save_data[100];
	memset(Temp,0, MAXBLOCKSIZE);
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
					Header sta= ps_parase(Temp,Number,remain);
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
	thread t1(pushStream , "rtsp://localhost:554/tes", video_frame.buff_frame, video_frame.len);
	download("http://172.22.172.1:9580/876221_13660360807_3ZWCA333447FELB.ps", "c:/q.ps");/*调用示例，下载百度的首页到c:\index.html文件*/
	//pushStream("add1.h264","rtsp://localhost:554/t");
	t1.join();
	
	return 0;
}