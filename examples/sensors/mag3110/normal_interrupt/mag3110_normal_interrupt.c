/*
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file mag3110_normal_interrupt.c
 * @brief The mag3110_normal_interrupt.c file implements the ISSDK MAG3110 sensor driver
 *        example demonstration with interrupt mode.
 */

//-----------------------------------------------------------------------
// C Library Includes
//-----------------------------------------------------------------------
#include <stdio.h>

//-----------------------------------------------------------------------
// CMSIS Includes
//-----------------------------------------------------------------------
#include "cmsis_vio.h"
#include "Driver_I2C.h"
#include "Driver_GPIO.h"

//-----------------------------------------------------------------------
// ISSDK Includes
//-----------------------------------------------------------------------
#include "issdk_hal.h"
#include "mag3110_drv.h"

//-----------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------
#define MAG3110_DATA_SIZE (6) /* 2 byte X,Y,Z Axis Data each. */

//-----------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------
/*! @brief Register settings for Normal (non buffered) mode since Interrupt is enabled implicitly. */
const registerwritelist_t cMag3110ConfigNormal[] = {
    /* Set Ouput Rate @10HZ (ODR = 2 and OSR = 32). */
    {MAG3110_CTRL_REG1, MAG3110_CTRL_REG1_DR_ODR_2 | MAG3110_CTRL_REG1_OS_OSR_32,
                        MAG3110_CTRL_REG1_DR_MASK | MAG3110_CTRL_REG1_OS_MASK},
    /* Set Auto Magnetic Sensor Reset. */
    {MAG3110_CTRL_REG2, MAG3110_CTRL_REG2_MAG_RST_EN | MAG3110_CTRL_REG2_AUTO_MSRT_EN_EN | MAG3110_CTRL_REG2_RAW_RAW,
                        MAG3110_CTRL_REG2_MAG_RST_MASK | MAG3110_CTRL_REG2_AUTO_MSRT_EN_MASK | MAG3110_CTRL_REG2_RAW_MASK},
    __END_WRITE_DATA__};

/*! @brief Address and size of Raw Pressure+Temperature Data in Normal Mode. */
const registerreadlist_t cMag3110OutputNormal[] = {{.readFrom = MAG3110_OUT_X_MSB, .numBytes = MAG3110_DATA_SIZE},
                                                   __END_READ_DATA__};

//-----------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------
volatile bool gMag3110DataReady;

//-----------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------
/*! -----------------------------------------------------------------------
 *  @brief       This is the Sensor Data Ready ISR implementation.
 *  @details     This function sets the flag which indicates if a new sample(s) is available for reading.
 *  @param[in]   pin This is the GPIO pin on which event occurred.
 *  @param[in]   event This is the GPIO event which occurred.
 *  @return      void  There is no return value.
 *  @constraints None
 *  @reeentrant  Yes
 *  -----------------------------------------------------------------------*/
void mag3110_int_data_ready_callback(ARM_GPIO_Pin_t pin, uint32_t even)
{ /*! @brief Set flag to indicate Sensor has signalled data ready. */
    gMag3110DataReady = true;
}

/*! -----------------------------------------------------------------------
 *  @brief       This is the application main function implementation.
 *  @details     This function brings up the sensor and enters an endless loop
 *               to continuously read available samples.
 *  @param[in]   void This is no input parameter.
 *  @return      void  There is no return value.
 *  @constraints None
 *  @reeentrant  No
 *  -----------------------------------------------------------------------*/
