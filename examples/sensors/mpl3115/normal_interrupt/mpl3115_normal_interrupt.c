/*
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file mpl3115_normal_interrupt.c
 * @brief The mpl3115_normal_interrupt.c file implements the ISSDK MPL3115 sensor driver
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
#include "mpl3115_drv.h"

//-----------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------
#define MPL3115_DATA_SIZE (5) /* 3 byte Pressure/Altitude and 2 byte Temperature. */
/*! In MPL3115 the Auto Acquisition Time Step (ODR) can be set only in powers of 2 (i.e. 2^x, where x is the
 *  SAMPLING_EXPONENT).
 *  This gives a range of 1 second to 2^15 seconds (9 hours). */
#define MPL3115_SAMPLING_EXPONENT (1) /* 2 seconds */

//-----------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------
/*! @brief Register settings for Interrupt (non buffered) mode. */
const registerwritelist_t cMpl3115ConfigNormal[] = {
    /* Enable Data Ready and Event flags for Pressure, Temperature or either. */
    {MPL3115_PT_DATA_CFG,
     MPL3115_PT_DATA_CFG_TDEFE_ENABLED | MPL3115_PT_DATA_CFG_PDEFE_ENABLED | MPL3115_PT_DATA_CFG_DREM_ENABLED,
     MPL3115_PT_DATA_CFG_TDEFE_MASK | MPL3115_PT_DATA_CFG_PDEFE_MASK | MPL3115_PT_DATA_CFG_DREM_MASK},
    /* Set Over Sampling Ratio to 128. */
    {MPL3115_CTRL_REG1, MPL3115_CTRL_REG1_OS_OSR_128, MPL3115_CTRL_REG1_OS_MASK},
    /* Set Auto acquisition time step. */
    {MPL3115_CTRL_REG2, MPL3115_SAMPLING_EXPONENT, MPL3115_CTRL_REG2_ST_MASK},
    /* Set INT1 Active High. */
    {MPL3115_CTRL_REG3, MPL3115_CTRL_REG3_IPOL1_HIGH, MPL3115_CTRL_REG3_IPOL1_MASK},
    /* Enable Interrupts for Data Ready Events. */
    {MPL3115_CTRL_REG4, MPL3115_CTRL_REG4_INT_EN_DRDY_INTENABLED, MPL3115_CTRL_REG4_INT_EN_DRDY_MASK},
    /* Route Interrupt to INT1. */
    {MPL3115_CTRL_REG5, MPL3115_CTRL_REG5_INT_CFG_DRDY_INT1, MPL3115_CTRL_REG5_INT_CFG_DRDY_MASK},
    __END_WRITE_DATA__};

/*! @brief Address and size of Raw Pressure+Temperature Data in Normal Mode. */
const registerreadlist_t cMpl3115OutputNormal[] = {{.readFrom = MPL3115_OUT_P_MSB, .numBytes = MPL3115_DATA_SIZE},
                                                   __END_READ_DATA__};

//-----------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------
volatile uint8_t gMpl3115DataReady;

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
void mpl3115_int_data_ready_callback(ARM_GPIO_Pin_t pin, uint32_t even)
{ /*! @brief Set flag to indicate Sensor has signalled data ready. */
    gMpl3115DataReady = true;
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
    int16_t tempInDegrees;
    uint32_t pressureInPascals;
    int32_t status;
    uint8_t data[MPL3115_DATA_SIZE];
    mpl3115_pressuredata_t rawData;

    ARM_DRIVER_I2C *I2Cdrv = &MPL3115_I2C_DRIVER;
    mpl3115_i2c_sensorhandle_t mpl3115Driver;
    ARM_DRIVER_GPIO *pGpioDriver = &Driver_GPIO0;
    uint32_t vioOut = 0U;

    PRINTF("\r\n ISSDK MPL3115 sensor driver example demonstration with interrupt mode.\r\n");

    /*! Setup MAG3110 pin used by board */
    pGpioDriver->Setup(MPL3115_INT1, &mpl3115_int_data_ready_callback);
    pGpioDriver->SetDirection(MPL3115_INT1, ARM_GPIO_INPUT);
    pGpioDriver->SetEventTrigger(MPL3115_INT1, ARM_GPIO_TRIGGER_RISING_EDGE);

    /*! Initialize the I2C driver. */
    status = I2Cdrv->Initialize(I2C_SignalEvent(MPL3115_I2C_INDEX));
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

    /*! Initialize MPL3115 sensor driver. */
    status = MPL3115_I2C_Initialize(&mpl3115Driver, &MPL3115_I2C_DRIVER, MPL3115_I2C_INDEX, MPL3115_I2C_ADDR,
                                    MPL3115_WHOAMI_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n Sensor Initialization Failed\r\n");
        return -1;
    }
    PRINTF("\r\n Successfully Initiliazed Sensor\r\n");

    /*!  Set the task to be executed while waiting for I2C transactions to complete. */
    MPL3115_I2C_SetIdleTask(&mpl3115Driver, (registeridlefunction_t)COMM_IDLE_FUNC, COMM_IDLE_ARG);

    gMpl3115DataReady = false;

    /*! Configure the MAG3110 sensor driver. */
    status = MPL3115_I2C_Configure(&mpl3115Driver, cMpl3115ConfigNormal);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\nMPL3115 configuration failed...\r\n");
        return -1;
    }
    PRINTF("\r\n Successfully Applied MPL3115 Sensor Configuration\r\n");

    for (;;) /* Forever loop */
    {        /* In ISR Mode we do not need to check Data Ready Register.
              * The receipt of interrupt will indicate data is ready. */
        if (false == gMpl3115DataReady)
        { /* Loop, if new sample is not available. */
            ENTER_SLEEP();
            continue;
        }
        else
        { /*! Clear the data ready flag, it will be set again by the ISR. */
            gMpl3115DataReady = false;
            vioOut ^= vioLED1;
            vioSetSignal(vioLED1, vioOut);
        }

        /*! Read new raw sensor data from the MPL3115. */
        status = MPL3115_I2C_ReadData(&mpl3115Driver, cMpl3115OutputNormal, data);
        if (ARM_DRIVER_OK != status)
        {
            PRINTF("\r\n Read Failed. \r\n");
            return -1;
        }

        /*! Process the sample and convert the raw sensor data to signed 16-bit container. */
        rawData.pressure = (uint32_t)((data[0]) << 16) | ((data[1]) << 8) | ((data[2]));
        pressureInPascals = rawData.pressure / MPL3115_PRESSURE_CONV_FACTOR;

        rawData.temperature = (int16_t)((data[3]) << 8) | (data[4]);
        tempInDegrees = rawData.temperature / MPL3115_TEMPERATURE_CONV_FACTOR;

        /* NOTE: PRINTF is relatively expensive in terms of CPU time, specially when used with-in execution loop. */
        PRINTF("\r\nPressure    = %d Pa\r\n", pressureInPascals);
        PRINTF("\r\nTemperature = %d degC\r\n", tempInDegrees);
        ASK_USER_TO_RESUME(8); /* Ask for user input after processing 8 samples. */
    }
}
