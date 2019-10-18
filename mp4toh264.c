/****************************************************
camera -> openRTSP -> RTP file -> -RTP head -> +MP4 head -> .MP4 file

黑色塑料摄像头h264头数据：

 -- LEN -- NAL-固定值
|		  |
00 00 01 1D 61 E0 08	->	00 00 00 01 61 E0 08
				  ..
00 00 00 E7 61 E0 E8	->	00 00 00 01 61 E0 08

00 00 00 0E 67 4D 00	->	00 00 00 01 67 4D 00
00 00 00 04 68 EE 3C	->	00 00 00 01 68 EE 3C
00 00 00 05 06 E5 01	->	00 00 00 01 06 E5 01
00 00 A1 15 65 B8 XX XX	->	00 00 00 01 65 B8 XX XX

常用的NAL头的取值如下：
0x67: SPS
0x68: PPS
0x65: IDR
0x61: non-IDR Slice
0x01: B Slice
0x06: SEI
0x09: AU Delimiter

黄色金属摄像头h264头数据：
00 00 xx xx 41 E1/E0

****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MP4_FILE		"bad-4m.mp4"
#define H264_FILE		"bad-4m.h264"

#define PARA_NUM		3

int main(int argc, char **argv)
{
	int i=0;
	int ret=0;
	int date_len=0;
	FILE * pFile;
	FILE * pFile_out;
	unsigned char *buffer = NULL;
	unsigned char *p_buffer = NULL;

	if(PARA_NUM != argc){
		printf("please input 2 parameters! ps: .exe in.mp4 out.h264\n");
		return -1;
	}
	printf("eason %d,%s,%s\n",argc, argv[1], argv[2]);


	if((pFile = fopen(argv[1], "r"))==NULL)
	{
		printf("cant open the file %s\n",argv[1]);
		exit(-1);
	}

	struct stat statbuf;
	stat(argv[1], &statbuf);
	int size=statbuf.st_size;
	printf("mp4 file size=%d byte\n",size);

	buffer = (unsigned char *)malloc(size);
	if(NULL == buffer){
		printf("Fail to malloc size=%d byte\n",size);
		goto end;
	}
	memset(buffer,0,size);


	ret = fread (buffer , 1, size, pFile);
	printf("read size=%d\n", ret);
	if(ret != size){
		printf("Fail to fread size=%d\n",size);
		goto end;
	}

#if 0
	#define READ_LEN		4096
	//print data
	for(i=0;i<READ_LEN;i++){
		printf("%02X ",buffer[i]);
	}
	printf("\n");
#endif

	if((pFile_out = fopen(argv[2], "wb"))==NULL)
	{
		printf("cant open the file %s\n", argv[2]);
		goto end;
	}
	p_buffer = buffer;

	for(i=0; i<size-3; i++){
		if((0x00 == buffer[i] && 0x00 == buffer[i+1]) &&
			((0x61 == buffer[i+4] && 0xE0 == buffer[i+5]) ||
			 (0x67 == buffer[i+4] && 0x4D == buffer[i+5] && 0x00 == buffer[i+6]) ||
			 (0x68 == buffer[i+4] && 0xEE == buffer[i+5] && 0x3C == buffer[i+6]) ||
			 (0x06 == buffer[i+4] && 0xE5 == buffer[i+5] && 0x01 == buffer[i+6]) ||
			 (0x65 == buffer[i+4] && 0xB8 == buffer[i+5])))
		{
			date_len = ((buffer[i+2]<<8) | buffer[i+3]) + 4;
			buffer[i+2] = 0x00;
			buffer[i+3] = 0x01;

			ret = fwrite(p_buffer+i, 1, date_len, pFile_out);
			//printf("ret=%02X\n",ret);
			if(ret != date_len)
			{
				printf("Fail to fwrite size=%d\n",date_len);
				break;
			}
		}
		else if(0x00 == buffer[i+2] && 0x18 == buffer[i+3] && 0x66 == buffer[i+4])
		{
			printf("over index=%x, flag: 00 00 00 18 66\n",i);
			break;
		}
	}

	fclose(pFile_out);

end:
	fclose(pFile);
	if(NULL != buffer){
		free(buffer);
		buffer = NULL;
	}
	return 0;
}