int app_main(void)
{
    int32_t status;
    uint8_t data[MAG3110_DATA_SIZE];
    mag3110_magdata_t rawData;

    ARM_DRIVER_I2C *I2Cdrv = &MAG3110_I2C_DRIVER;
    mag3110_i2c_sensorhandle_t mag3110Driver;
    ARM_DRIVER_GPIO *pGpioDriver = &Driver_GPIO0;
    uint32_t vioOut = 0U;

    PRINTF("\r\n ISSDK MAG3110 sensor driver example demonstration with interrupt mode.\r\n");

    /*! Setup MAG3110 pin used by board */
    pGpioDriver->Setup(MAG3110_INT1, &mag3110_int_data_ready_callback);
    pGpioDriver->SetDirection(MAG3110_INT1, ARM_GPIO_INPUT);
    pGpioDriver->SetEventTrigger(MAG3110_INT1, ARM_GPIO_TRIGGER_RISING_EDGE);

    /*! Initialize the I2C driver. */
    status = I2Cdrv->Initialize(I2C_SignalEvent(MAG3110_I2C_INDEX));
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Initialization Failed\r\n");
        return -1;
    }

    /*! Set the I2C Power mode. */
    status = I2Cdrv->PowerControl(ARM_POWER_FULL);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Power Mode setting Failed\r\n");
        return -1;
    }

    /*! Set the I2C bus speed. */
    status = I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Control Mode setting Failed\r\n");
        return -1;
    }

    /*! Initialize MAG3110 sensor driver. */
    status = MAG3110_I2C_Initialize(&mag3110Driver, &MAG3110_I2C_DRIVER, MAG3110_I2C_INDEX, MAG3110_I2C_ADDR,
                                    MAG3110_WHOAMI_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n Sensor Initialization Failed\r\n");
        return -1;
    }
    PRINTF("\r\n Successfully Initiliazed Sensor\r\n");

    /*!  Set the task to be executed while waiting for I2C transactions to complete. */
    MAG3110_I2C_SetIdleTask(&mag3110Driver, (registeridlefunction_t)COMM_IDLE_FUNC, COMM_IDLE_ARG);

    gMag3110DataReady = true; /* Since, INT for MAG3110 is by default High after Power ON, we have to directly read the
                                 first sample for MAG3110 to clear INT. */

    /*! Configure the MAG3110 sensor driver. */
    status = MAG3110_I2C_Configure(&mag3110Driver, cMag3110ConfigNormal);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n nMAG3110 Sensor Configuration Failed, Err = %d\r\n", status);
        return -1;
    }
    PRINTF("\r\n Successfully Applied MAG3110 Sensor Configuration\r\n");

    for (;;) /* Forever loop */
    {        /* In ISR Mode we do not need to check Data Ready Register.
              * The receipt of interrupt will indicate data is ready. */
        if (false == gMag3110DataReady)
        { /* Loop, if new sample is not available. */
            ENTER_SLEEP();
            continue;
        }
        else
        { /*! Clear the data ready flag, it will be set again by the ISR. */
            gMag3110DataReady = false;
            vioOut ^= vioLED1;
            vioSetSignal(vioLED1, vioOut);
        }

        /*! Read the raw sensor data from the MAG3110. */
        status = MAG3110_I2C_ReadData(&mag3110Driver, cMag3110OutputNormal, data);
        if (ARM_DRIVER_OK != status)
        {
            PRINTF("\r\n Read Failed. \r\n");
            return -1;
        }

        /*! Process the sample and convert the raw sensor data to signed 16-bit container. */
        rawData.mag[0] = ((int16_t)data[0] << 8) | data[1];
        rawData.mag[1] = ((int16_t)data[2] << 8) | data[3];
        rawData.mag[2] = ((int16_t)data[4] << 8) | data[5];

		MAG3110_CalibrateHardIronOffset(&rawData.mag[0], &rawData.mag[1], &rawData.mag[2]);
		
        /* NOTE: PRINTF is relatively expensive in terms of CPU time, specially when used with-in execution loop. */
        PRINTF("\r\n Mag  X = %d  Y = %d  Z = %d\r\n", rawData.mag[0], rawData.mag[1], rawData.mag[2]);
        ASK_USER_TO_RESUME(100); /* Ask for user input after processing 100 samples. */
    }
}
