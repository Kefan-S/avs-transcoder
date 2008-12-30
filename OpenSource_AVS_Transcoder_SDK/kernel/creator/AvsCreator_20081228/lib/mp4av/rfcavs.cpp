
#include <mp4av_common.h>
#include <mp4av_avs.h>




extern "C" MP4TrackId MP4AV_AVS_HintTrackCreate (MP4FileHandle mp4File,
												 MP4TrackId mediaTrackId)
{
	MP4TrackId hintTrackId =
		MP4AddHintTrack(mp4File, mediaTrackId);	//���hint��ص�ԭ��mdia.minf.hmhd;mdia.minf.stbl.stsd.rtp
	//mdia.minf.stbl.stsd.rtp .tims.timeScale;tref.hint;udta.hnti.sdp
	//udta.hinf
	
	if (hintTrackId == MP4_INVALID_TRACK_ID) {					
		return MP4_INVALID_TRACK_ID;
	}
	
	u_int8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;
	
	// don't include mpeg4-esid
	MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
		"AVS1-P2", &payloadNumber, 0,						//�ı����ơ�H264��
		NULL, true, false);
				
	// get the mpeg4 video configuration 
	u_int8_t **pSeq, **pPict ;
	u_int32_t *pSeqSize, *pPictSize;								//�����ı�
	char *base64;
	uint32_t profile_level;
	char *sprop = NULL;
	uint32_t ix = 0;
	
	MP4GetTrackAVSSeqHeaders(mp4File,
		mediaTrackId,
		&pSeq,
		&pSeqSize);										//**�������ͷ��ز���pSeq��pSeqSize
	
	if (pSeqSize && pSeqSize[0] != 0) {								//���seqͷ
		// we have valid sequence and picture headers
		uint8_t *p = pSeq[0];
		if (*p == 0 && p[1] == 0 && p[2] == 1 ) {
			p += 3;
		}
		profile_level = p[1] << 8 | p[2];
		while (pSeqSize[ix] != 0) {
			base64 = MP4BinaryToBase64(pSeq[ix], pSeqSize[ix]);
			if (sprop == NULL) {
				sprop = strdup(base64);
			} else {
				sprop = (char *)realloc(sprop, strlen(sprop) + strlen(base64) + 1 + 1);
				strcat(sprop, ",");
				strcat(sprop, base64);
			}
			free(base64);
			free(pSeq[ix]);
			ix++;
		}
		free(pSeq);
		free(pSeqSize);
		
		
		// create the appropriate SDP attribute *
		char* sdpBuf = (char*)malloc(strlen(sprop) + 128);				//sdp
		
		u_int16_t svideoWidth = MP4GetTrackVideoWidth(mp4File, mediaTrackId);		
		u_int16_t svideoHeight = MP4GetTrackVideoHeight(mp4File, mediaTrackId);	//********���********
		
		
		sprintf(sdpBuf,
			"a=cliprect:0,0,%d,%d\015\012"
			"a=fmtp:%u profile-level-id=%04x; sprop-parameter-sets=%s; packetization-mode=1\015\012",
			svideoWidth,
			svideoHeight,
			payloadNumber,
			profile_level,
			sprop); 
		
		/* add this to the track's sdp */
		MP4AppendHintTrackSdp(mp4File, hintTrackId, sdpBuf);
		
		free(sprop);
		free(sdpBuf);
	}
	return hintTrackId;
}


static uint32_t avs_get_nal_size (uint8_t *pData,
								  uint32_t sizeLength)
{
	if (sizeLength == 1) {
		return *pData;
	} else if (sizeLength == 2) {
		return (pData[0] << 8) | pData[1];
	} else if (sizeLength == 3) {
		return (pData[0] << 16) |(pData[1] << 8) | pData[2];
	}
	return (pData[0] << 24) |(pData[1] << 16) |(pData[2] << 8) | pData[3];
}



static uint8_t avs_get_sample_nal_type (uint8_t *pSampleBuffer, 
										uint32_t sampleSize,
										uint32_t sizeLength)
{
	uint8_t nal_type = pSampleBuffer[sizeLength] & 0x1f;
	return nal_type;
}




