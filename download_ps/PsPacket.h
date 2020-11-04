#ifndef _PSUNPACKET_H_
#define _PSUNPACKET_H_

#include <atlbase.h>
#include "streamdef.h"
#include <vector>
using namespace std;

#define MAX_VIDEO_BUFSIZE  (200*1024)

struct ESFrameData
{
	unsigned char* buffer;
	int				len;
	int             frame_num; //��������֡����Ŀ
	unsigned char	cFrameType;			//֡������, 'I'�� 'P'�� 'B'
	unsigned int	bDropFrame;			//�Ƿ񶪵���֡������ж������򶪵�
	//PSStatus        ps_status;   //PS������
	__int64  first_pts_time; //֡��������ܺ��ж�֡����¼��һ֡��ʱ���
	__int64 ptsTime; //֡���������һ֡��ʱ���
	__int64 dtsTime;
	unsigned char pts_dts_flag; //һ��PS֡�ɶ��PES֡��ɣ����п�ͷ֡��ʱ�����ֵΪ2����PTS����3����PTS��DTS����������ͷ֮֡�������֡,��ֵΪ0������PTS��DTS��Ϊ0.
};


//#include <tuple>
//using namespace std::tr1;

#ifndef AV_RB16
#   define AV_RB16(x)                           \
    ((((const unsigned char*)(x))[0] << 8) |          \
    ((const unsigned char*)(x))[1])
#endif


static inline unsigned __int64 ff_parse_pes_pts(const unsigned char* buf) {
	return (unsigned __int64)(*buf & 0x0e) << 29 |
		(AV_RB16(buf + 1) >> 1) << 15 |
		AV_RB16(buf + 3) >> 1;
}

static unsigned __int64 get_pts(optional_pes_header* option)
{
	if (option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0)
	{
		return 0;
	}
	if ((option->PTS_DTS_flags & 2) == 2)
	{
		unsigned char* pts = (unsigned char*)option + sizeof(optional_pes_header);
		return ff_parse_pes_pts(pts);
	}
	return 0;
}

static unsigned __int64 get_dts(optional_pes_header* option)
{
	if (option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0)
	{
		return 0;
	}
	if ((option->PTS_DTS_flags & 3) == 3)
	{
		unsigned char* dts = (unsigned char*)option + sizeof(optional_pes_header) + 5;
		return ff_parse_pes_pts(dts);
	}
	return 0;
}

bool inline is_ps_header(ps_header_t* ps)
{
	if (ps->pack_start_code[0] == 0 && ps->pack_start_code[1] == 0 && ps->pack_start_code[2] == 1 && ps->pack_start_code[3] == 0xBA)
		return true;
	return false;
}
bool inline is_sh_header(sh_header_t* sh)
{
	if (sh->system_header_start_code[0] == 0 && sh->system_header_start_code[1] == 0 && sh->system_header_start_code[2] == 1 && sh->system_header_start_code[3] == 0xBB)
		return true;
	return false;
}

//bool inline is_psm_header(psm_header_t* psm)
//{
//    if(psm->promgram_stream_map_start_code[0] == 0 && psm->promgram_stream_map_start_code[1] == 0 && psm->promgram_stream_map_start_code[2] == 1 && psm->promgram_stream_map_start_code[3] == 0xBC)
//        return true;
//    return false;
//}
bool inline is_psm_header(program_stream_map* psm)
{
	if (psm->PackStart.start_code[0] == 0 && psm->PackStart.start_code[1] == 0 && psm->PackStart.start_code[2] == 1 && psm->PackStart.stream_id[0] == 0xBC)
		return true;
	return false;
}

bool inline is_pes_video_header(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1 && (pes->stream_id == 0xE0 || pes->stream_id == 0xE2))
		return true;
	return false;
}
bool inline is_pes_audio_header(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1 && pes->stream_id == 0xC0)
		return true;
	return false;
}

