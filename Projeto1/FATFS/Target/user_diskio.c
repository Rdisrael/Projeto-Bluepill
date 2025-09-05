/**
  ******************************************************************************
  * @file    user_diskio.c
  * @brief   SPI SD Card Disk I/O implementation for FatFs
  ******************************************************************************
  */

#include "user_diskio.h"
#include "main.h"      // sempre inclui as definições globais
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_spi.h"
/* Private typedef -----------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

/* Extern SPI handler */
extern SPI_HandleTypeDef hspi1;

/* CS pin definition */
#define SD_CS_GPIO_Port GPIOA
#define SD_CS_Pin       GPIO_PIN_4

/* Private function prototypes -----------------------------------------------*/
static void     SD_Select(void);
static void     SD_Unselect(void);
static uint8_t  SD_SPI_TxRx(uint8_t data);
static void     SD_SendDummyClocks(uint8_t n);
static uint8_t  SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc);
/* Prototypes required by FatFs driver */
DSTATUS USER_initialize(BYTE pdrv);
DSTATUS USER_status(BYTE pdrv);
DRESULT USER_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
DRESULT USER_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif
#if _USE_IOCTL == 1
DRESULT USER_ioctl(BYTE pdrv, BYTE cmd, void *buff);
#endif

/* Diskio driver structure */
Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE == 1
  USER_write,
#endif
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif
};

/* Helper functions ----------------------------------------------------------*/
static void SD_Select(void) {
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

static void SD_Unselect(void) {
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

static uint8_t SD_SPI_TxRx(uint8_t data) {
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

static void SD_SendDummyClocks(uint8_t n) {
    while(n--) SD_SPI_TxRx(0xFF);
}

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    uint8_t buf[6];
    uint8_t res;
    uint8_t n;

    buf[0] = 0x40 | cmd;
    buf[1] = (arg >> 24) & 0xFF;
    buf[2] = (arg >> 16) & 0xFF;
    buf[3] = (arg >> 8) & 0xFF;
    buf[4] = arg & 0xFF;
    buf[5] = crc;

    SD_Select();
    for (n = 0; n < 6; n++) SD_SPI_TxRx(buf[n]);

    n = 10; // até 10 tentativas para resposta
    do {
        res = SD_SPI_TxRx(0xFF);
    } while ((res & 0x80) && --n);

    return res;
}

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/* Functions required by FatFs ----------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number
  * @retval DSTATUS
  */
DSTATUS USER_initialize(BYTE pdrv) {
    uint8_t resp;
    uint16_t retry = 0xFFF;

    Stat = STA_NOINIT;
    SD_Unselect();

    // manda pelo menos 74 clocks com CS alto
    SD_SendDummyClocks(10);

    // CMD0 -> Reset
    resp = SD_SendCmd(0, 0, 0x95);
    if (resp != 0x01) {
        SD_Unselect();
        return Stat;
    }

    // CMD1 -> inicialização
    do {
        resp = SD_SendCmd(1, 0, 0xFF);
    } while (resp != 0x00 && --retry);

    SD_Unselect();

    if (retry == 0) {
        return Stat;
    }

    Stat &= ~STA_NOINIT; // sucesso
    return Stat;
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number
  * @retval DSTATUS
  */
DSTATUS USER_status(BYTE pdrv) {
    return Stat;
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: drive number
  * @param  buff: buffer para armazenar dados
  * @param  sector: setor (LBA)
  * @param  count: número de setores
  * @retval DRESULT
  */
DRESULT USER_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
    uint8_t token;
    uint16_t i;

    if (count != 1) return RES_PARERR;

    if (SD_SendCmd(17, sector * 512, 0xFF) != 0x00) {
        SD_Unselect();
        return RES_ERROR;
    }

    // espera token 0xFE
    do {
        token = SD_SPI_TxRx(0xFF);
    } while (token == 0xFF);

    if (token != 0xFE) {
        SD_Unselect();
        return RES_ERROR;
    }

    // lê 512 bytes
    for (i = 0; i < 512; i++) {
        buff[i] = SD_SPI_TxRx(0xFF);
    }

    // descarta CRC
    SD_SPI_TxRx(0xFF);
    SD_SPI_TxRx(0xFF);

    SD_Unselect();
    return RES_OK;
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: drive number
  * @param  buff: buffer com dados
  * @param  sector: setor (LBA)
  * @param  count: número de setores
  * @retval DRESULT
  */
#if _USE_WRITE == 1
DRESULT USER_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
    uint8_t resp;
    uint16_t i;

    if (count != 1) return RES_PARERR;

    if (SD_SendCmd(24, sector * 512, 0xFF) != 0x00) {
        SD_Unselect();
        return RES_ERROR;
    }

    SD_SPI_TxRx(0xFE); // token start

    for (i = 0; i < 512; i++) {
        SD_SPI_TxRx(buff[i]);
    }

    // CRC dummy
    SD_SPI_TxRx(0xFF);
    SD_SPI_TxRx(0xFF);

    resp = SD_SPI_TxRx(0xFF);
    if ((resp & 0x1F) != 0x05) {
        SD_Unselect();
        return RES_ERROR;
    }

    // espera busy
    while (SD_SPI_TxRx(0xFF) == 0);

    SD_Unselect();
    return RES_OK;
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control
  * @param  pdrv: drive number
  * @param  cmd: comando
  * @param  buff: buffer
  * @retval DRESULT
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    switch (cmd) {
        case CTRL_SYNC:
            SD_Select();
            SD_Unselect();
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(DWORD*)buff = 32768; // valor fictício (16MB)
            return RES_OK;
    }
    return RES_PARERR;
}
#endif /* _USE_IOCTL == 1 */
