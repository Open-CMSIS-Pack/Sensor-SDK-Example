/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fxos8700_interrupt.c
 * @brief The fxos8700_interrupt.c file implements the ISSDK FXOS8700 sensor
 *        driver example demonstration with interrupt mode.
 */

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
#include "fxos8700_drv_i2c.h"

/*******************************************************************************
 * Macro Definitions
 ******************************************************************************/
#define RAW_ACCEL_DATA_SIZE (6)

//-----------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------
/*! Prepare the register write list to configure FXAS21002 in non-FIFO mode. */
const registerwritelist_t fxos8700_Config_Interrupt[] = {
    /*! System and Control registers. */
    /*! Configure the FXOS8700 to 12.5Hz sampling rate. */
    {FXOS8700_CTRL_REG1, FXOS8700_CTRL_REG1_DR_SINGLE_12P5_HZ, FXOS8700_CTRL_REG1_DR_MASK},
    {FXOS8700_CTRL_REG3, FXOS8700_CTRL_REG3_IPOL_ACTIVE_HIGH | FXOS8700_CTRL_REG3_PP_OD_PUSH_PULL,
                         FXOS8700_CTRL_REG3_IPOL_MASK | FXOS8700_CTRL_REG3_PP_OD_MASK}, /*! Active High, Push-Pull */
    {FXOS8700_CTRL_REG4, FXOS8700_CTRL_REG4_INT_EN_DRDY_EN,
                         FXOS8700_CTRL_REG4_INT_EN_DRDY_MASK}, /*! Data Ready Event. */
    {FXOS8700_CTRL_REG5, FXOS8700_CTRL_REG5_INT_CFG_DRDY_INT2, FXOS8700_CTRL_REG5_INT_CFG_DRDY_MASK}, /*! INT2 Pin  */
    /*! Configure the fxos8700 to Accel Only */
    {FXOS8700_M_CTRL_REG2, FXOS8700_M_CTRL_REG2_M_AUTOINC_ACCEL_ONLY_MODE, FXOS8700_M_CTRL_REG2_M_AUTOINC_MASK},
    __END_WRITE_DATA__};

/*! Command definition to read the Data Ready Status */
const registerreadlist_t FXOS8700_STATUS_READ[] = {{.readFrom = FXOS8700_STATUS, .numBytes = 1}, __END_READ_DATA__};

/*! Command definition to read the Accel Data */
const registerreadlist_t FXOS8700_ACCEL_READ[] = {{.readFrom = FXOS8700_OUT_X_MSB, .numBytes = RAW_ACCEL_DATA_SIZE},
                                                  __END_READ_DATA__};

//-----------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------
volatile bool fxos8700Interrupt = false;

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
void fxos8700_isr_callback(ARM_GPIO_Pin_t pin, uint32_t event)
{ /*! @brief Set flag to indicate Sensor has signalled data ready. */
    fxos8700Interrupt = true;
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
    uint8_t data[RAW_ACCEL_DATA_SIZE];
    fxos8700_accelmagdata_t rawData;

    ARM_DRIVER_I2C *I2Cdrv = &FXOS8700_I2C_DRIVER;
    fxos8700_i2c_sensorhandle_t FXOS8700drv;
    ARM_DRIVER_GPIO *pGpioDriver = &Driver_GPIO0;
    uint32_t vioOut = 0U;

    PRINTF("\r\n ISSDK FXOS8700 sensor driver example demonstration with interrupt mode.\r\n");

    /*! Setup INT2_FXOS8700 pin */
    pGpioDriver->Setup(FXOS8700_INT2, &fxos8700_isr_callback);
    pGpioDriver->SetDirection(FXOS8700_INT2, ARM_GPIO_INPUT);
    pGpioDriver->SetEventTrigger(FXOS8700_INT2, ARM_GPIO_TRIGGER_RISING_EDGE);

    /*! Initialize the I2C driver. */
    status = I2Cdrv->Initialize(I2C_SignalEvent(FXOS8700_I2C_INDEX));
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

    /*! Initialize the FXOS8700 sensor driver. */
    status = FXOS8700_I2C_Initialize(&FXOS8700drv, &FXOS8700_I2C_DRIVER, FXOS8700_I2C_INDEX, FXOS8700_I2C_ADDR,
                                     FXOS8700_WHO_AM_I_PROD_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n Sensor Initialization Failed\r\n");
        return -1;
    }
    PRINTF("\r\n Successfully Initiliazed Sensor\r\n");

    /*!  Set the task to be executed while waiting for I2C transactions to complete. */
    FXOS8700_I2C_SetIdleTask(&FXOS8700drv, (registeridlefunction_t)COMM_IDLE_FUNC, COMM_IDLE_ARG);

    /*! Configure the fxos8700 sensor driver. */
    status = FXOS8700_I2C_Configure(&FXOS8700drv, fxos8700_Config_Interrupt);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n FXOS8700 Sensor Configuration Failed, Err = %d\r\n", status);
        return -1;
    }
    PRINTF("\r\n Successfully Applied FXOS8700 Sensor Configuration\r\n");

    for (;;) /* Forever loop */
    {        /* In ISR Mode we do not need to check Data Ready Register.
              * The receipt of interrupt will indicate data is ready. */
        if (false == fxos8700Interrupt)
        { /* Loop, if new sample is not available. */
            ENTER_SLEEP();
            continue;
        }
        else
        { /*! Clear the data ready flag, it will be set again by the ISR. */
            fxos8700Interrupt = false;
            vioOut ^= vioLED1;
            vioSetSignal(vioLED1, vioOut);
        }

        /*! Read the raw sensor data from the fxos8700. */
        status = FXOS8700_I2C_ReadData(&FXOS8700drv, FXOS8700_ACCEL_READ, data);
        if (ARM_DRIVER_OK != status)
        {
            PRINTF("\r\n Read Failed. \r\n");
            return -1;
        }

        /*! Convert the raw sensor data to signed 16-bit container for display to the debug port. */
        rawData.accel[0] = ((int16_t)data[0] << 8) | data[1];
        rawData.accel[0] /= 4;
        rawData.accel[1] = ((int16_t)data[2] << 8) | data[3];
        rawData.accel[1] /= 4;
        rawData.accel[2] = ((int16_t)data[4] << 8) | data[5];
        rawData.accel[2] /= 4;

        /* NOTE: PRINTF is relatively expensive in terms of CPU time, specially when used with-in execution loop. */
        PRINTF("\r\n Accel X = %d  Y = %d  Z = %d\r\n", rawData.accel[0], rawData.accel[1], rawData.accel[2]);
        ASK_USER_TO_RESUME(50); /* Ask for user input after processing 100 samples. */
    }
}