bool inline is_pes_header(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1)
	{
		if (pes->stream_id == 0xC0 || (pes->stream_id == 0xE0 || pes->stream_id == 0xE2))
		{
			return true;
		}
	}
	return false;
}
PSStatus inline pes_type(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1)
	{
		if (pes->stream_id == 0xC0)
		{
			return ps_pes_audio;
		}
		else if (pes->stream_id == 0xE0 || pes->stream_id == 0xE2)
		{
			return ps_pes_video;
		}
	}
	return ps_padding;
}

static bool NextMpegStartCode(BYTE& code, char* pBuffer, int nLeftLen, int& nReadBytes)
{
	//BitByteAlign();
	int i = 0;
	nReadBytes = 0;

	DWORD dw = -1;
	do
	{
		if (nLeftLen-- == 0) return(false);
		dw = (dw << 8) | (BYTE)pBuffer[i++];
		nReadBytes += 1;;
	} while ((dw & 0xffffff00) != 0x00000100);
	code = (BYTE)(dw & 0xff);
	return(true);
}



/*
_1 �Ƿ��������
_2 ��һ��PS״̬
_3 ����ָ��
_4 ���ݳ���
*/
//typedef std::tr1::tuple<bool, PSStatus, pes_header_t*> pes_tuple;

struct pes_tuple
{
	bool           bContainData;
	PSStatus       nStatus;
	pes_header_t* pPESPtr;
	bool  bDataLost; //�Ƿ񶪰�

	pes_tuple(bool bContain, PSStatus status, pes_header_t* pPes, bool bLost)
	{
		bContainData = bContain;
		bDataLost = bLost;
		nStatus = status;
		pPESPtr = pPes;
	}
};


/*
_1 �Ƿ��������
_2 ��������
_3 PTSʱ���
_4 DTSʱ���
_5 ����ָ��
_6 ���ݳ���
*/
//typedef std::tr1::tuple<bool, unsigned char, unsigned __int64, unsigned __int64, char*, unsigned int> naked_tuple;

struct naked_tuple
{
	bool bContainData;
	unsigned char nStreamID;//0xE0-- ��Ƶ��0xC0--��Ƶ
	unsigned __int64 ptsTime;
	unsigned __int64 dtsTime;
	char* pDataPtr;
	unsigned int nDataLen;
	bool  bDataLost; //�Ƿ񶪰�
	unsigned char pts_dts_flag; // �Ƿ������PTS��DTS���������PTS��ֵ����2���������PTS��DTS��ֵ����3�����û�а���PTS��DTS��ֵ����0

	naked_tuple(bool bContain, unsigned char nStreamType, unsigned __int64 pts, unsigned char fpts_dts, unsigned __int64 dts, char* pData, unsigned int nLen, bool bLost)
	{
		bContainData = bContain;
		nStreamID = nStreamType;
		ptsTime = pts;
		dtsTime = dts;
		pDataPtr = pData;
		nDataLen = nLen;
		bDataLost = bLost;
		pts_dts_flag = fpts_dts;
	}
};

class PSPacket
{
public:
	PSPacket()
	{
		m_pPSBuffer = new char[MAX_PS_LENGTH];
		m_pPESBuffer = new char[MAX_PES_LENGTH];
		m_pESBuffer = new char[MAX_ES_LENGTH];

		m_status = ps_padding;
		m_nESLength = m_nPESIndicator = m_nPSWrtiePos = m_nPESLength = 0;

		m_bRealTime = false;
	}
	~PSPacket()
	{
		delete m_pPSBuffer;
		delete m_pPESBuffer;
		delete m_pESBuffer;
	}

	int GetWritePos()
	{
		return m_nPSWrtiePos;
	}

	int  GetReadPos()
	{
		return m_nPESIndicator;
	}

