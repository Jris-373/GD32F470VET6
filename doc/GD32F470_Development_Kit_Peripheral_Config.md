# GD32F470 Development Kit V2.0 外设配置导出

来源：

- `E:\XIMENZI\docs\1_GD32F470 Development Kit V2.0 原理图.pdf`
- `E:\XIMENZI\docs\CIMC_GD32F470_Dev_Kit_User_Manual.pdf`

说明：本文件按原理图网名整理外设连接，用于后续 BSP 驱动和 `main.c` 外设验证。若原理图只给出功能模块网名但未直接连到 MCU 管脚，本文件标注为“需跳线/需确认”。

## MCU 与系统基础

| 项目 | 配置 |
| --- | --- |
| MCU | GD32F470VET6 |
| 主晶振 | `OSC24M_IN` / `OSC24M_OUT`，连接到 `PH0` / `PH1` |
| 低速晶振 | `OSC32K_IN` / `OSC32K_OUT`，连接到 `PC14` / `PC15` |
| 复位 | `NRST`，10k 上拉到 3.3V，按键拉低复位 |
| 调试 | `SWDIO` / `SWCLK`，连接到 `PA13` / `PA14` |
| 启动 | `BOOT0` 通过拨码/跳线配置；`BOOT1` 在启动配置区引出 |
| 唤醒按键 | `WK_UP`，连接到 `PA0` |

## LED

| 模块网名 | 电路 | MCU 管脚 |
| --- | --- | --- |
| `LED1` | 串 2k 电阻后接 LED 到 `DGND` | 不指定 I/O，需从 H4 连接到选通 GPIO |
| `LED2` | 串 2k 电阻后接 LED 到 `DGND` | 不指定 I/O，需从 H4 连接到选通 GPIO |
| `LED3` | 串 2k 电阻后接 LED 到 `DGND` | 不指定 I/O，需从 H4 连接到选通 GPIO |
| `LED4` | 串 2k 电阻后接 LED 到 `DGND` | 不指定 I/O，需从 H4 连接到选通 GPIO |
| `LED5` | 串 2k 电阻后接 LED 到 `DGND` | 不指定 I/O，需从 H4 连接到选通 GPIO |
| `LED6` | 串 2k 电阻后接 LED 到 `DGND` | 不指定 I/O，需从 H4 连接到选通 GPIO |

LED 输入网名为高电平点亮：GPIO 输出高电平时电流经电阻和 LED 流向 `DGND`。官方手册确认 6 路 LED 为共阴极设计，选通端不指定 I/O，用户按需求将 H4 中端口连接到选通 I/O。

## 按键

| 模块网名 | 电路 | MCU 管脚 |
| --- | --- | --- |
| `FUN_KEY1` | 10k 上拉到 3.3V，按键按下接 `DGND`，并联 100nF 去抖电容 | 不指定 I/O，需从 H3 连接到选通 GPIO |
| `FUN_KEY2` | 同上 | 不指定 I/O，需从 H3 连接到选通 GPIO |
| `FUN_KEY3` | 同上 | 不指定 I/O，需从 H3 连接到选通 GPIO |
| `FUN_KEY4` | 同上 | 不指定 I/O，需从 H3 连接到选通 GPIO |
| `FUN_KEY5` | 同上 | 不指定 I/O，需从 H3 连接到选通 GPIO |
| `FUN_KEY6` | 同上 | 不指定 I/O，需从 H3 连接到选通 GPIO |

按键为低有效：未按下读高电平，按下读低电平。官方手册确认 6 路用户按键选通端不指定 I/O，用户按需求将 H3 中端口连接到选通 I/O。

## I2C OLED 与 EEPROM

| 外设 | 器件/接口 | 信号 | MCU 管脚 | 备注 |
| --- | --- | --- | --- | --- |
| OLED | `OLED1` | `OLED_CLK` | `PB8` | 与 `E_SCL` 共用 |
| OLED | `OLED1` | `OLED_DAT` | `PB9` | 与 `E_SDA` 共用 |
| EEPROM | M24C08 | `E_SCL` | `PB8` | 10k 上拉到 3.3V |
| EEPROM | M24C08 | `E_SDA` | `PB9` | 10k 上拉到 3.3V |

