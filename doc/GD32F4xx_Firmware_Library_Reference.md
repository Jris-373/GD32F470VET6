# GD32F4xx 固件库与 OLED 例程整理

## 资料来源

- 官方 OLED 例程：`E:\XIMENZI\CIMC-Repository\01_官方资料与例程\GD32F4例程代码(苹果派)\基础例程\10.OLEDDisplay`
- 固件库使用指南：`E:\XIMENZI\GD32F4xx_Firmware_Library_V3.3.3\Docs\User Guide\GD32F4xx_固件库使用指南_Rev1.2.pdf`
- 固件库 Utilities：`E:\XIMENZI\GD32F4xx_Firmware_Library_V3.3.3\Utilities`

## OLED 例程结论

官方 `10.OLEDDisplay` 不是 I2C OLED 例程，而是 SSD1306 的 4 线 GPIO 模拟串行接口例程。原例程使用 `PC10 RES`、`PA4 CS`、`PA5 SCK`、`PA6 DC`、`PA7 DIN`，与当前开发板 I2C OLED 的 `PB8 SCL`、`PB9 SDA` 不匹配。

已保留到项目中的参考文件：

- `third_party/oled_display_example/App/OLED/OLED.c`
- `third_party/oled_display_example/App/OLED/OLED.h`
- `third_party/oled_display_example/App/OLED/OLEDFont.h`
- `Driver/bsp/oled_font.h`

可复用内容：

- SSD1306 初始化命令序列。
- `128 x 8 page` 显存组织方式。
- 清屏、刷新、画点、字符、数字、字符串显示逻辑。
- ASCII 12x6、16x8 字库。

不能直接编译接入的原因：

- 通信方式不一致：官方例程是 GPIO 模拟串行，当前板子是 I2C。
- 原文件依赖 `gd32f470x_conf.h`、`DataType.h`、`SysTick.h`、`DelayNms()`，与当前工程结构不一致。
- 官方例程包含完整 Keil 工程、重复 vendor 库和旧的启动文件，不应覆盖当前 `startup/`、`drivers/`、`.eide/`。

当前项目实现方式：

- `Driver/bsp/bsp_i2c.c/.h`：I2C0 总线，`PB8/PB9`，对外使用 7-bit 地址。
- `Driver/bsp/bsp_oled.c/.h`：SSD1306 I2C OLED，默认 7-bit 地址 `0x3C`，按开发板 0.91 寸 `128x32` OLED 配置，命令控制字 `0x00`，数据控制字 `0x40`。

## 固件库 API 参考

I2C 参考接口：

- `i2c_clock_config()`
- `i2c_mode_addr_config()`
- `i2c_start_on_bus()`
- `i2c_master_addressing()`
- `i2c_data_transmit()`
- `i2c_data_receive()`
- `i2c_flag_get()` / `i2c_flag_clear()`

SPI 参考接口：

- `spi_parameter_struct`
- `spi_i2s_deinit()`
- `spi_struct_para_init()`
- `spi_init()`
- `spi_enable()`
- `spi_i2s_data_transmit()`
- `spi_i2s_data_receive()`
- `spi_i2s_flag_get()`

RTC 参考接口：

- `pmu_backup_write_enable()`
- `rcu_rtc_clock_config()`
- `rtc_register_sync_wait()`
- `rtc_init()`
- `rtc_current_time_get()`

RTC 时间字段采用 BCD 格式，项目 BSP 对外使用普通二进制时间，在 `bsp_rtc.c` 内部做 BCD 转换。

TIMER 参考接口：

- `timer_parameter_struct`
- `timer_struct_para_init()`
- `timer_init()`
- `timer_interrupt_enable()`
- `timer_interrupt_flag_get()`
- `timer_interrupt_flag_clear()`

当前项目使用 `TIMER6` 作为基础周期定时器，ISR 路径为：

`TIMER6_IRQHandler()` -> `nvic_timer6_callback()` -> `bsp_timer_irq_handler()`

## Utilities 结论

- `Utilities/gd32f450i_eval.c/.h` 可参考官方 BSP 风格，但引脚属于 GD32F450I-EVAL，不适合直接搬入。
- `Utilities/gd32f450i_lcd_eval.c/.h` 和 `Utilities/LCD_common` 是 TLI/RGB LCD 方向，不是 I2C OLED。
- `Utilities/Third_Party/fat_fs` 后续做 SD 卡或文件系统时再考虑，当前 SPI Flash 原始读写阶段不需要。

## 新增 BSP 模块

- `Driver/bsp/bsp_i2c.c/.h`：I2C0 阻塞式读写、寄存器读写、设备探测。
- `Driver/bsp/bsp_oled.c/.h`：I2C SSD1306 OLED 初始化、开关显示、清屏、填充、刷新、画点、显示字符/字符串/数字。
- `Driver/bsp/bsp_spi_flash.c/.h`：GD25Q40E 类 SPI Nor Flash 读 ID、读数据、页写、跨页写、扇区擦除、整片擦除。
- `Driver/bsp/bsp_rtc.c/.h`：RTC 时钟初始化、配置标志、设置/读取日期时间；优先 LXTAL，失败时回退 IRC32K，并按时钟源选择分频。
- `Driver/bsp/bsp_timer.c/.h`：TIMER6 周期中断、elapsed 标志、tick 计数；按 APB1 分频状态计算 TIMER6 输入时钟。

## EIDE Source 更新

已补入 `.eide/eide.yml`：

- `drivers/GD32F4xx_standard_peripheral/Source/gd32f4xx_i2c.c`
- `drivers/GD32F4xx_standard_peripheral/Source/gd32f4xx_spi.c`
- `drivers/GD32F4xx_standard_peripheral/Source/gd32f4xx_pmu.c`
- `drivers/GD32F4xx_standard_peripheral/Source/gd32f4xx_rtc.c`

暂未加入：

- `gd32f4xx_tli.c`
- `gd32f4xx_exmc.c`
- `Utilities/gd32f450i_lcd_eval.c`
- `Utilities/LCD_common/*`
- FatFs 相关文件

这些文件属于 RGB LCD、外部存储控制器或文件系统方向，和当前 I2C OLED、SPI Flash、RTC、基础定时器 BSP 不是同一阶段。

## 硬件注意点

- I2C OLED 和 EEPROM 共用 `PB8/PB9`，后续 EEPROM BSP 应复用 `bsp_i2c0_*`，不要重复初始化成另一套 I2C 驱动。
- SPI Flash 使用 `PB3 SCK`、`PB4 MISO`、`PB5 MOSI`、`PA15 CS`。这些脚与 JTAG 相关，调试保留 SWD，避免 JTAG 占用。
- RTC 优先使用 `LXTAL`，若外部 32.768 kHz 晶振不起振，当前 BSP 会尝试回退到 `IRC32K`。
