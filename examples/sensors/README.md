# Known combination issues

## fxas21002
Project: interrupt
  - Board: LPCXpresso55S69  
    Shield: AGM01  
    Issue: Blue LED clashes with ARDUINO_UNO_D5 Interrupt 1

## fxls8974cf
Project: motion_wakeup
  - Board: EVK-MIMXRT1060  
    Shield: A8974  
    Issue: No Red LED

  - Board: LPCXpresso54114  
    Shield: A8974  
    Issue: Red LED clashes with ARDUINO_UNO_D5 (overrides I2C/SPI jumper selection)