	void PSWrite(char* pBuffer, unsigned int sz)
	{
		if (m_nPSWrtiePos + sz < MAX_PS_LENGTH)
		{
			memcpy((m_pPSBuffer + m_nPSWrtiePos), pBuffer, sz);
			m_nPSWrtiePos += sz;
		}
		else
		{
			m_status = ps_padding;
			m_nESLength = m_nPESIndicator = m_nPSWrtiePos = m_nPESLength = 0;

			if (m_nPSWrtiePos + sz < MAX_PS_LENGTH)
			{
				memcpy((m_pPSBuffer + m_nPSWrtiePos), pBuffer, sz);
				m_nPSWrtiePos += sz;
			}
		}

	}

	//�������ָ��
	void  Empty()
	{
		m_status = ps_padding;
		m_nESLength = m_nPESIndicator = m_nPSWrtiePos = m_nPESLength = 0;
		m_ProgramMap.clear();
	}

	//����ʵʱ���ı�־
	void SetRealTime(bool bRealTime)
	{
		m_bRealTime = bRealTime;
	}

	int GetEsData(ESFrameData* pVideoESFrame, ESFrameData* pAudioESFrame, BOOL& bDataLost)
	{
		int nRet = 0;
		int nStreamType = 0;
		unsigned char* pV = NULL;
		unsigned char* pA = NULL;

		if (pVideoESFrame != NULL)
		{
			pV = pVideoESFrame->buffer;
			pVideoESFrame->frame_num = 0;
		}
		if (pAudioESFrame != NULL)
		{
			pA = pAudioESFrame->buffer;
			pAudioESFrame->frame_num = 0;
		}

		while (1)
		{

			naked_tuple es_packet = naked_payload();

			if (!es_packet.bContainData || es_packet.nDataLen == 0)
			{
				break;
			}

			if (es_packet.nStreamID == 0xE0 || es_packet.nStreamID == 0xE2)
			{
				if (pVideoESFrame != NULL)
				{
					ATLASSERT(pVideoESFrame->len + es_packet.nDataLen < MAX_VIDEO_BUFSIZE);

					if (pVideoESFrame->len + es_packet.nDataLen > MAX_VIDEO_BUFSIZE)
					{
						OutputDebugString(_T("Data Len is over buffer size \n"));
						break;
					}

					memcpy(pV, es_packet.pDataPtr, es_packet.nDataLen);
					pV += es_packet.nDataLen;
					pVideoESFrame->len += es_packet.nDataLen;

					pVideoESFrame->pts_dts_flag = es_packet.pts_dts_flag; //�����ֵ��Ϊ0����ʾ����һ��֡��֡ͷ�����Ҵ�ʱ���

					pVideoESFrame->ptsTime = es_packet.ptsTime;

					if (pVideoESFrame->frame_num == 0)
						pVideoESFrame->first_pts_time = pVideoESFrame->ptsTime;

					pVideoESFrame->frame_num++;
				}

				if (es_packet.bDataLost)
					bDataLost = TRUE;

				nStreamType |= 0x01;

				nRet = 1;
			}
			else if (es_packet.nStreamID == 0xC0)
			{
				if (pAudioESFrame != NULL)
				{
					ATLASSERT(pAudioESFrame->len + es_packet.nDataLen < 192 * 1024);

					memcpy(pA, es_packet.pDataPtr, es_packet.nDataLen);
					pA += es_packet.nDataLen;
					pAudioESFrame->len += es_packet.nDataLen;

					pAudioESFrame->ptsTime = es_packet.ptsTime;

					pAudioESFrame->pts_dts_flag = es_packet.pts_dts_flag;

					if (pAudioESFrame->frame_num == 0)
						pAudioESFrame->first_pts_time = pAudioESFrame->ptsTime;

					pAudioESFrame->frame_num++;
				}

				nStreamType |= 0x02;

				nRet = 1;
			}
		}

		//      if(nStreamType == 0)
			  //{
			  //	ATLTRACE("Error: GetEsData found no pes----------- \n");
			  //}

		return nRet;
	}

