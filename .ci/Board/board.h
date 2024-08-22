/*------------------------------------------------------------------------------
 * Sensor-SDK-Example
 * Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    board.h
 * Purpose: Board definitions
 *----------------------------------------------------------------------------*/

#ifndef BOARD_H_
#define BOARD_H_

#include "Driver_GPIO.h"
#include "Driver_I2C.h"
#include "Driver_SPI.h"
#include "Driver_USART.h"

// Arduino Connector Digital I/O
#define ARDUINO_UNO_D2   2U
#define ARDUINO_UNO_D3   3U
#define ARDUINO_UNO_D4   4U
#define ARDUINO_UNO_D5   5U
#define ARDUINO_UNO_D6   6U
#define ARDUINO_UNO_D7   7U
#define ARDUINO_UNO_D8   8U
#define ARDUINO_UNO_D9   9U
#define ARDUINO_UNO_D10 10U
#define ARDUINO_UNO_D14 14U
#define ARDUINO_UNO_D15 15U
#define ARDUINO_UNO_D16 16U
#define ARDUINO_UNO_D17 17U
#define ARDUINO_UNO_D18 18U
#define ARDUINO_UNO_D19 19U

// CMSIS Driver instances on Arduino Connector
#define ARDUINO_UNO_I2C     0
#define ARDUINO_UNO_SPI     0
#define ARDUINO_UNO_UART    0

// CMSIS Drivers
extern ARM_DRIVER_GPIO  Driver_GPIO0;
extern ARM_DRIVER_I2C   ARM_Driver_I2C_(ARDUINO_UNO_I2C);       // Arduino I2C
extern ARM_DRIVER_SPI   ARM_Driver_SPI_(ARDUINO_UNO_SPI);       // Arduino SPI
extern ARM_DRIVER_USART ARM_Driver_USART_(ARDUINO_UNO_UART);    // Arduino UART

#ifdef CMSIS_shield_header
#include CMSIS_shield_header
#endif

#endif /* BOARD_H_ */
