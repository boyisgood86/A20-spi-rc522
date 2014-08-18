
#include <string.h>
#include <stdio.h>
#include <unistd.h>
//#include "bcm2835.h"
#include "rc522.h"
#include "main.h"
#include "spidev_nfc.h"
//#define  FUNCTION()  printf("in function [%s] \n",__func__) 
#define  FUNCTION()   

void InitRc522(void)
{
	FUNCTION() ;
// in PcdRest function many changes 	
	PcdReset();
//	PcdAntennaOn();
}

char PcdRequest(uint8_t req_code,uint8_t *pTagType)
{
	FUNCTION() ;
	char   status;
	uint8_t   unLen;
	uint8_t   ucComMF522Buf[MAXRLEN];

	WriteRawRC(BitFramingReg,0x07);
	ucComMF522Buf[0] = req_code;

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);
	if ((status == TAG_OK) && (unLen == 0x10))
	{
		*pTagType     = ucComMF522Buf[0];
		*(pTagType+1) = ucComMF522Buf[1];
	}
	else if (status == TAG_COLLISION) {
//		printf("ATQA %02x%02x\n",ucComMF522Buf[0],ucComMF522Buf[1]);
	}
	else if (status!=TAG_NOTAG) {
		status = TAG_ERR;
	}

	return status;
}

char PcdAnticoll(uint8_t cascade, uint8_t *pSnr)
{
	FUNCTION() ;
	char   status;
	uint8_t   i,snr_check=0;
	uint8_t   unLen;
	uint8_t   ucComMF522Buf[MAXRLEN];
	uint8_t   pass=32;
	uint8_t	  collbits=0;

	i=0;
	WriteRawRC(BitFramingReg,0x00);
	do {
		ucComMF522Buf[0] = cascade;
		ucComMF522Buf[1] = 0x20+collbits;
		//	WriteRawRC(0x0e,0);
		status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2+i,ucComMF522Buf,&unLen);
		if (status == TAG_COLLISION) {
			collbits=ReadRawRC(CollReg)&0x1f;
			if (collbits==0) collbits=32;
			i=(collbits-1)/8 +1;
//			printf ("--- %02x %02x %02x %02x %d\n",ucComMF522Buf[0],ucComMF522Buf[1],ucComMF522Buf[2],ucComMF522Buf[3],unLen);
			ucComMF522Buf[i-1]|=(1<<((collbits-1)%8));
			ucComMF522Buf[5]=ucComMF522Buf[3];
			ucComMF522Buf[4]=ucComMF522Buf[2];
			ucComMF522Buf[3]=ucComMF522Buf[1];
			ucComMF522Buf[2]=ucComMF522Buf[0];
			WriteRawRC(BitFramingReg,(collbits % 8));
//			printf (" %d %d %02x %d\n",collbits,i,ucComMF522Buf[i+1],collbits % 8);
		}
	} while (((--pass)>0)&&(status==TAG_COLLISION));

	if (status == TAG_OK)
	{
		for (i=0; i<4; i++)
		{
			*(pSnr+i)  = ucComMF522Buf[i];
			snr_check ^= ucComMF522Buf[i];
		}
		if (snr_check != ucComMF522Buf[i])
		{   status = TAG_ERR;    }
	}

	return status;
}

char PcdSelect(uint8_t cascade, uint8_t *pSnr)
{
	FUNCTION() ;
	char   status;
	uint8_t   i;
	uint8_t   unLen;
	uint8_t   ucComMF522Buf[MAXRLEN];

	ucComMF522Buf[0] = cascade;
	ucComMF522Buf[1] = 0x70;
	ucComMF522Buf[6] = 0;
	for (i=0; i<4; i++)
	{
		ucComMF522Buf[i+2] = *(pSnr+i);
		ucComMF522Buf[6]  ^= *(pSnr+i);
	}
	CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);

	ClearBitMask(Status2Reg,0x08);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);

	if ((status == TAG_OK) && (unLen == 0x18))
	{   status = TAG_OK;  }
	else
	{   status = TAG_ERR;    }

	return status;
}