	void RTPWrite(char* pBuffer, unsigned int sz)
	{
		char* data = (pBuffer + sizeof(RTP_header_t));
		unsigned int length = sz - sizeof(RTP_header_t);
		if (m_nPSWrtiePos + length < MAX_PS_LENGTH)
		{
			memcpy((m_pPSBuffer + m_nPSWrtiePos), data, length);
			m_nPSWrtiePos += length;
		}
		else
		{
			m_status = ps_padding;
			m_nESLength = m_nPESIndicator = m_nPSWrtiePos = m_nPESLength = 0;

			if (m_nPSWrtiePos + length < MAX_PS_LENGTH)
			{
				memcpy((m_pPSBuffer + m_nPSWrtiePos), data, length);
				m_nPSWrtiePos += length;
			}
		}
	}

	//��ȡPS���ͣ�����������Ƶ����Ƶ��ע�⺯������ΪStatic����
	static PSStatus DetectPSType(BYTE* pBuffer, int nSize)
	{
		PSStatus _status = ps_padding;
		ps_header_t* _ps;
		pes_header_t* _pes;

		const int nEndPos = nSize - 1;
		int nReadPos = 0;

		while (nReadPos < nEndPos)
		{
			int nRead = 0;
			BYTE b = 0;

			if (!NextMpegStartCode(b, (char*)pBuffer + nReadPos, nEndPos - nReadPos, nRead))
			{
				//ATLASSERT(0);
				break;
			}

			nReadPos += nRead - 4; //��λ����ʼ��

			_ps = (ps_header_t*)(pBuffer + nReadPos);
			if (is_ps_header(_ps))
			{
				_status = ps_ps;
				nReadPos += sizeof(ps_header_t); continue;
			}

			if (b >= 0xbd && b < 0xf0) //�Ϸ���PES֡
			{
				_pes = (pes_header_t*)(pBuffer + nReadPos);

				//unsigned short PES_packet_length = ntohs(_pes->PES_packet_length);

				if (is_pes_header(_pes))
				{
					_status = ps_pes;
				}

				if (_status == ps_pes)
				{
					_status = pes_type(_pes);

					return _status;
				}
				else
				{
					nReadPos += 1;
				}

			} // if(b >= 0xbd && b < 0xf0)
			else
			{
				nReadPos += 4;
			}

		}//while

		//_status = ps_padding;

		return _status;
	}

