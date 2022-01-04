#include "mpu9250_drv.h"

#include <stdio.h>
#include "driver/i2c.h"
#include "i2c-easy.h"
#include "esp_log.h"


#define MPU9250_PWR_MGMT_1_REG     0X6B
#define MPU9250_PWR_MGMT_1_VAL     0x80 

#define MPU9250_PWR_MGMT_2_REG     0X6C
#define MPU9250_PWR_MGMT_2_VAL     0x07

#define MPU9250_WHO_AM_I_REG       0X75

#define MPU9250_USR_CTRL_REG       0X6A  // I2C mode
#define MPU9250_USR_CTRL_VAL       0X44

#define MPU9250_I2C_ADDR           0X68

#define MPU9250_ACCEL_XOUT_H_REG   0X3B
#define MPU9250_ACCEL_XOUT_L_REG   0X3C
#define MPU9250_ACCEL_YOUT_H_REG   0X3D
#define MPU9250_ACCEL_YOUT_L_REG   0X3E
#define MPU9250_ACCEL_ZOUT_H_REG   0X3F
#define MPU9250_ACCEL_ZOUT_L_REG   0X40

#define MPU9250_TEMP_OUT_H_REG     0X41
#define MPU9250_TEMP_OUT_L_REG     0X42


#define I2C_MINDOT_MASTER_SCL_IO          13             /*!< gpio number for I2C master clock */
#define I2C_MINDOT_MASTER_SDA_IO          15               /*!< gpio number for I2C master data  */
#define I2C_MINDOT_MASTER_NUM             I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_MINDOT_MASTER_TX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_MINDOT_MASTER_RX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_MINDOT_MASTER_FREQ_HZ         100000           /*!< I2C master clock frequency */


#define MPU9250_SMPLRT_DIV_REG            0x19     
#define MPU9250_SMPLRT_DIV_VAL           0x07

#define MPU9250_CONFIG_REG  0x1A
#define MPU9250_CONFIG_VAL  0x06

#define MPU9250_ACCEL_CONFIG_1_REG  0x1C
#define MPU9250_ACCEL_CONFIG_1_VAL  0x18 

#define MPU9250_ACCEL_CONFIG_2_REG  0x1D
// #define MPU9250_ACCEL_CONFIG_2_VAL  0x06  // 4KHz
#define MPU9250_ACCEL_CONFIG_2_VAL  0x0F // 1KHz


#define MPU9250_FIFO_ENABLE_REG            0X23
#define MPU9250_FIFO_ENABLE_VAL            0X08

#define MPU9250_FIFO_COUNT_H_REG           0X72
#define MPU9250_FIFO_COUNT_L_REG           0X73

#define MPU9250_FIFO_R_W_REG               0X74

#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */


void MPU9250_init()
{   
    i2c_master_init(I2C_MINDOT_MASTER_NUM, I2C_MINDOT_MASTER_SDA_IO, I2C_MINDOT_MASTER_SCL_IO);
    vTaskDelay(30 / portTICK_RATE_MS);

    i2c_write_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_PWR_MGMT_1_REG, MPU9250_PWR_MGMT_1_VAL);
    vTaskDelay(30 / portTICK_RATE_MS);

    i2c_write_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_PWR_MGMT_2_REG, MPU9250_PWR_MGMT_2_VAL);
    vTaskDelay(30 / portTICK_RATE_MS);

    i2c_write_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_SMPLRT_DIV_REG, MPU9250_SMPLRT_DIV_VAL);
    vTaskDelay(30 / portTICK_RATE_MS);
    
    i2c_write_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_CONFIG_REG, MPU9250_CONFIG_VAL);
    vTaskDelay(30 / portTICK_RATE_MS);
    
    i2c_write_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_ACCEL_CONFIG_1_REG, MPU9250_ACCEL_CONFIG_1_VAL);
    vTaskDelay(30 / portTICK_RATE_MS);
    i2c_write_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_ACCEL_CONFIG_2_REG, MPU9250_ACCEL_CONFIG_2_VAL);
    vTaskDelay(30 / portTICK_RATE_MS);
    i2c_write_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_FIFO_ENABLE_REG, MPU9250_FIFO_ENABLE_VAL);
    vTaskDelay(30 / portTICK_RATE_MS);
    i2c_write_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_USR_CTRL_REG, MPU9250_USR_CTRL_VAL);
    vTaskDelay(30 / portTICK_RATE_MS);
    // mpu9250_write_byte(MPU9250_SMPLRT_DIV_REG, MPU9250_SMPLRT_DIV_VAL);
    // mpu9250_write_byte(MPU9250_SMPLRT_DIV_REG, MPU9250_SMPLRT_DIV_VAL);

}


unsigned int MPU9250_get_FIFO_count()
{
    unsigned int count_H = 0;
    unsigned int count_L = 0;

    i2c_read_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_FIFO_COUNT_H_REG, &count_H);
    i2c_read_byte(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_FIFO_COUNT_L_REG, &count_L);

    // ESP_LOGI("mpu", "fifo count = %d, %d", count_H, count_L);
    
    return (count_H << 8) | count_L;
}

void MPU9250_read_FIFO(unsigned char * buf, unsigned int buf_len)
{
    i2c_read_bytes(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_FIFO_R_W_REG, buf, buf_len);
}

void MPU9250_read_accel(unsigned char * buf, unsigned int buf_len) 
{
    i2c_read_bytes(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_ACCEL_XOUT_H_REG, buf, buf_len);
}

void MPU9250_read_temp(unsigned char * buf, unsigned int buf_len)
{
    i2c_read_bytes(I2C_MINDOT_MASTER_NUM, MPU9250_I2C_ADDR, MPU9250_TEMP_OUT_H_REG, buf, buf_len);
}





















