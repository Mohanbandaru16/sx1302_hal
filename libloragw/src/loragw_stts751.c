/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Basic driver for ST ts751 temperature sensor

License: Revised BSD License, see LICENSE.TXT file include in the project
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf */

#include "loragw_i2c.h"
#include "loragw_stts751.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#if DEBUG_I2C == 1
    #define DEBUG_MSG(str)              fprintf(stdout, str)
    #define DEBUG_PRINTF(fmt, args...)  fprintf(stdout,"%s:%d: "fmt, __FUNCTION__, __LINE__, args)
    #define CHECK_NULL(a)               if(a==NULL){fprintf(stderr,"%s:%d: ERROR: NULL POINTER AS ARGUMENT\n", __FUNCTION__, __LINE__);return LGW_REG_ERROR;}
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, args...)
    #define CHECK_NULL(a)               if(a==NULL){return LGW_REG_ERROR;}
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define STTS751_REG_TEMP_H      0x00
#define STTS751_REG_STATUS      0x01
#define STTS751_STATUS_TRIPT    BIT(0)
#define STTS751_STATUS_TRIPL    BIT(5)
#define STTS751_STATUS_TRIPH    BIT(6)
#define STTS751_REG_TEMP_L      0x02
#define STTS751_REG_CONF        0x03
#define STTS751_CONF_RES_MASK   0x0C
#define STTS751_CONF_RES_SHIFT  2
#define STTS751_CONF_EVENT_DIS  BIT(7)
#define STTS751_CONF_STOP       BIT(6)
#define STTS751_REG_RATE        0x04
#define STTS751_REG_HLIM_H      0x05
#define STTS751_REG_HLIM_L      0x06
#define STTS751_REG_LLIM_H      0x07
#define STTS751_REG_LLIM_L      0x08
#define STTS751_REG_TLIM        0x20
#define STTS751_REG_HYST        0x21
#define STTS751_REG_SMBUS_TO    0x22

#define STTS751_REG_PROD_ID     0xFD
#define STTS751_REG_MAN_ID      0xFE
#define STTS751_REG_REV_ID      0xFF

#define STTS751_0_PROD_ID       0x00
#define STTS751_1_PROD_ID       0x01
#define ST_MAN_ID               0x53

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

int stts751_configure(int i2c_fd, uint8_t i2c_addr) {
    (void) i2c_fd;
    (void)i2c_addr;

    return LGW_I2C_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int stts751_get_temperature(int i2c_fd, uint8_t i2c_addr, float * temperature) {
    (void) i2c_fd;
    (void)i2c_addr;

    *temperature = 30;

    return LGW_I2C_SUCCESS;
}

/* --- EOF ------------------------------------------------------------------ */