	pes_tuple pes_payload()
	{
		//int nLastPos = m_nPESIndicator; //����ɵĶ�ȡλ�ã���������ȡ����ָ�����͵�֡��ps_ps/ps_pes�����򽫶�ȡָ��Ҫ���¶�λ���ɵ�λ��

		while (m_nPESIndicator < m_nPSWrtiePos)
		{
			int nRead = 0;
			BYTE b = 0;

			if (!NextMpegStartCode(b, m_pPSBuffer + m_nPESIndicator, m_nPSWrtiePos - m_nPESIndicator, nRead))
			{
				//ATLASSERT(0);
				break;
			}

			m_nPESIndicator += nRead - 4; //��λ����ʼ��

			m_ps = (ps_header_t*)(m_pPSBuffer + m_nPESIndicator);
			if (is_ps_header(m_ps))
			{
				m_status = ps_ps;
				m_nPESIndicator += sizeof(ps_header_t); continue;
			}

			if (b >= 0xbd && b < 0xf0) //�Ϸ���PES֡
			{
				m_pes = (pes_header_t*)(m_pPSBuffer + m_nPESIndicator);

				unsigned short PES_packet_length = ntohs(m_pes->PES_packet_length);

				if (is_pes_header(m_pes))
				{
					m_status = ps_pes; //�޸ĵ�ǰ״̬Ϊps_pes
				}

				if (m_status == ps_pes)
				{
					if (b == 0xbe || b == 0xbf)
					{
						m_nPESIndicator += PES_packet_length + sizeof(pes_header_t);
						continue;
					}

					if (m_pes->stream_id == 0xC0)
					{
						int dummy = 1;
					}


					if ((m_nPESIndicator + PES_packet_length + sizeof(pes_header_t)) <= m_nPSWrtiePos)
					{
						m_nPESLength = PES_packet_length + sizeof(pes_header_t);
						memcpy(m_pPESBuffer, m_pes, m_nPESLength);

						PSStatus status = pes_type(m_pes);

						char* next = (m_pPSBuffer + m_nPESIndicator + sizeof(pes_header_t) + PES_packet_length);

						m_ps = (ps_header_t*)next;
						m_pes = (pes_header_t*)next;

						//��λ��һ��pes ���� ps
						m_nPESIndicator += m_nPESLength;

						m_status = ps_pes;

						return pes_tuple(true, status, (pes_header_t*)m_pPESBuffer, false);
					}
					else//PES֡���ݲ��������������Ѿ�����β��
					{
						int remain = m_nPSWrtiePos - m_nPESIndicator;

						if (m_bRealTime) //ʵʱ���������������
						{
							m_nPESLength = /*PES_packet_length + sizeof(pes_header_t)*/remain;
							memcpy(m_pPESBuffer, m_pes, m_nPESLength);

							PSStatus status = pes_type(m_pes);

							m_ps = NULL;
							m_pes = NULL;

							m_nPESIndicator = 0;
							m_nPSWrtiePos = 0;

							m_status = ps_pes;

							return pes_tuple(true, status, (pes_header_t*)m_pPESBuffer, true);
						}
						else //��ʣ�������Ƶ�������ͷ����������һ�ν���
						{

							if (m_nPESIndicator > 0) //���������ж�
							{
								memmove(m_pPSBuffer, m_pPSBuffer + m_nPESIndicator, remain);
							}
							m_nPSWrtiePos = remain;
							m_nPESIndicator = 0;

							m_ps = (ps_header_t*)m_pPSBuffer;
							m_pes = (pes_header_t*)m_pPSBuffer;

							m_status = ps_pes;

							return pes_tuple(false, m_status, NULL, false);
						}
					}


				}// if(m_status == ps_pes)
				else
				{
					m_nPESIndicator += 1;
				}


			} // if(b >= 0xbd && b < 0xf0)
			else
			{
				//��ȡPSM�����������Ŀ��ÿ��������Ϣ
				program_stream_map* psm_hdr = NULL;
				psm_hdr = (program_stream_map*)(m_pPSBuffer + m_nPESIndicator);
				if (is_psm_header(psm_hdr))
				{
					m_status = ps_psm;

					unsigned short psm_packet_len = ntohs(psm_hdr->PackLength.length);

					printf("psm map length: %d \n", psm_packet_len);

					if ((m_nPESIndicator + psm_packet_len + 2 + 4) > m_nPSWrtiePos || psm_packet_len > 36) //��鳤���Ƿ�Խ�� | PSM��ĳ����Ƿ�Ϊ�Ƿ�ֵ
						break;

					unsigned char* p = (unsigned char*)psm_hdr + 6;

					p += 2;
					unsigned short  program_stream_info_length = ntohs(*(unsigned short*)p);
					p += 2;

					unsigned short elementary_stream_map_length = ntohs(*(unsigned short*)p);
					p += 2;

					int nStreamNum = elementary_stream_map_length / 4;
					for (int i = 0; i < nStreamNum; i++)
					{
						unsigned char stream_type;
						unsigned char elementary_stream_id;
						unsigned short elementary_stream_info_length;

						stream_type = *(unsigned char*)p;
						elementary_stream_id = *(unsigned char*)(p + 1);
						elementary_stream_info_length = ntohs(*(unsigned short*)(p + 2));

						p += 4;
						p += elementary_stream_info_length;

						if (!FindESStreamInfo(elementary_stream_id))
						{
							program_map_info mapInfo;
							mapInfo.elementary_stream_id = elementary_stream_id;
							mapInfo.stream_type = stream_type;

							m_ProgramMap.push_back(mapInfo);
						}

					}

					m_nPESIndicator += psm_packet_len + 2 + 4;

					continue;
				}

				m_nPESIndicator += 4;
			}

		}//while

		m_nPSWrtiePos = 0;
		m_nPESIndicator = 0;

		m_status = ps_padding;

	END_FIND_PAYLOAD:
		return pes_tuple(false, ps_padding, NULL, false);

	}

