/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fxas21002_drv_i2c.c
 * @brief The fxas21002_drv_i2c.c file implements the FXAS21002 sensor driver interfaces.
 */

//-----------------------------------------------------------------------
// ISSDK Includes
//-----------------------------------------------------------------------
#include "gpio_driver.h"
#include "fxas21002_drv_i2c.h"

//-----------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------
int32_t FXAS21002_I2C_Initialize(
    fxas21002_i2c_sensorhandle_t *pSensorHandle, ARM_DRIVER_I2C *pBus, uint8_t index, uint16_t sAddress, uint8_t whoAmi)
{
    int32_t status;
    uint8_t reg;

    /*! Check the input parameters. */
    if ((pSensorHandle == NULL) || (pBus == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    pSensorHandle->deviceInfo.deviceInstance = index;
    pSensorHandle->deviceInfo.functionParam = NULL;
    pSensorHandle->deviceInfo.idleFunction = NULL;

    /*!  Read and store the device's WHO_AM_I.*/
    status = Register_I2C_Read(pBus, &pSensorHandle->deviceInfo, sAddress, FXAS21002_WHO_AM_I, 1, &reg);
    if ((ARM_DRIVER_OK != status) || (whoAmi != reg))
    {
        pSensorHandle->isInitialized = false;
        return SENSOR_ERROR_INIT;
    }

    /*! Initialize the sensor handle. */
    pSensorHandle->pCommDrv = pBus;
    pSensorHandle->slaveAddress = sAddress;
    pSensorHandle->isInitialized = true;
    return SENSOR_ERROR_NONE;
}

void FXAS21002_I2C_SetIdleTask(fxas21002_i2c_sensorhandle_t *pSensorHandle,
                               registeridlefunction_t idleTask,
                               void *userParam)
{
    pSensorHandle->deviceInfo.functionParam = userParam;
    pSensorHandle->deviceInfo.idleFunction = idleTask;
}

int32_t FXAS21002_I2C_Configure(fxas21002_i2c_sensorhandle_t *pSensorHandle, const registerwritelist_t *pRegWriteList)
{
    int32_t status;

    /*! Validate for the correct handle and register write list.*/
    if ((pSensorHandle == NULL) || (pRegWriteList == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    /*! Check whether sensor handle is initialized before applying configuration.*/
    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Put the device into standby mode so that configuration can be applied.*/
    status =
        Register_I2C_Write(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                           FXAS21002_CTRL_REG1, FXAS21002_CTRL_REG1_MODE_STANDBY, FXAS21002_CTRL_REG1_MODE_MASK, false);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_WRITE;
    }

    /*! Apply the Sensor Configuration based on the Register Write List */
    status = Sensor_I2C_Write(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                              pRegWriteList);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_WRITE;
    }

    /*! Put the device into active mode and ready for reading data.*/
    status =
        Register_I2C_Write(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                           FXAS21002_CTRL_REG1, FXAS21002_CTRL_REG1_MODE_ACTIVE, FXAS21002_CTRL_REG1_MODE_MASK, false);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_WRITE;
    }

    return SENSOR_ERROR_NONE;
}

int32_t FXAS21002_I2C_ReadData(fxas21002_i2c_sensorhandle_t *pSensorHandle,
                               const registerreadlist_t *pReadList,
                               uint8_t *pBuffer)
{
    int32_t status;

    /*! Validate for the correct handle and register read list.*/
    if ((pSensorHandle == NULL) || (pReadList == NULL) || (pBuffer == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    /*! Check whether sensor handle is initialized before reading sensor data.*/
    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Parse through the read list and read the data one by one. */
    status = Sensor_I2C_Read(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                             pReadList, pBuffer);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_READ;
    }

    return SENSOR_ERROR_NONE;
}

int32_t FXAS21002_I2C_Deinit(fxas21002_i2c_sensorhandle_t *pSensorHandle)
{
    int32_t status;

    if (pSensorHandle == NULL)
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    /*! Check whether sensor handle is initialized before triggering sensor reset.*/
    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Trigger sensor device reset.*/
    status =
        Register_I2C_Write(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                           FXAS21002_CTRL_REG1, FXAS21002_CTRL_REG1_RST_TRIGGER, FXAS21002_CTRL_REG1_RST_MASK, false);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_WRITE;
    }
    else
    {
        /*! De-initialize sensor handle. */
        pSensorHandle->isInitialized = false;
    }

    return SENSOR_ERROR_NONE;
}