extern "C" void MP4AV_AVS_HintAddSample (MP4FileHandle mp4File,
										 MP4TrackId hintTrackId,
										 MP4SampleId sampleId,
										 uint8_t *pSampleBuffer,
										 uint32_t sampleSize,				//sampleSize������sample���ٸ�byte
										 uint32_t sizeLength,
										 MP4Duration duration,
										 MP4Duration renderingOffset,
										 bool isSyncSample,
										 uint16_t maxPayloadSize)
{
	uint8_t nal_type = avs_get_sample_nal_type(pSampleBuffer, 
		sampleSize, 
		sizeLength);
	bool pic_is_idr = false;
	
	if (nal_type==AVS_NAL_TYPE_I_PIC_HEADER) 
	{
		pic_is_idr=true;
	}
	
	// for now, we don't know if we can drop frames, so don't indiate
	// that any are "b" frames
	bool isBFrame = false;
	uint32_t nal_size;									//nal_size��һ��nalu���ٸ�byte��������ǰ������4byte������naluͷ
	uint32_t offset = 0;
	uint32_t remaining = sampleSize;	
	/*#ifdef DEBUG_H264_HINT			
	printf("hint for sample %d %u\n", sampleId, remaining);
#endif*/
	MP4AddRtpVideoHint(mp4File, hintTrackId, isBFrame, renderingOffset);
	
	if (sampleSize - sizeLength < maxPayloadSize) {				//sampleС��MTU	sizelength�൱����ͷ��Ϣ��������sample�ľ�����				
		uint32_t first_nal = avs_get_nal_size(pSampleBuffer, sizeLength);
		if (first_nal + sizeLength == sampleSize) {					//һ��sampleֻ��һ��nalu����С��MTUʱ�����
			// we have a single nal, less than the maxPayloadSize,	//ֻҪ��һ��rtp���Ϳ�����
			// so, we have Single Nal unit mode
			MP4AddRtpPacket(mp4File, hintTrackId, true);
			MP4AddRtpSampleData(mp4File, hintTrackId, sampleId,
				sizeLength, sampleSize - sizeLength);
			MP4WriteRtpHint(mp4File, hintTrackId, duration, 
				pic_is_idr);//nal_type�ı����
			return;
		}
	}
	
	// TBD should scan for resync markers (if enabled in ES config)
	// and packetize on those boundaries
	while (remaining) {				//sample���ݴ���MTUʱ��ֻҪ���sample�������ݾ�ִ��********sample��**ѭ��*******************
		
		nal_size = avs_get_nal_size(pSampleBuffer + offset,
			sizeLength);								//sizelength=4
		//******* skip the sizeLength								
		/*#ifdef DEBUG_H264_HINT//��ִ��
		printf("offset %u nal size %u remain %u\n", offset, nal_size, remaining);
#endif*/
		
		offset += sizeLength;
		remaining -= sizeLength;
		
		if (nal_size > maxPayloadSize) {				//���nalu_size�����������ֵMTU ��Ҫ�ָ� FU
#ifdef DEBUG_H264_HINT								//FUͷ8bit�����˵��
			printf("fragmentation units\n");
#endif
			uint8_t head = pSampleBuffer[offset];//************************************************************************
			offset++;
			nal_size--;
			remaining--;
			
			uint8_t fu_header[2];
			// uint8_t *final_fu_header;
			fu_header[0] = (head&0xe0)|0x1c;			//FUָʾ
			fu_header[1] = head;						//FUͷ
			fu_header[1] |= 0x80;
			fu_header[1] &= 0x9f;				//100+type5bit
			
			
			while (nal_size > 0) {			//******************ֻҪnalu����ֵ************************nalu��**ѭ��*******************
				
				
				uint32_t write_size;
				if (nal_size + 2 <= maxPayloadSize) {//���nalu���ɼ�naluͷ��FUͷС�ڵ���MTU������ָԪ
					fu_header[1] |= 0x40;//�ָԪ����
					write_size = nal_size;
				} else {
					write_size = maxPayloadSize - 2;//write_size��������ָԪ������payload=MTU-2
				}
#ifdef DEBUG_H264_HINT
				printf("frag off %u write %u left in nal %u remain %u\n",
					offset, write_size, nal_size, remaining);
#endif
				remaining -= write_size;//remainingΪ����һ��MTU�����sampleʣ�������
				
				MP4AddRtpPacket(mp4File, hintTrackId, remaining == 0);
				MP4AddRtpImmediateData(mp4File, hintTrackId, 
					fu_header, 2);
				fu_header[1] &= 0x7f;//��ʾ������һ���ָԪ
				
				MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
					offset, write_size);
				offset += write_size;
				nal_size -= write_size;//nal_sizeΪ���nalu�ָ��ʣ������ݣ�����������ݾ��ڴ�ѭ��������µķָԪ
			}
		}								//************FU****end********
		
		else {												//���nalu_size С�� �������ֵMTU����Ҫ�����Ƿ�򸴺ϰ�stap
			// we have a smaller than MTU nal.  check the next sample			//��仰ʲô��˼,Ϊʲô��next sample��Ӧ����nalu��
			// see if the next one fits;				
			uint32_t next_size_offset;
			bool have_stap = false;
			next_size_offset = offset + nal_size;
			if (next_size_offset < sampleSize) {//if (next_size_offset < remaining) {//�Ƿ�����һ��nalu���У���ִ������ remaining�ĳ�samplesize
				// we have a remaining NAL
				uint32_t next_nal_size = 
					avs_get_nal_size(pSampleBuffer + next_size_offset, sizeLength);
				if (next_nal_size + nal_size + 4 + 1 <= maxPayloadSize) {					//�ж���һ������nalu��С�������Ƿ�С��MTU
					have_stap = true;															//С��MTU�Ļ�,���Ǿ���STAP���ϰ�
				}																		//4 ����NALU�ߴ� 1 һ��STAP-A����ͷ (NALU�ߴ�Ϊ1byte)
			} 
			if (have_stap == false) {									//�������nalu����MTU����STAP,ֱ�Ӱ����nalu���һ��rtp������һ���������ж�
				MP4AddRtpPacket(mp4File, hintTrackId, next_size_offset >= remaining);
				MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
					offset, nal_size);
				offset += nal_size;
				remaining -= nal_size;
			} 
			
			else {													//****************�������NALUС��MTU��ʹ��STAP-A���ϰ�ʵ��******
				uint32_t bytes_in_stap = 1 + 2 + nal_size;					//1byte STAP-A����ͷ;2byte��STAP��nalu�ߴ�:�������nalu(����naluͷ)��С
				
				
				uint8_t max_nri = pSampleBuffer[offset] & 0x60;	//0 11 00000	//**��Ҫ����:һ��sample��������nalu**
				while (next_size_offset <= sampleSize && bytes_in_stap <= maxPayloadSize)//while (next_size_offset < sampleSize && bytes_in_stap <= maxPayloadSize) //remaining�ĳ�samplesize//while (next_size_offset < remaining && bytes_in_stap < maxPayloadSize) 
				{													
					uint8_t nri;										//��һ��stap�ڱȽϳ�nalu����NRI���ȼ����ھ���ͷ��NRI��Ԫ����������ȼ�
					nri = pSampleBuffer[next_size_offset + sizeLength] & 0x60;		//0 11 00000
					if (nri > max_nri) max_nri = nri;
					
					uint32_t next_nal_size = 
						avs_get_nal_size(pSampleBuffer + next_size_offset, sizeLength);
					bytes_in_stap += 2 + next_nal_size;
					next_size_offset += sizeLength + next_nal_size;
				}
				
				//������֤�Ƿ������һ��nalu
				bool last;
				if (next_size_offset > sampleSize)// && bytes_in_stap <= maxPayloadSizey)//if (next_size_offset <= remaining && bytes_in_stap <= maxPayloadSize)		
					// stap is last frame						//�Ѿ��������sample���һ��nalu��M��־λ��1����ʾsample����
				{
					last = true;
				} else last = false;						
				MP4AddRtpPacket(mp4File, hintTrackId, last);//last=trueʱz˵�����stap�����sample���һ��stap����M��־λ��1
				uint8_t data[3];
				data[0] = max_nri | 24;						//stap-A ����ͷ
				data[1] = nal_size >> 8;
				data[2] = nal_size & 0xff;					//NALU�ߴ�2 byte
				MP4AddRtpImmediateData(mp4File, hintTrackId, 
					data, 3);
				MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
					offset, nal_size);
				offset += nal_size;
				remaining -= nal_size;
				bytes_in_stap = 1 + 2 + nal_size;							//1byte STAP-A����ͷ;2byte��STAP��nalu�ߴ�:�������nalu(����naluͷ)��С
				nal_size = avs_get_nal_size(pSampleBuffer + offset, sizeLength);
				while (bytes_in_stap + nal_size + 2 <= maxPayloadSize &&remaining) {			//***������NALU�Ժ�С��MTU�������nalu�ٴ���stap**ѭ��*
					offset += sizeLength;
					remaining -= sizeLength;
					data[0] = nal_size >> 8;
					data[1] = nal_size & 0xff;
					MP4AddRtpImmediateData(mp4File, hintTrackId, data, 2);
					MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, offset, nal_size);
					offset += nal_size;
					remaining -= nal_size;
					bytes_in_stap += nal_size + 2;							//�ٴν�nalu����stapʱ�Ͳ����ټ�1byte����ͷ�ˣ�ֻ��Ҫ��2byte nalu�ߴ�
					if (remaining) {
						nal_size = avs_get_nal_size(pSampleBuffer + offset, sizeLength);
					}
				} // end while stap
			} // end have stap	  
		} // end check size
   }
   MP4WriteRtpHint(mp4File, hintTrackId, duration,				//����sample�����д������
	   pic_is_idr);
   
}


