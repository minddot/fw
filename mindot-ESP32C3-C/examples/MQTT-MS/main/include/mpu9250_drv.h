#ifndef __MPU9250_DRIVER_H_
#define __MPU9250_DRIVER_H_



void MPU9250_init();
unsigned int MPU9250_get_FIFO_count();
void MPU9250_read_FIFO(unsigned char * buf, unsigned int buf_len);
void MPU9250_read_accel(unsigned char * buf, unsigned int buf_len);
void MPU9250_read_temp(unsigned char * buf, unsigned int buf_len);

#endif
