#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f1xx_hal.h"

#define MPU6050_ADDR (0x68 << 1)

typedef struct {
    int16_t X;
    int16_t Y;
    int16_t Z;
} AxisRaw;

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MPU6050_Read_Accel(I2C_HandleTypeDef *hi2c, AxisRaw *accel);
HAL_StatusTypeDef MPU6050_Read_Gyro(I2C_HandleTypeDef *hi2c, AxisRaw *gyro);

#endif
