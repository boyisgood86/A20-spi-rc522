/*
 * main.c
 *
 *
 *  Created on: 14.08.2013
 *      Author: alexs
 */
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/wait.h>
#include <signal.h>
#include "rfid.h"
//#include "bcm2835.h"
//#include "config.h"
#include "spidev_nfc.h" 
#include "spidev.h" 
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include "spidev.h"


void usage(char *);
char *KeyA="Test01";
char *KeyB="Test10";
uint8_t debug=0;

unsigned char Tmpbuff[20] ;
//unsigned char key[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
//unsigned char key[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//unsigned char key[6] = {0x06, 0x40, 0x00, 0x00, 0x00, 0x00};

int main(int argc, char *argv[]) 
{

	uid_t uid;
	uint8_t SN[10];
	uint16_t CType=0;
	uint8_t SN_len=0;
	char status;
	int tmp,i;

	char str[255];
	char *p;
	char sn_str[23];
	pid_t child;
	int max_page=0;
	uint8_t page_step=0;

	FILE * fmem_str;
	char save_mem=0;
	char fmem_path[255];
	uint8_t use_gpio=0;
	uint8_t gpio=255;
	uint32_t spi_speed=10000000L;
	int fd = -1 ;

//by myself
	unsigned char blockaddr = (unsigned char)atoi(argv[1]);
	unsigned char key_passwd = (unsigned char)atoi(argv[2]) ;
	int n = 0;
	unsigned char statu = 0;
	unsigned char data_test[16];

	for(n = 0; n < 6; n++) {
		data_test[n] = 0x00;
	}
	data_test[6] = 0xff ;
	data_test[7] = 0x07;
	data_test[8] = 0x80 ;
	data_test[9] = 0x69 ;

	for(n=10;n<16;n++){
		data_test[n] = 0xFF ;
	}
	
#if 1
	if(argc < 3) {
		printf("command error and try again like this : ./a.out 1 \n");
		return -1;
	}
#endif
	if( (fd = spi_open() )< 0) return -1 ;
	/*
	 * spi mode
	 */
	unsigned char mode = 3;
//	unsigned char mode = 1;
	unsigned char mode_t ;
	unsigned char bits = 8 ;
	unsigned int  speed = 120000000 ;
	int ret = -1 ;
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		printf("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode_t);
	if (ret == -1)
		printf("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("can't set bits per word");

	unsigned char bits_t ;
	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits_t);
	if (ret == -1)
		printf("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("can't set max speed hz");

	unsigned int  speed_t ;
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed_t);
	if (ret == -1)
		printf("can't get max speed hz");

	printf("spidev hw info : mode:%d  bits:%d speed:%d\n",mode_t,bits_t,speed_t) ;

//many changes in InitRc522 function, please check it.	
	printf("we will read block [%d] value !\n",blockaddr);

	printf("rc522 is soft_rest ....\n");
	PcdReset();
#if 1	
	printf("rc522 open antenna ... \n");
	usleep(100*1000);
	PcdAntennaOn();
	
	memset(Tmpbuff,0,20);

	printf("now find tag .....\n");
	while(1) {
	 statu = PcdRequest(PICC_REQALL,Tmpbuff);
	 if(statu == TAG_OK) {
	 	printf("find the tag and the tag type is :");
		for(n = 0; n < (sizeof(Tmpbuff) / sizeof(Tmpbuff[0])); n++) {
			printf("0x%x ",Tmpbuff[n]);
		}
		printf("\n");
	 }else {
	 	continue ;
	 }
	 
	 printf("Now is running Anticoll ....\n");
	 statu = PcdAnticoll(PICC_ANTICOLL1, Tmpbuff);
	 if(statu == TAG_OK) {
	 	printf("Anticoll success and the tag sn is ");
		for(n = 0; n < (sizeof(Tmpbuff) / sizeof(Tmpbuff[0])); n++) {
			printf("0x%x ",Tmpbuff[n]);
		}
		printf("\n");
	 }else {
		printf("Anticoll faild !\n");
	 	continue ;
	 }

	 printf("Now start select an tag .... \n");
	 statu = PcdSelect(PICC_ANTICOLL1, Tmpbuff);
	 if(statu == TAG_OK) {
	 	printf("select tag is success !\n");
	 }else {
	 	printf("sorry,select tag faild !\n");
		continue ;
	 }

	 printf("Now start authstate ... \n");
	 statu = PcdAuthState(PICC_AUTHENT1A, blockaddr, key, Tmpbuff);
	// statu = PcdAuthState(PICC_AUTHENT1B, blockaddr, key, Tmpbuff);
	 if(statu == TAG_OK) {
	 	printf("authstate success !\n");
	 }else {
	 	printf("authstate faild and continue find tag....\n");
		continue ;
	 }
#if 1
	 if(key_passwd == 3 ) {
		 printf("Now start write [%d] blockaddr... \n",key_passwd);
		 statu = PcdWrite(key_passwd, data_test);
		 if(statu == TAG_OK) {
				printf("write success !\n");
		 }else {
		 	printf("write faild !\n");
			continue ;
		 }

		statu = PcdRead(key_passwd,Tmpbuff);
	 if(statu == TAG_OK) {
	 	printf("read success and the data is ");
		for(n = 0; n < 16; n++) {
			printf("0x%x ",Tmpbuff[n]);
		}
		printf("\n");
	 }else {
	 	printf("read faild and continue find tag ....\n");
		continue ;
	 }

	 }else {
		 printf("[%d] address is only read !\n",blockaddr);
	 }
#endif
	 printf("Now start read [%d] addresss ... \n",blockaddr);
	 statu = PcdRead(blockaddr,Tmpbuff);
	 if(statu == TAG_OK) {
	 	printf("read success and the data is ");
		for(n = 0; n < 16; n++) {
			printf("0x%x ",Tmpbuff[n]);
		}
		printf("\n");
	 }else {
	 	printf("read faild and continue find tag ....\n");
		continue ;
	 }

	 printf("Now sleep the picc \n");
	 PcdHalt();
	 break ;
	}

//	InitRc522();
#if 0

	while(1) {
		status= find_tag(&CType);
		printf("status is [%d] :Ctype [%x] \n",status,CType) ;
		if (status==TAG_NOTAG) {
			printf("find tag again !\n") ;
			usleep(200000);
			continue;
		}else if ((status!=TAG_OK)&&(status!=TAG_COLLISION)) {
			printf("status!=TAG_OK)&&(status!=TAG_COLLISION again \n") ;
			continue;
		}
		printf("Ctype is %d\n",CType);
		if (select_tag_sn(SN,&SN_len)!=TAG_OK) {
			printf("select_tag_sn(SN,&SN_len)!=TAG_OK) again \n") ;
			continue;
		}

		read_tag_str(0,str);
		printf("str:%s\n",str) ;
		PcdHalt();
		break  ;
	}
#endif
#endif
	printf("now start close spi orpe.....\n");
	spi_close() ; 

	return 0;

}