建议 BSP 抽象为同一条 I2C 总线，分别挂载 OLED 和 EEPROM 设备。

## SPI Flash

| 器件 | 信号 | MCU 管脚 | 备注 |
| --- | --- | --- | --- |
| GD25Q40E | `SPI_SCK` | `PB3` | SPI 时钟 |
| GD25Q40E | `SPI_MISO` | `PB4` | Flash SO |
| GD25Q40E | `SPI_MOSI` | `PB5` | Flash SI |
| GD25Q40E | `FLASH_CS` | `PA15` | 片选，10k 上拉到 3.3V |
| GD25Q40E | `WP` / `HOLD` | 3.3V | 固定为非写保护/非保持 |

注意：`PB3`、`PB4`、`PA15` 同时属于 JTAG 相关管脚。若使用这些 GPIO/SPI 功能，固件中需要关闭 JTAG 并保留 SWD 调试。

## TF / SD 卡

| 信号 | MCU 管脚 | 备注 |
| --- | --- | --- |
| `SD_CLK` | `PC12` | SDIO 时钟 |
| `SD_CMD` | `PD2` | SDIO 命令 |
| `SD_DAT0` | `PC9` | 数据线 0 |
| `SD_DAT1` | `PC8` | 数据线 1 |
| `SD_DAT2` | `PC10` | 数据线 2 |
| `SD_DAT3` | `PC11` | 数据线 3 |
| `SD_CD` | `PE2` | 卡检测，10k 上拉到 3.3V |

## ADC / DAC / 模拟参考

| 功能 | 信号 | MCU 管脚 | 备注 |
| --- | --- | --- | --- |
| 电位器 ADC | `ADC_IN10` | `PC0` | 10k 电位器，接 3.3V 与 `DGND`，滑动端输出 |
| ADC 外部基准校准 | `ADC_IN12` | `PC2` | 通过 H7 选通接入 TL431 外部基准；选通时作为满度校准，不用于普通测量 |
| DAC0 | `DAC0_OUT` | `PA4` | 模拟输出 |
| DAC1 | `DAC1_OUT` | `PA5` | 模拟输出 |
| ADC 参考 | `3.3VREF` | 模拟参考网络 | TL431 为 MCU Vref 区域供电；H7 短接后可读到约 4096 满度值 |

## USART / 串口相关

| 功能 | 信号 | MCU 管脚 | 备注 |
| --- | --- | --- | --- |
| USART0 | `USART0_TX` | `PA9` | 可通过跳线连接 USB 转串口 |
| USART0 | `USART0_RX` | `PA10` | 可通过跳线连接 USB 转串口 |
| USART1 | `USART1_TX` | `PD5` | 通过 H10 选通连接 RS485/RS232；未短接时该 IO 独立 |
| USART1 | `USART1_RX` | `PD6` | 通过 H10 选通连接 RS485/RS232；未短接时该 IO 独立 |
| USART2 | `USART2_TX` | `PD8` | 通过 H11 选通连接 TTL 串口或通信扩展板；未短接时该 IO 独立 |
| USART2 | `USART2_RX` | `PD9` | 通过 H11 选通连接 TTL 串口或通信扩展板；未短接时该 IO 独立 |
| USART5 | `USART5_TX` | `PC6` | 传感器接口引出 |
| USART5 | `USART5_RX` | `PC7` | 传感器接口引出 |

## USB 转串口

| 器件 | 信号 | 连接 |
| --- | --- | --- |
| CH340C | `USB_TX` | 通过 H12 短接到 `PA10` / `USART0_RX` |
| CH340C | `USB_RX` | 通过 H12 短接到 `PA9` / `USART0_TX` |
| USB 接口 | `CH340_D+` / `CH340_D-` | 连接 USB D+ / D- |

USB 转串口与 MCU USART0 之间通过 H12 上下短接连接，代码侧使用 `PA9/PA10`。

## RS485