extern "C" bool MP4AV_AVSHinter(
								MP4FileHandle mp4File, 
								MP4TrackId mediaTrackId, 
								u_int16_t maxPayloadSize)
{
	u_int32_t numSamples = MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);
	u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
	
	uint32_t sizeLength;
	
	if (numSamples == 0 || maxSampleSize == 0) {
		return false;
	}
	
	/*if (MP4GetTrackAVSLengthSize(mp4File, mediaTrackId, &sizeLength) == false) {
    return false;
}*/
	sizeLength=4;						//why?
	
	MP4TrackId hintTrackId = 
		MP4AV_AVS_HintTrackCreate(mp4File, mediaTrackId);				//****AVSspecial****
	
	if (hintTrackId == MP4_INVALID_TRACK_ID) {
		return false;
	}
	
	u_int8_t* pSampleBuffer = (u_int8_t*)malloc(maxSampleSize);
	if (pSampleBuffer == NULL) {
		MP4DeleteTrack(mp4File, hintTrackId);
		return false;
	}
	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
		u_int32_t sampleSize = maxSampleSize;
		MP4Timestamp startTime;
		MP4Duration duration;
		MP4Duration renderingOffset;
		bool isSyncSample;//stssָ��ͬ��֡
		
		bool rc = MP4ReadSample(
			mp4File, mediaTrackId, sampleId, 
			&pSampleBuffer, &sampleSize, 
			&startTime, &duration, 
			&renderingOffset, &isSyncSample);
		
		if (!rc) {
			MP4DeleteTrack(mp4File, hintTrackId);
			CHECK_AND_FREE(pSampleBuffer);
			return false;
		}
		
		MP4AV_AVS_HintAddSample(mp4File,								//****AVSspecial****
			hintTrackId,
			sampleId,
			pSampleBuffer,
			sampleSize,
			sizeLength,
			duration,
			renderingOffset,
			isSyncSample,
			maxPayloadSize);
		
	}
	CHECK_AND_FREE(pSampleBuffer);
	
	return true;
}

