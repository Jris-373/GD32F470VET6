# Driver Layer

This folder contains board-level hardware drivers required by the CIMC contest structure.

- `bsp/`: GD32F470 development-board support APIs for LED, key, UART, ADC, DAC, I2C OLED, SPI Flash, RTC, timer, power, TF card, and SDIO card access.
- Vendor MCU peripheral sources remain under `drivers/` to keep upstream files separate from board support code.