| 器件 | 信号 | MCU/连接 | 备注 |
| --- | --- | --- | --- |
| SP3485 | `485_TX` | 通过 H10 靠左选通到 `PD5` / `USART1_TX` | 发送数据输入 |
| SP3485 | `485_RX` | 通过 H10 靠左选通到 `PD6` / `USART1_RX` | 接收数据输出 |
| SP3485 | `485_CS` | `PE8` | DE/RE 方向控制 |
| 接口 | `485_A` / `485_B` | RS485 差分总线 | 端接/偏置电阻在原理图中给出 |

建议 BSP 提供方向控制 API：发送前置 `485_CS`，发送完成后释放为接收。

## RS232

| 器件 | 信号 | 连接 |
| --- | --- | --- |
| SP3232 | `232_TX` / `232_RX` | 经电平转换连接外部 RS232 接口 |
| SP3232 | `232_OUT_TX` / `232_OUT_RX` | 接口侧信号 |
| H10 | `USART1_TX` / `USART1_RX` | H10 靠右短接时选通 SP3232，靠左短接时选通 SP3485 |

## Sensor_Interface / 传感扩展板

`Sensor_Interface` 对应用户手册中的“传感扩展板”。U10 为传感扩展板安装区域，用于连接不同功能传感扩展板实现数据采集。连接端口左侧第 3 引脚为 `NC`，用于防反插识别；H5、H6 为两侧端子引出，可按功能需要测量或连接到 I/O。

| 接口信号 | MCU 管脚/总线 | 备注 |
| --- | --- | --- |
| `S_SPI_CS` | `PE10` | 传感扩展板 SPI 片选 |
| `S_SPI_SCK` / `S_SCK` | `PE12` / `SPI3_SCK` | 与通信扩展板共用 SPI3 时钟 |
| `S_SPI_MISO` / `S_MISO` | `PE13` / `SPI3_MISO` | 与通信扩展板共用 SPI3 MISO |
| `S_SPI_MOSI` / `S_MOSI` | `PE14` / `SPI3_MOSI` | 与通信扩展板共用 SPI3 MOSI |
| `S_UARTTX` | `PC6` / `USART5_TX` | 传感扩展板 UART TX |
| `S_UARTRX` | `PC7` / `USART5_RX` | 传感扩展板 UART RX |
| `S_IICSCL` / `S_IICSDA` | 接口网名 | 传感器 I2C 接口，按实际扩展板确认 |
| `S_ADC` / `S_DAC` | 接口网名 | 传感器模拟输入/输出，按实际扩展板确认 |
| `S_GPIO1..5` | 未连接到指定 MCU 端口 | 通过 H5/H6 引出，用户自行连接到需要的 I/O |
| `3V3` / `5V` / `DGND` | 电源 | 接口供电 |

编写 BSP 时优先按实际扩展板类型选择 SPI3、USART5、I2C、ADC/DAC 或 GPIO。`S_GPIO1..5` 不是固定 MCU 管脚，不能直接在代码中使用，必须先确认跳线或外部连接。

## WAN_Interface / 通信扩展板

`WAN_Interface` 对应用户手册中的“通信扩展板”。U17 为通信扩展板连接区域，可连接不同通信扩展板实现联网或通信功能。连接端口右侧第 5 引脚为 `NC`，用于防反插识别；H13、H14 为两侧端子引出，可按功能需要测量或连接到 I/O。

| 接口信号 | MCU 管脚/总线 | 备注 |
| --- | --- | --- |
| `W_RST` | `PA3` | 通信扩展板复位；未使用通信扩展板时可作普通 IO |
| `W_SPI_CS` | `PE9` | 通信扩展板 SPI 片选 |
| `W_SPI_SCK` | `PE12` / `SPI3_SCK` | 与传感扩展板共用 SPI3 时钟 |
| `W_SPI_MISO` | `PE13` / `SPI3_MISO` | 与传感扩展板共用 SPI3 MISO |
| `W_SPI_MOSI` | `PE14` / `SPI3_MOSI` | 与传感扩展板共用 SPI3 MOSI |
| `USART2_TX` | `PD8` | 通过 H11 选通到通信扩展板；H11 靠下短接 |
| `USART2_RX` | `PD9` | 通过 H11 选通到通信扩展板；H11 靠下短接 |
| `W_GPIO1..6` | 未连接到指定 MCU 端口 | 通过 H13/H14 引出，用户自行连接到需要的 I/O |
| `3V3` / `5V` / `DGND` | 电源 | 接口供电 |

