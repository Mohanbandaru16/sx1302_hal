/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2020 Semtech

Description:
    Host specific functions to address the LoRa concentrator registers through
    a USB interface.
    Single-byte read/write and burst read/write.

License: Revised BSD License, see LICENSE.TXT file include in the project
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf */
#include <stdlib.h>     /* malloc free */
#include <unistd.h>     /* lseek, close */
#include <fcntl.h>      /* open */
#include <string.h>     /* memset */
#include <errno.h>      /* Error number definitions */
#include <termios.h>    /* POSIX terminal control definitions */

#include "loragw_usb.h"
#include "loragw_mcu.h"
#include "loragw_aux.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#if DEBUG_COM == 1
    #define DEBUG_MSG(str)                fprintf(stderr, str)
    //#define DEBUG_PRINTF(fmt, args...)    fprintf(stderr,"%s:%d: "fmt, __FUNCTION__, __LINE__, args)
    #define DEBUG_PRINTF(fmt, args...)    fprintf(stderr, fmt, args)
    #define CHECK_NULL(a)                if(a==NULL){fprintf(stderr,"%s:%d: ERROR: NULL POINTER AS ARGUMENT\n", __FUNCTION__, __LINE__);return LGW_USB_ERROR;}
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, args...)
    #define CHECK_NULL(a)                if(a==NULL){return LGW_USB_ERROR;}
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE TYPES -------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES  --------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

int set_interface_attribs_linux(int fd, int speed) {
    struct termios tty;

    memset(&tty, 0, sizeof tty);

    /* Get current attributes */
    if (tcgetattr(fd, &tty) != 0) {
        DEBUG_PRINTF("ERROR: tcgetattr failed with %d - %s", errno, strerror(errno));
        return LGW_USB_ERROR;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    /* Control Modes */
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* set 8-bit characters */
    tty.c_cflag |= CLOCAL;                      /* local connection, no modem control */
    tty.c_cflag |= CREAD;                       /* enable receiving characters */
    tty.c_cflag &= ~PARENB;                     /* no parity */
    tty.c_cflag &= ~CSTOPB;                     /* one stop bit */
    /* Input Modes */
    tty.c_iflag &= ~IGNBRK;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    /* Output Modes */
    tty.c_oflag &= ~IGNBRK;
    tty.c_oflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    /* Local Modes */
    tty.c_lflag = 0;
    /* Settings for non-canonical mode */
    tty.c_cc[VMIN] = 0;                         /* non-blocking mode */
    tty.c_cc[VTIME] = 50;                       /* wait for (n * 0.1) seconds before returning */

    /* Set attributes */
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        DEBUG_PRINTF("ERROR: tcsetattr failed with %d - %s", errno, strerror(errno));
        return LGW_USB_ERROR;
    }

    return LGW_USB_SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* configure serial interface to be read blocking or not*/
int set_blocking_linux(int fd, bool blocking) {
    struct termios tty;

    memset(&tty, 0, sizeof tty);

    /* Get current attributes */
    if (tcgetattr(fd, &tty) != 0) {
        DEBUG_PRINTF("ERROR: tcgetattr failed with %d - %s", errno, strerror(errno));
        return LGW_USB_ERROR;
    }

    tty.c_cc[VMIN] = (blocking == true) ? 1 : 0;    /* set blocking or non-blocking mode */
    tty.c_cc[VTIME] = 1;                            /* wait for (n * 0.1) seconds before returning */

    /* Set attributes */
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        DEBUG_PRINTF("ERROR: tcsetattr failed with %d - %s", errno, strerror(errno));
        return LGW_USB_ERROR;
    }

    return LGW_USB_SUCCESS;
}

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

int lgw_usb_open(const char * com_path, void **com_target_ptr) {
    int *usb_device = NULL;
    char portname[50];
    int x;
    int fd;
    s_ping_info gw_info;

    /*check input variables*/
    CHECK_NULL(com_target_ptr);

    usb_device = malloc(sizeof(int));
    if (usb_device == NULL) {
        DEBUG_MSG("ERROR : MALLOC FAIL\n");
        return LGW_USB_ERROR;
    }

    /* open tty port */
    sprintf(portname, "%s", com_path);
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("ERROR: failed to open COM port %s - %s\n", portname, strerror(errno));
    } else {
        x = set_interface_attribs_linux(fd, B115200);
        x |= set_blocking_linux(fd, true);
        if (x != 0) {
            printf("ERROR: failed to configure COM port %s\n", portname);
            free(usb_device);
            return LGW_USB_ERROR;
        }

        *usb_device = fd;
        *com_target_ptr = (void*)usb_device;

        /* Initialize pseudo-random generator for MCU request ID */
        srand(0);

        /* Check MCU version (ignore first char of the received version (release/debug) */
        if (mcu_ping(fd, &gw_info) != 0) {
            printf("ERROR: failed to ping the concentrator MCU\n");
            return LGW_USB_ERROR;
        }
        if (strncmp(gw_info.version + 1, mcu_version_string, sizeof mcu_version_string) != 0) {
            printf("ERROR: MCU version mismatch (expected:%s, got:%s)\n", mcu_version_string, gw_info.version);
            return -1;
        }
        printf("INFO: Concentrator MCU version is %s\n", gw_info.version);

        /* Reset SX1302 */
        x  = mcu_gpio_write(fd, 0, 1, 1); /*   set PA1 : POWER_EN*/
        x |= mcu_gpio_write(fd, 0, 2, 1); /*   set PA2 : SX1302_RESET active*/
        x |= mcu_gpio_write(fd, 0, 2, 0); /* unset PA2 : SX1302_RESET inactive*/
        if (x != 0) {
            printf("ERROR: failed to reset SX1302\n");
            free(usb_device);
            return LGW_USB_ERROR;
        }

        return LGW_USB_SUCCESS;
    }

    free(usb_device);
    return LGW_USB_ERROR;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* SPI release */
