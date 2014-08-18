

#ifndef __SPIDEV_NFC_H__
#define __SPIDEV__NFC_H__ 

extern int spi_open() ;
extern int spi_close() ;

extern int spi_read(void*,int) ;
extern int spi_write(void*,int) ;

#endif 