	naked_tuple naked_payload()
	{
		naked_tuple tuple = naked_tuple(false, 0, 0, 0, 0, NULL, 0, false);
		do
		{
			pes_tuple t = pes_payload();
			if (!t.bContainData)
			{
				break;
			}
			PSStatus status = t.nStatus;
			pes_header_t* pes = t.pPESPtr;
			optional_pes_header* option = (optional_pes_header*)((char*)pes + sizeof(pes_header_t));
			if (option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0)
			{
				break;
			}
			unsigned __int64 pts = get_pts(option);
			unsigned __int64 dts = get_dts(option);
			unsigned char stream_id = pes->stream_id;
			unsigned short PES_packet_length = ntohs(pes->PES_packet_length); //������PESͷ�ĳ���
			char* pESBuffer = ((char*)option + sizeof(optional_pes_header) + option->PES_header_data_length);
			int nESLength = 0;

			if (m_bRealTime && t.bDataLost)
			{
				nESLength = (int)m_nPESLength - sizeof(pes_header_t) - (sizeof(optional_pes_header) + option->PES_header_data_length);
			}
			else
			{
				nESLength = (int)PES_packet_length - (sizeof(optional_pes_header) + option->PES_header_data_length);
			}

			if (m_nESLength + nESLength > MAX_ES_LENGTH || nESLength < 0) //����Ƿ񳬳���������С
			{
				ATLASSERT(0);
				break;
			}

			memcpy(m_pESBuffer + m_nESLength, pESBuffer, nESLength);
			m_nESLength += nESLength;
			if ((stream_id == 0xE0 || stream_id == 0xE2) && (status == ps_ps || status == ps_pes_video))
			{
				tuple = naked_tuple(true, 0xE0, pts, dts, option->PTS_DTS_flags, m_pESBuffer, m_nESLength, t.bDataLost);
				m_nESLength = 0;
			}
			else if (stream_id == 0xC0)
			{
				tuple = naked_tuple(true, 0xC0, pts, dts, option->PTS_DTS_flags, m_pESBuffer, m_nESLength, false);
				m_nESLength = 0;
			}
		} while (false);
		return tuple;
	}

	bool FindESStreamInfo(unsigned char stream_id)
	{
		std::vector<program_map_info>::iterator it;
		for (it = m_ProgramMap.begin(); it != m_ProgramMap.end(); it++)
		{
			if (it->elementary_stream_id == stream_id)
			{
				return true;
			}
		}
		return false;
	}

private:
	PSStatus      m_status;                     //��ǰ״̬

	//char          m_pPSBuffer[MAX_PS_LENGTH];   //PS������
	char* m_pPSBuffer;

	unsigned int  m_nPSWrtiePos;                //PSд��λ��
	unsigned int  m_nPESIndicator;              //PESָ��
private:
	// char          m_pPESBuffer[MAX_PES_LENGTH]; //PES������
	char* m_pPESBuffer;

	unsigned int  m_nPESLength;                 //PES���ݳ���
private:
	ps_header_t* m_ps;                         //PSͷ
	sh_header_t* m_sh;                         //ϵͳͷ
	//psm_header_t* m_psm;                        //��Ŀ��ͷ
	pes_header_t* m_pes;                        //PESͷ
private:
	// char         m_pESBuffer[MAX_ES_LENGTH];    //������
	char* m_pESBuffer;

	unsigned int		m_nESLength;               //����������

	bool   m_bRealTime; //�Ƿ�Ϊʵʱ��

public:
	std::vector<program_map_info> m_ProgramMap;
};

#endif