int lgw_usb_close(void *com_target) {
    int usb_device;
    int a;

    /* check input variables */
    CHECK_NULL(com_target);

    /* close file & deallocate file descriptor */
    usb_device = *(int *)com_target;
    a = close(usb_device);
    free(com_target);

    /* determine return code */
    if (a < 0) {
        DEBUG_MSG("ERROR: USB PORT FAILED TO CLOSE\n");
        return LGW_USB_ERROR;
    } else {
        DEBUG_MSG("Note: USB port closed\n");
        return LGW_USB_SUCCESS;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Simple write */
int lgw_usb_w(void *com_target, uint8_t spi_mux_target, uint16_t address, uint8_t data) {
    int usb_device;
    uint8_t command_size = 5;
    uint8_t out_buf[command_size];
    uint8_t in_buf[command_size];
    int a;

    /* check input variables */
    CHECK_NULL(com_target);

    usb_device = *(int *)com_target;

    /* prepare frame to be sent */
    out_buf[0] = 0;
    out_buf[1] = spi_mux_target;
    out_buf[2] = 0x80 | ((address >> 8) & 0x7F);
    out_buf[3] =        ((address >> 0) & 0xFF);
    out_buf[4] = data;
    a = mcu_spi_access(usb_device, out_buf, command_size, in_buf);

    /* determine return code */
    if (a != 0) {
        DEBUG_MSG("ERROR: USB WRITE FAILURE\n");
        return -1;
    } else {
        DEBUG_MSG("Note: USB write success\n");
        return 0;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Simple read */
int lgw_usb_r(void *com_target, uint8_t spi_mux_target, uint16_t address, uint8_t *data) {
    int usb_device;
    uint8_t command_size = 6;
    uint8_t out_buf[command_size];
    uint8_t in_buf[command_size];
    int a;

    /* check input variables */
    CHECK_NULL(com_target);
    CHECK_NULL(data);

    usb_device = *(int *)com_target;

    /* prepare frame to be sent */
    out_buf[0] = 0;
    out_buf[1] = spi_mux_target;
    out_buf[2] = 0x00 | ((address >> 8) & 0x7F);
    out_buf[3] =        ((address >> 0) & 0xFF);
    out_buf[4] = 0x00;
    out_buf[5] = 0x00;
    a = mcu_spi_access(usb_device, out_buf, command_size, in_buf);

    /* determine return code */
    if (a != 0) {
        DEBUG_MSG("ERROR: USB READ FAILURE\n");
        return -1;
    } else {
        DEBUG_MSG("Note: USB read success\n");
        *data = in_buf[command_size - 1]; /* the last byte contains the register value */
        return 0;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Burst (multiple-byte) write */
int lgw_usb_wb(void *com_target, uint8_t spi_mux_target, uint16_t address, const uint8_t *data, uint16_t size) {
    int usb_device;
    uint16_t command_size = size + 4;
    uint8_t in_buf[command_size];
    int i;
    int a;

    printf("%s\n", __FUNCTION__);

    /* check input parameters */
    CHECK_NULL(com_target);
    CHECK_NULL(data);

    usb_device = *(int *)com_target;

    /* prepare command byte */
    buf_req[0] = 0;
    buf_req[1] = spi_mux_target;
    buf_req[2] = 0x80 | ((address >> 8) & 0x7F);
    buf_req[3] =        ((address >> 0) & 0xFF);
    for (i = 0; i < size; i++) {
        buf_req[i + 4] = data[i];
    }
    a = mcu_spi_access(usb_device, buf_req, command_size, in_buf);

    /* determine return code */
    if (a != 0) {
        DEBUG_MSG("ERROR: USB WRITE BURST FAILURE\n");
        return -1;
    } else {
        DEBUG_MSG("Note: USB write burst success\n");
        return 0;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Burst (multiple-byte) read */
int lgw_usb_rb(void *com_target, uint8_t spi_mux_target, uint16_t address, uint8_t *data, uint16_t size) {
    int usb_device;
    uint16_t command_size = size + 5;
    uint8_t in_buf[command_size];
    int i;
    int a;

    printf("%s\n", __FUNCTION__);

    /* check input parameters */
    CHECK_NULL(com_target);
    CHECK_NULL(data);

    usb_device = *(int *)com_target;

    /* prepare command byte */
    buf_req[0] = 0;
    buf_req[1] = spi_mux_target;
    buf_req[2] = 0x00 | ((address >> 8) & 0x7F);
    buf_req[3] =        ((address >> 0) & 0xFF);
    buf_req[4] = 0x00;
    for (i = 0; i < size; i++) {
        buf_req[i + 5] = 0;
    }

    a = mcu_spi_access(usb_device, buf_req, command_size, in_buf);

    /* determine return code */
    if (a != 0) {
        DEBUG_MSG("ERROR: USB READ BURST FAILURE\n");
        return -1;
    } else {
        DEBUG_MSG("Note: USB read burst success\n");
        memcpy(data, in_buf + 5, size);
        return 0;
    }
}

/* --- EOF ------------------------------------------------------------------ */