`WAN_Interface` 不是板载以太网；板载以太网是 LAN8720A + RJ45。它是通信模块扩展座，可用于 `CIMC_Comm_Module` 或其他通信子板。SPI3 总线同时服务通信扩展板和传感扩展板，因此同时使用两类扩展板时必须管理片选 `PE9` / `PE10`，并确认总线电气兼容。

## Ethernet

| 器件 | 信号 | MCU 管脚 | 备注 |
| --- | --- | --- | --- |
| LAN8720A | `ETH_MDIO` | `PA2` | PHY 管理数据 |
| LAN8720A | `ETH_MDC` | `PC1` | PHY 管理时钟 |
| LAN8720A | `ETH_REF_CLK` | `PA1` | RMII 参考时钟 |
| LAN8720A | `ETH_CRS_DV` | `PA7` | RMII CRS/DV |
| LAN8720A | `ETH_RXD0` | `PC4` | RMII RXD0 |
| LAN8720A | `ETH_RXD1` | `PC5` | RMII RXD1 |
| LAN8720A | `ETH_TXD0` | `PB12` | RMII TXD0 |
| LAN8720A | `ETH_TXD1` | `PB13` | RMII TXD1 |
| LAN8720A | `ETH_TX_EN` | `PB11` | RMII TX enable |
| LAN8720A | `PHY_RST` | `PD3` | PHY 复位，不建议作普通 IO |
| RJ45 | `ETH_TXP/TXN`, `ETH_RXP/RXN` | PHY 差分线 | 带 Link/Speed LED |

LAN8720A 处于复位状态下，部分 RMII 数据脚可作为普通 IO 使用；但 `ETH_REF_CLK`、`ETH_MDIO` 等 PHY 管理/时钟信号不建议复用。

## 外部扩展排针

原理图提供两组 2x20 MCU IO 排针，直接引出大量 GPIO，包括 `PAx`、`PBx`、`PCx`、`PDx`、`PEx` 以及 3.3V/GND。LED、KEY 等未固定到 MCU 的板载功能模块应通过 H3/H4 与 MCU IO 排针跳接验证。

## BSP 开发建议顺序

1. 先从 MCU IO 排针选择一个空闲 GPIO，经 H4 跳接到 `LED1`，验证高电平点亮。
2. LED 验证通过后，经 H3 跳接 `FUN_KEY1` 到一个空闲 GPIO，验证低有效输入和去抖。
3. 使用 `PB8/PB9` 建立 I2C BSP，先驱动 OLED，再读写 EEPROM。
4. 使用 `PB3/PB4/PB5 + PA15` 驱动 SPI Flash，注意关闭 JTAG。
5. 使用 `PA9/PA10` 验证 USB 转串口。
6. 使用 `PD5/PD6 + PE8`，并将 H10 靠左短接，验证 RS485。
7. 使用 `PD8/PD9`，并将 H11 靠上短接，验证 TTL USART2；H11 靠下短接时用于 `WAN_Interface` 通信扩展板。
8. 使用 `PC6/PC7` 或 `PE10/PE12/PE13/PE14` 验证 `Sensor_Interface`。
9. 后续再推进 TF、ADC/DAC、`WAN_Interface`、Ethernet。

## 待硬件确认项

- LED1-LED6 需要通过 H4 自行连接到 MCU IO。
- FUN_KEY1-FUN_KEY6 需要通过 H3 自行连接到 MCU IO。
- `Sensor_Interface` 中 `S_GPIO1..5` 未固定到 MCU，需按实际扩展板或跳线确认。
- `WAN_Interface` 中 `W_GPIO1..6` 未固定到 MCU，需按实际扩展板或跳线确认。
- H10 的实际短接方向：靠左为 SP3485/RS485，靠右为 SP3232/RS232。
- H11 的实际短接方向：靠上为 TTL USART2，靠下为通信扩展板。
