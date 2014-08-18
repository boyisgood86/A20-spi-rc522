
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <malloc.h> 
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "spidev_nfc.h" 

#define  SPIDEV            "/dev/spidev1.0"
#define  EXISTMODE         F_OK
#define  MODE              O_RDWR

static int spi_fd = -1 ;
/*
** Function : spi_open
** Note     : open spidev1.0 node for transfer 
** return   : success return open fd else return -1 
*/
int 
spi_open()
{
	if(access(SPIDEV,EXISTMODE) < 0) return -1 ; 
	if((spi_fd = open(SPIDEV,MODE)) < 0) return -1 ;

	return spi_fd ;
}

/*
** Function : spi_close
** Note     : close spidev1.0 opened fd
** return   : success return 0 else return -1
*/
int 
spi_close()
{
	if(spi_fd == -1) return -1 ;
	close(spi_fd) ;
	return 0 ;
}


/*
** Function   : spi_read
** Note       : read message frome spidev
** return     : success return read size else return -1
*/

int 
spi_read(void* buf, int size)
{
	if((spi_fd == -1) || (buf == NULL) || (size <= 0)) return -1 ;
	int size_read = read(spi_fd,buf,size) ;
	if(size_read < 0) return -1 ;
	return size_read ;
}

/*
** Function   :spi_write
** Note       :write message to spidev
** return     :success return write size else return -1
*/
int 
spi_write(void* buf ,int size) 
{
	if((spi_fd == -1) || (NULL == buf) || (size <= 0)) return -1 ;
	int size_write = write(spi_fd,buf,size) ;
	if(size_write < 0) return -1 ;
	return size_write ;
}


