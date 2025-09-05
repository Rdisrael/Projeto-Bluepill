#include "gy521.h"

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t check;
    HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, 0x75, 1, &check, 1, HAL_MAX_DELAY);
    if (check != 0x68) return HAL_ERROR;
    uint8_t data = 0;
    HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, 0x6B, 1, &data, 1, HAL_MAX_DELAY);
    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_Read_Accel(I2C_HandleTypeDef *hi2c, AxisRaw *accel) {
    uint8_t buf[6];
    HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, 0x3B, 1, buf, 6, HAL_MAX_DELAY);
    accel->X = (buf[0] << 8) | buf[1];
    accel->Y = (buf[2] << 8) | buf[3];
    accel->Z = (buf[4] << 8) | buf[5];
    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_Read_Gyro(I2C_HandleTypeDef *hi2c, AxisRaw *gyro) {
    uint8_t buf[6];
    HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, 0x43, 1, buf, 6, HAL_MAX_DELAY);
    gyro->X = (buf[0] << 8) | buf[1];
    gyro->Y = (buf[2] << 8) | buf[3];
    gyro->Z = (buf[4] << 8) | buf[5];
    return HAL_OK;
}
