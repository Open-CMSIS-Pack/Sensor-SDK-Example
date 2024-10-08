/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fxos8700_drv_i2c.c
 * @brief The fxos8700_drv_i2c.c file implements the fxos8700 sensor driver functional interfaces.
 */

//-----------------------------------------------------------------------
// ISSDK Includes
//-----------------------------------------------------------------------
#include "gpio_driver.h"
#include "fxos8700_drv_i2c.h"

//-----------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------
int32_t FXOS8700_I2C_Initialize(
    fxos8700_i2c_sensorhandle_t *pSensorHandle, ARM_DRIVER_I2C *pBus, uint8_t index, uint16_t sAddress, uint8_t whoAmi)
{
    int32_t status;
    FXOS8700_WHO_AM_I_t reg;

    if ((pSensorHandle == NULL) || (pBus == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    pSensorHandle->deviceInfo.deviceInstance = index;
    pSensorHandle->deviceInfo.functionParam = NULL;
    pSensorHandle->deviceInfo.idleFunction = NULL;

    /*!  Read and store the device's WHO_AM_I.*/
    status = Register_I2C_Read(pBus, &pSensorHandle->deviceInfo, sAddress, FXOS8700_WHO_AM_I, 1, &reg);
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

void FXOS8700_I2C_SetIdleTask(fxos8700_i2c_sensorhandle_t *pSensorHandle,
                              registeridlefunction_t idleTask,
                              void *userParam)
{
    pSensorHandle->deviceInfo.functionParam = userParam;
    pSensorHandle->deviceInfo.idleFunction = idleTask;
}

int32_t FXOS8700_I2C_Configure(fxos8700_i2c_sensorhandle_t *pSensorHandle, const registerwritelist_t *pRegWriteList)
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

    /* Put the device into standby mode so that configuration can be applied.*/
    status = Register_I2C_Write(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                                FXOS8700_CTRL_REG1, FXOS8700_CTRL_REG1_ACTIVE_STANDBY_MODE,
                                FXOS8700_CTRL_REG1_ACTIVE_MASK, false);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_WRITE;
    }

    /* Appy the Sensor Configuration based on the Register List */
    status = Sensor_I2C_Write(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                              pRegWriteList);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_WRITE;
    }

    /* Put the device into active mode and ready for reading data.*/
    status = Register_I2C_Write(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                                FXOS8700_CTRL_REG1, FXOS8700_CTRL_REG1_ACTIVE_ACTIVE_MODE,
                                FXOS8700_CTRL_REG1_ACTIVE_MASK, false);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_WRITE;
    }

    return SENSOR_ERROR_NONE;
}

int32_t FXOS8700_I2C_ReadData(fxos8700_i2c_sensorhandle_t *pSensorHandle,
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

    /*!  Parse through the read list and read the data one by one*/
    status = Sensor_I2C_Read(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                             pReadList, pBuffer);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_READ;
    }

    return SENSOR_ERROR_NONE;
}

int32_t FXOS8700_I2C_Deinit(fxos8700_i2c_sensorhandle_t *pSensorHandle)
{
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
    Register_I2C_Write(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                       FXOS8700_CTRL_REG2, FXOS8700_CTRL_REG2_RST_EN, FXOS8700_CTRL_REG2_RST_MASK, false);
    pSensorHandle->isInitialized = false;
    return SENSOR_ERROR_NONE;
}
