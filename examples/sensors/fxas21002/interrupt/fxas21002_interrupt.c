/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fxas21002_interrupt.c
 * @brief The fxas21002_interrupt.c file implements the ISSDK FXAS21002 sensor
 *        driver example demonstration with interrupt mode.
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
#include "fxas21002_drv_i2c.h"

//-----------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------
/*! Prepare the register write list to configure FXAS21002 in non-FIFO mode. */
const registerwritelist_t fxas21002_Config_Isr[] = {
    /*! Configure CTRL_REG1 register to put FXAS21002 to 12.5Hz sampling rate. */
    {FXAS21002_CTRL_REG1, FXAS21002_CTRL_REG1_DR_12_5HZ, FXAS21002_CTRL_REG1_DR_MASK},
    /*! Configure CTRL_REG2 register to set interrupt configuration settings. */
    {FXAS21002_CTRL_REG2, FXAS21002_CTRL_REG2_IPOL_ACTIVE_HIGH | FXAS21002_CTRL_REG2_INT_EN_DRDY_ENABLE |
                              FXAS21002_CTRL_REG2_INT_CFG_DRDY_INT1,
     FXAS21002_CTRL_REG2_IPOL_MASK | FXAS21002_CTRL_REG2_INT_EN_DRDY_MASK | FXAS21002_CTRL_REG2_INT_CFG_DRDY_MASK},
    __END_WRITE_DATA__};

/*! Prepare the register read list to read FXAS21002 DataReady status. */
const registerreadlist_t fxas21002_DRDY[] = {{.readFrom = FXAS21002_STATUS, .numBytes = 1}, __END_READ_DATA__};

/*! Prepare the register read list to read the raw gyro data from the FXAS21002. */
const registerreadlist_t fxas21002_Output_Values[] = {
    {.readFrom = FXAS21002_OUT_X_MSB, .numBytes = FXAS21002_GYRO_DATA_SIZE}, __END_READ_DATA__};
//-----------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------
volatile bool fxas21002Interrupt = false;

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
void fxas21002_isr(ARM_GPIO_Pin_t pin, uint32_t even)
{ /*! @brief Set flag to indicate Sensor has signalled data ready. */
    fxas21002Interrupt = true;
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
    uint8_t data[FXAS21002_GYRO_DATA_SIZE];
    fxas21002_gyrodata_t rawData;

    ARM_DRIVER_I2C *I2Cdrv = &FXAS21002_I2C_DRIVER;
    fxas21002_i2c_sensorhandle_t FXAS21002drv;
    ARM_DRIVER_GPIO *pGpioDriver = &Driver_GPIO0;
    uint32_t vioOut = 0U;

    PRINTF("\r\n ISSDK FXAS21002 sensor driver example demonstration with interrupt mode.\r\n");

    /*! Setup INT1_FXAS21002 pin used by board */
    pGpioDriver->Setup(FXAS21002_INT1, &fxas21002_isr);
    pGpioDriver->SetDirection(FXAS21002_INT1, ARM_GPIO_INPUT);
    pGpioDriver->SetEventTrigger(FXAS21002_INT1, ARM_GPIO_TRIGGER_RISING_EDGE);

    /*! Initialize the I2C driver. */
    status = I2Cdrv->Initialize(I2C_SignalEvent(FXAS21002_I2C_INDEX));
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

    /*! Initialize the FXAS21002 sensor driver. */
    status = FXAS21002_I2C_Initialize(&FXAS21002drv, &FXAS21002_I2C_DRIVER, FXAS21002_I2C_INDEX, FXAS21002_I2C_ADDR,
                                      FXAS21002_WHO_AM_I_WHOAMI_PROD_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n Sensor Initialization Failed\r\n");
        return -1;
    }
    PRINTF("\r\n Successfully Initiliazed Sensor\r\n");

    /*!  Set the task to be executed while waiting for I2C transactions to complete. */
    FXAS21002_I2C_SetIdleTask(&FXAS21002drv, (registeridlefunction_t)COMM_IDLE_FUNC, COMM_IDLE_ARG);

    /*! Configure the FXAS21002 sensor driver. */
    status = FXAS21002_I2C_Configure(&FXAS21002drv, fxas21002_Config_Isr);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n FXAS21002 Sensor Configuration Failed, Err = %d\r\n", status);
        return -1;
    }
    PRINTF("\r\n Successfully Applied FXAS21002 Sensor Configuration\r\n");

    for (;;) /* Forever loop */
    {        /* In ISR Mode we do not need to check Data Ready Register.
              * The receipt of interrupt will indicate data is ready. */
        if (false == fxas21002Interrupt)
        { /* Loop, if new sample is not available. */
            ENTER_SLEEP();
            continue;
        }
        else
        { /*! Clear the data ready flag, it will be set again by the ISR. */
            fxas21002Interrupt = false;
            vioOut ^= vioLED1;
            vioSetSignal(vioLED1, vioOut);
        }

        /*! Read the raw sensor data from the FXAS21002. */
        status = FXAS21002_I2C_ReadData(&FXAS21002drv, fxas21002_Output_Values, data);
        if (ARM_DRIVER_OK != status)
        {
            PRINTF("\r\n Read Failed. \r\n");
            return -1;
        }

        /*! Convert the raw sensor data to signed 16-bit container for display to the debug port. */
        rawData.gyro[0] = ((int16_t)data[0] << 8) | data[1];
        rawData.gyro[1] = ((int16_t)data[2] << 8) | data[3];
        rawData.gyro[2] = ((int16_t)data[4] << 8) | data[5];

        /* NOTE: PRINTF is relatively expensive in terms of CPU time, specially when used with-in execution loop. */
        PRINTF("\r\n Gyro X = %d  Y = %d  Z = %d\r\n", rawData.gyro[0], rawData.gyro[1], rawData.gyro[2]);
        ASK_USER_TO_RESUME(100); /* Ask for user input after processing 100 samples. */
    }
}
