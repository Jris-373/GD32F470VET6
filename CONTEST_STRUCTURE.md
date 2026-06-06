# 2026 CIMC Contest Structure Notes

The contest PDF requires the Bootloader and App projects to place code in three layers:

- `Driver`: UART, IIC/I2C, SPI, OLED, Flash, RTC, and other low-level drivers.
- `Protocol`: frame parsing, response framing, ASCII hex handling, and CRC.
- `Function`: sampling, ratios, thresholds, alarms, parameter management, update state, and other business logic.

The internal Flash layout now follows the contest map:

| Region | Start | End | Size |
| --- | --- | --- | --- |
| Bootloader | `0x08000000` | `0x0800FFFF` | 64K |
| Parameters | `0x08010000` | `0x08010FFF` | 4K |
| App | `0x08011000` | `0x08030FFF` | 128K |
| App backup | `0x08031000` | `0x08050FFF` | 128K |
| Firmware staging | `0x08051000` | `0x08070FFF` | 128K |

Current state:

- `Driver/bsp` contains the existing board support code.
- `Function/boot` and `Function/app` contain the existing Bootloader/App update logic.
- `Protocol` contains the base ASCII-hex, CRC16-Modbus, and frame parser/builder helpers.
- EIDE still uses two targets, `Bootloader` and `App`; the next major contest step is to split or export these into two separate submitted project folders if required by the final packaging workflow.