char PcdAuthState(uint8_t   auth_mode,uint8_t   addr,uint8_t *pKey,uint8_t *pSnr)
{
	FUNCTION() ;
	char   status;
	uint8_t   unLen;
	uint8_t   ucComMF522Buf[MAXRLEN];

	ucComMF522Buf[0] = auth_mode;
	ucComMF522Buf[1] = addr;
	memcpy(&ucComMF522Buf[2], pKey, 6);
	memcpy(&ucComMF522Buf[8], pSnr, 4);

	status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
	if ((status != TAG_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
	{   status = TAG_ERR;   }

	return status;
}

char PcdRead(uint8_t addr,uint8_t *p )
{
	FUNCTION() ;
	char   status;
	uint8_t   unLen;
	uint8_t   i,ucComMF522Buf[MAXRLEN];
	uint8_t   CRC_buff[2];

	memset(ucComMF522Buf,0,sizeof(ucComMF522Buf));
	ucComMF522Buf[0] = PICC_READ;
	ucComMF522Buf[1] = addr;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
	CalulateCRC(ucComMF522Buf,16,CRC_buff);
	//	printf("debug %02x%02x %02x%02x   ",ucComMF522Buf[16],ucComMF522Buf[17],CRC_buff[0],CRC_buff[1]);

	if ((status == TAG_OK) && (unLen == 0x90))
	{
		if ((CRC_buff[0]!=ucComMF522Buf[16])||(CRC_buff[1]!=ucComMF522Buf[17])) { status = TAG_ERRCRC; }
		for (i=0; i<16; i++)
		{    *(p +i) = ucComMF522Buf[i];   }
	}
	else
	{   status = TAG_ERR;   }

	return status;
}

char PcdWrite(uint8_t   addr,uint8_t *p )
{
	FUNCTION() ;
	char   status;
	uint8_t   unLen;
	uint8_t   i,ucComMF522Buf[MAXRLEN];

	ucComMF522Buf[0] = PICC_WRITE;
	ucComMF522Buf[1] = addr;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

	if ((status != TAG_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
	{   status = TAG_ERR;   }

	if (status == TAG_OK)
	{
		for (i=0; i<16; i++)
		{
			ucComMF522Buf[i] = *(p +i);
		}
		CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

		status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
		if ((status != TAG_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
		{   status = TAG_ERR;   }
	}

	return status;
}

char PcdHalt(void)
{
	FUNCTION() ;
	uint8_t status;
	uint8_t unLen;
	uint8_t ucComMF522Buf[MAXRLEN];

	ucComMF522Buf[0] = PICC_HALT;
	ucComMF522Buf[1] = 0;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

	return status;
}

void CalulateCRC(uint8_t *pIn ,uint8_t   len,uint8_t *pOut )
{
	FUNCTION() ;
	uint8_t   i,n;
	ClearBitMask(DivIrqReg,0x04);
	WriteRawRC(CommandReg,PCD_IDLE);
	SetBitMask(FIFOLevelReg,0x80);
	for (i=0; i<len; i++)
	{   WriteRawRC(FIFODataReg, *(pIn +i));   }
	WriteRawRC(CommandReg, PCD_CALCCRC);
	i = 0xFF;
	do
	{
		n = ReadRawRC(DivIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x04));
	pOut [0] = ReadRawRC(CRCResultRegL);
	pOut [1] = ReadRawRC(CRCResultRegM);
}

char PcdReset(void)
{
	uint8_t i = 0;

	FUNCTION() ;
	i = ReadRawRC(ComIEnReg);
	printf("before soft reset   0x02 address value is 0x%x\n",i);
	WriteRawRC(CommandReg,PCD_RESETPHASE);
	usleep(10000);
// this is by myself
	i = ReadRawRC(ComIEnReg);
	if (i == 0x80) {
		printf("read success  and after reset 0x02 value is 0x%x\n",i);
	}else {
		printf("after reset 0x02 value is 0x%x , not is 0x80",i);
	}
#if 0
	i &= 0x0F;
	
	if(i == PCD_IDLE) {
		printf("now a20 tran cancle command : 0x%x \n",PCD_IDLE);
	}else if(i == PCD_AUTHENT) {
		printf("now a20 tran authent command : 0x%x\n",PCD_AUTHENT);
	}else if(i == PCD_RECEIVE) {
		printf("now a20 tran receive command : 0x%x\n",PCD_RECEIVE);
	}else if(i == PCD_TRANSMIT) {
		printf("now a20 tran transmit command : 0x%x\n",PCD_TRANSMIT);
	}else if(i == PCD_RESETPHASE) {
		printf("now a20 tran so_reset command : 0x%x\n",PCD_RESETPHASE);
	}else if(i == PCD_CALCCRC) {
		printf("now a20 tran calccrc command: 0x%x\n",PCD_CALCCRC);
	}else {
		printf("unkonw a20 command,check spi read/write code\n");
	}

#endif

#if 1

	ClearBitMask(TxControlReg,0x03);
	usleep(10000);

	SetBitMask(TxControlReg,0x03);
	WriteRawRC(TModeReg,0x8D);
	WriteRawRC(TPrescalerReg,0x3E);
	WriteRawRC(TReloadRegL,30);
	WriteRawRC(TReloadRegH,0);
	WriteRawRC(TxASKReg,0x40);
	WriteRawRC(ModeReg,0x3D);            //6363
	//	WriteRawRC(DivlEnReg,0x90);
	WriteRawRC(RxThresholdReg,0x84);
	WriteRawRC(RFCfgReg,0x68);
	WriteRawRC(GsNReg,0xff);
	WriteRawRC(CWGsCfgReg,0x2f);
	//	WriteRawRC(ModWidthReg,0x2f);
#endif
	return TAG_OK;
}

/*
char M500PcdConfigISOType(uint8_t   type)
{
	if (type == 'A')
	{
		ClearBitMask(Status2Reg,0x08);
		WriteRawRC(ModeReg,0x3D);
		WriteRawRC(RxSelReg,0x86);
		WriteRawRC(RFCfgReg,0x7F);
		WriteRawRC(TReloadRegL,30);
		WriteRawRC(TReloadRegH,0);
		WriteRawRC(TModeReg,0x8D);
		WriteRawRC(TPrescalerReg,0x3E);
		PcdAntennaOn();
	}
	else{ return 1; }

	return TAG_OK;
}
 */

uint8_t ReadRawRC(uint8_t Address)
{
	char buff[2];
	int size = 0;

	memset(buff,0,2);
	buff[0] = ((Address<<1)&0x7E)|0x80;
	//bcm2835_spi_transfern(buff,2);
//	printf("=======before read buff[0]:%x buff[1]:%x\n",buff[0],buff[1]) ;
	if(spi_read(buff,2) < 0) {
		printf("spi read faild !--------------------------------------->\n");
		return -1;
	}
//	spi_write(buff[0],1);
//	spi_read(buff[0],1);
	return (uint8_t)buff[1];
}

void WriteRawRC(uint8_t Address, uint8_t value)
{
	char buff[2];
	memset(buff,0,2);

	buff[0] = (char)((Address<<1)&0x7E);
	buff[1] = (char)value;
	//bcm2835_spi_transfern(buff,2);
	if(spi_write(buff,2) <0) {
		printf("spi write faild !--------------------------------------->\n");
		return;
	}
//	spi_write(buff[0],1);
//	spi_write(buff[1],1);
}

void SetBitMask(uint8_t   reg,uint8_t   mask)
{
	FUNCTION() ;
	char   tmp = 0x0;
	tmp = ReadRawRC(reg);
//	printf("tmp --------------> by jaosn 0x%x\n",tmp);
	WriteRawRC(reg,tmp | mask);  // set bit mask
}

void ClearBitMask(uint8_t   reg,uint8_t   mask)
{
	FUNCTION() ;
	char   tmp = 0x0;
	tmp = ReadRawRC(reg);
	WriteRawRC(reg, tmp & ~mask);  // clear bit mask
}

char PcdComMF522(uint8_t   Command,
		uint8_t *pIn ,
		uint8_t   InLenByte,
		uint8_t *pOut ,
		uint8_t *pOutLenBit)
{
	FUNCTION() ;
	char   status = TAG_ERR;
	uint8_t   irqEn   = 0x00;
	uint8_t   waitFor = 0x00;
	uint8_t   lastBits;
	uint8_t   n;
	uint32_t   i;
	uint8_t PcdErr;

	//	printf("CMD %02x\n",pIn[0]);
	switch (Command)
	{
	case PCD_AUTHENT:
		irqEn   = 0x12;
		waitFor = 0x10;
		break;
	case PCD_TRANSCEIVE:
		irqEn   = 0x77;
		waitFor = 0x30;
		break;
	default:
		break;
	}

	WriteRawRC(ComIEnReg,irqEn|0x80);
	//	WriteRawRC(ComIEnReg,irqEn);
	ClearBitMask(ComIrqReg,0x80);
	SetBitMask(FIFOLevelReg,0x80);
	WriteRawRC(CommandReg,PCD_IDLE);

	for (i=0; i<InLenByte; i++) {
		WriteRawRC(FIFODataReg, pIn [i]);
	}

	WriteRawRC(CommandReg, Command);

	if (Command == PCD_TRANSCEIVE) {
		SetBitMask(BitFramingReg,0x80);
	}

	//i = 600;//���ʱ��Ƶ�ʵ������M1�����ȴ�ʱ��25ms
	i = 150;
	do
	{
		usleep(200);
		//		bcm2835_delayMicroseconds(200);
		n = ReadRawRC(ComIrqReg);
		i--;
	}
	while ((i!=0) && (!(n&0x01)) && (!(n&waitFor)));

	ClearBitMask(BitFramingReg,0x80);

	if (i!=0)
	{
		PcdErr=ReadRawRC(ErrorReg);
		if (!(PcdErr & 0x11))
		{
			status = TAG_OK;
			if (n & irqEn & 0x01) {status = TAG_NOTAG;}
			if (Command == PCD_TRANSCEIVE) {
				n = ReadRawRC(FIFOLevelReg);
				lastBits = ReadRawRC(ControlReg) & 0x07;
				if (lastBits) {*pOutLenBit = (n-1)*8 + lastBits;}
				else {*pOutLenBit = n*8;}

				if (n == 0) {n = 1;}
				if (n > MAXRLEN) {n = MAXRLEN;}

				for (i=0; i<n; i++) {
					pOut [i] = ReadRawRC(FIFODataReg);
//					printf (".%02X ",pOut[i]);
				}
			}
		}
		else {
			//			fprintf (stderr,"Err %02x\n",PcdErr);
			status = TAG_ERR;}

		if (PcdErr&0x08) {
			if (debug) fprintf (stderr,"COllision \n");
			status = TAG_COLLISION;

		}

	}


	//    SetBitMask(ControlReg,0x80);           // stop timer now
	//    WriteRawRC(CommandReg,PCD_IDLE); ???????
//	printf ("PCD Err %02x\n",PcdErr);
	return status;
}

void PcdAntennaOn(void)
{
	FUNCTION() ;
	uint8_t   i;
	i = ReadRawRC(TxControlReg);
//	printf("PCdAntennaON i = 0x%x !!!!!!!!!!!!!!!!!!!! by jason\n",i);
	if (!(i & 0x03))
	{
		SetBitMask(TxControlReg, 0x03);
		//printf("set antenna again !!!!!   by jason \n");
	}
//	i = ReadRawRC(TxControlReg);
	//printf("in function PcdAntennaOn  i = %x ------------>  by jason\n",i);
	//if((i & 0x03) != 0x03) {
		//printf("tian xian open faild !\n");
	//}
}

void PcdAntennaOff(void)
{
	FUNCTION() ;
	ClearBitMask(TxControlReg, 0x03);
}
