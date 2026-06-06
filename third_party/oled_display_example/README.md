# OLEDDisplay Official Example

Source path:

`E:\XIMENZI\CIMC-Repository\01_官方资料与例程\GD32F4例程代码(苹果派)\基础例程\10.OLEDDisplay`

Copied files:

- `App/OLED/OLED.c`
- `App/OLED/OLED.h`
- `App/OLED/OLEDFont.h`

This example is kept as a reference only and is not added to EIDE build sources.
The official example drives SSD1306 through a 4-wire GPIO simulated serial
interface using `PA4/PA5/PA6/PA7` and `PC10`, while the current development kit
uses I2C OLED on `PB8/PB9`. The project BSP reuses the SSD1306 init sequence,
GRAM layout, and ASCII font data, but rewrites the transport layer in
`Driver/bsp/bsp_oled.c` on top of `Driver/bsp/bsp_i2c.c`.
