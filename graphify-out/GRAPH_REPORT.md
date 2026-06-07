# Graph Report - .  (2026-06-05)

## Corpus Check
- 141 files · ~273,470 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1598 nodes · 2438 edges · 41 communities detected
- Extraction: 71% EXTRACTED · 29% INFERRED · 0% AMBIGUOUS · INFERRED: 704 edges (avg confidence: 0.51)
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `move_window()` - 22 edges
2. `find_volume()` - 22 edges
3. `dir_register()` - 18 edges
4. `fmc_ready_wait()` - 17 edges
5. `ld_word()` - 16 edges
6. `r1_error_check()` - 16 edges
7. `system_clock_config()` - 16 edges
8. `follow_path()` - 15 edges
9. `f_rename()` - 15 edges
10. `get_fat()` - 14 edges

## Surprising Connections (you probably didn't know these)
- `Firmware Layering Rules Community` --semantically_similar_to--> `GD32F470 EIDE Firmware Project`  [INFERRED] [semantically similar]
  graphify-out\GRAPH_REPORT.md → AGENTS.md
- `Driver README Driver Layer` --semantically_similar_to--> `Driver/bsp Board-Support Layer`  [INFERRED] [semantically similar]
  Driver\README.md → AGENTS.md
- `Protocol README Protocol Layer` --semantically_similar_to--> `Protocol Layer`  [INFERRED] [semantically similar]
  Protocol\README.md → AGENTS.md
- `Function README Function Layer` --semantically_similar_to--> `Function Layer`  [INFERRED] [semantically similar]
  Function\README.md → AGENTS.md
- `Contest Protocol Layer` --semantically_similar_to--> `Protocol README Protocol Layer`  [INFERRED] [semantically similar]
  CONTEST_STRUCTURE.md → Protocol\README.md

## Hyperedges (group relationships)
- **Firmware Layered Architecture** — agents_src_main_flow, agents_driver_bsp_layer, agents_protocol_layer, agents_function_layer, agents_startup_layer, agents_drivers_vendor_layer [EXTRACTED 1.00]
- **Contest Internal Flash Update Map** — contest_structure_bootloader_region, contest_structure_parameters_region, contest_structure_app_region, contest_structure_app_backup_region, contest_structure_firmware_staging_region [EXTRACTED 1.00]
- **Shared I2C OLED EEPROM BSP Pattern** — gd32f470_development_kit_peripheral_config_i2c_pb8_pb9_bus, gd32f470_development_kit_peripheral_config_oled1_i2c, gd32f470_development_kit_peripheral_config_eeprom_m24c08, gd32f4xx_firmware_library_reference_bsp_i2c_driver, gd32f4xx_firmware_library_reference_bsp_oled_driver [INFERRED 0.86]

## Communities

### Community 0 - "GD32F4xx DAC Driver"
Cohesion: 0.02
Nodes (2): nvic_irq_enable(), nvic_priority_group_set()

### Community 1 - "FatFs Filesystem"
Cohesion: 0.07
Nodes (99): change_bitmap(), check_fs(), chk_chr(), chk_lock(), clear_lock(), clmt_clust(), clst2sect(), cmp_lfn() (+91 more)

### Community 2 - "GD32F4xx ENET Driver"
Cohesion: 0.03
Nodes (17): enet_default_init(), enet_deinit(), enet_disable(), enet_enable(), enet_init(), enet_initpara_reset(), enet_phy_config(), enet_phy_write_read() (+9 more)

### Community 3 - "GD32F4xx TIMER Driver"
Cohesion: 0.03
Nodes (9): timer_channel_input_capture_prescaler_config(), timer_external_clock_mode0_config(), timer_external_clock_mode1_config(), timer_external_trigger_as_external_clock_config(), timer_external_trigger_config(), timer_input_capture_config(), timer_input_pwm_capture_config(), timer_input_trigger_source_select() (+1 more)

### Community 4 - "CMSIS Core"
Cohesion: 0.03
Nodes (2): NVIC_SetPriority(), SysTick_Config()

### Community 5 - "Recommended Bsp Development Order"
Cohesion: 0.04
Nodes (58): Contest Driver Layer, Driver README BSP Peripheral APIs, Driver README Driver Layer, Driver README Vendor MCU Source Separation, Recommended BSP Development Order, M24C08 EEPROM I2C Device, LAN8720A RMII Ethernet, Hardware Confirmation Items (+50 more)

### Community 6 - "GD32F4xx USART Driver"
Cohesion: 0.04
Nodes (0): 

### Community 7 - "CMSIS Core"
Cohesion: 0.04
Nodes (0): 

### Community 8 - "Gd32F470 Eide Firmware Project"
Cohesion: 0.05
Nodes (49): Agent Graphify Project Memory, build Generated Artifacts, docs Hardware Manuals and Contest Documents, Driver/bsp Board-Support Layer, drivers MCU Vendor Layer, Function Layer, GD32F470 EIDE Firmware Project, Protocol Layer (+41 more)

### Community 9 - "GD32F4xx RCU Driver"
Cohesion: 0.04
Nodes (3): rcu_deinit(), rcu_flag_get(), rcu_osci_stab_wait()

### Community 10 - "GD32F4xx ADC Driver"
Cohesion: 0.04
Nodes (0): 

### Community 11 - "GD32F4xx EXMC Driver"
Cohesion: 0.04
Nodes (0): 

### Community 12 - "GD32F4xx SDIO Driver"
Cohesion: 0.04
Nodes (0): 

### Community 13 - "GD32F4xx RTC Driver"
Cohesion: 0.07
Nodes (11): rtc_coarse_calibration_config(), rtc_coarse_calibration_disable(), rtc_coarse_calibration_enable(), rtc_deinit(), rtc_init(), rtc_init_mode_enter(), rtc_init_mode_exit(), rtc_refclock_detection_disable() (+3 more)

### Community 14 - "GD32F4xx FMC Driver"
Cohesion: 0.07
Nodes (17): fmc_bank0_erase(), fmc_bank1_erase(), fmc_byte_program(), fmc_halfword_program(), fmc_mass_erase(), fmc_page_erase(), fmc_ready_wait(), fmc_sector_erase() (+9 more)

### Community 15 - "GD32F4xx I2C Driver"
Cohesion: 0.05
Nodes (0): 

### Community 16 - "GD32F4xx SPI Driver"
Cohesion: 0.05
Nodes (0): 

### Community 17 - "BSP Sdcard"
Cohesion: 0.12
Nodes (31): cmdsent_error_check(), command_response_wait(), dma_receive_config(), dma_transfer_config(), gpio_config(), r1_error_check(), r1_error_type_check(), r2_error_check() (+23 more)

### Community 18 - "Bootloader Update"
Cohesion: 0.12
Nodes (28): app_update_build_header(), app_update_emit_progress(), app_update_ext_flash_erase(), app_update_stage_tf_firmware(), boot_crc32_calc(), boot_crc32_finish(), boot_crc32_start(), boot_crc32_update() (+20 more)

### Community 19 - "Bootloader Update"
Cohesion: 0.12
Nodes (31): boot_usart_app_poll(), boot_usart_apply_staged_firmware(), boot_usart_bootloader_upgrade_window(), boot_usart_calc_expected_ascii_len(), boot_usart_frame_matches_device(), boot_usart_frame_receiver_reset(), boot_usart_handle_common_frame(), boot_usart_handle_receive_command() (+23 more)

### Community 20 - "BSP Tf"
Cohesion: 0.1
Nodes (21): bsp_rtc_from_bcd(), bsp_rtc_get_datetime(), bsp_rtc_init(), bsp_rtc_select_clock_source(), bsp_rtc_set_datetime(), bsp_rtc_to_bcd(), bsp_rtc_year_to_bcd(), append_char() (+13 more)

### Community 21 - "GD32F4xx DMA Driver"
Cohesion: 0.06
Nodes (0): 

### Community 22 - "Bsp Key"
Cohesion: 0.11
Nodes (23): bsp_key_id_is_valid(), bsp_key_init(), bsp_key_is_pressed(), bsp_key_is_pressed_id(), bsp_key_read(), bsp_key_read_id(), bsp_key_scan_event(), bsp_key_scan_event_id() (+15 more)

### Community 23 - "BSP Dac"
Cohesion: 0.09
Nodes (14): bsp_adc_raw_to_mv(), bsp_adc_read_mv(), bsp_adc_read_mv_timeout(), bsp_adc_read_raw(), bsp_adc_read_raw_timeout(), bsp_dac_get_mv(), bsp_dac_init(), bsp_dac_mv_to_raw() (+6 more)

### Community 24 - "Nvic"
Cohesion: 0.1
Nodes (12): nvic_busfault_callback(), nvic_capture_fault(), nvic_capture_stack(), nvic_debugmon_callback(), nvic_fault_loop(), nvic_hardfault_callback(), nvic_hardfault_callback_with_stack(), nvic_memmanage_callback() (+4 more)

### Community 25 - "GD32F4xx CAN Driver"
Cohesion: 0.08
Nodes (3): can_error_get(), can_interrupt_flag_get(), can_receive_message_length_get()

### Community 26 - "GD32F4xx IPA Driver"
Cohesion: 0.08
Nodes (0): 

### Community 27 - "GD32F4xx TLI Driver"
Cohesion: 0.08
Nodes (0): 

### Community 28 - "BSP Oled"
Cohesion: 0.17
Nodes (23): bsp_i2c0_is_device_ready(), bsp_i2c0_read(), bsp_i2c0_read_reg(), bsp_i2c0_send_start_and_address(), bsp_i2c0_timeout_limit(), bsp_i2c0_wait_flag(), bsp_i2c0_wait_stop(), bsp_i2c0_write() (+15 more)

### Community 29 - "Main"
Cohesion: 0.23
Nodes (23): app_current_version(), app_show_version(), app_update_oled_progress(), boot_poll_keys(), boot_show_menu(), boot_stage_firmware_from_tf(), boot_status_text(), boot_try_jump_app() (+15 more)

### Community 30 - "GD32F4xx CTC Driver"
Cohesion: 0.1
Nodes (0): 

### Community 31 - "GD32F4xx PMU Driver"
Cohesion: 0.1
Nodes (2): pmu_flag_get(), pmu_highdriver_switch_select()

### Community 32 - "BSP Uart"
Cohesion: 0.17
Nodes (18): bsp_uart0_byte_available(), bsp_uart0_irq_handler(), bsp_uart0_next_index(), bsp_uart0_read_byte(), bsp_uart0_read_char(), bsp_uart0_send_byte(), bsp_uart0_send_char(), bsp_uart0_send_string() (+10 more)

### Community 33 - "CMSIS Core"
Cohesion: 0.1
Nodes (2): _exit(), _kill()

### Community 34 - "CMSIS Core"
Cohesion: 0.18
Nodes (17): _soft_delay_(), system_clock_120m_25m_hxtal(), system_clock_120m_8m_hxtal(), system_clock_120m_irc16m(), system_clock_168m_25m_hxtal(), system_clock_168m_8m_hxtal(), system_clock_168m_irc16m(), system_clock_16m_irc16m() (+9 more)

### Community 35 - "BSP Spi Flash"
Cohesion: 0.48
Nodes (14): bsp_spi_flash_chip_erase(), bsp_spi_flash_cs_high(), bsp_spi_flash_cs_low(), bsp_spi_flash_init(), bsp_spi_flash_page_program(), bsp_spi_flash_read(), bsp_spi_flash_read_id(), bsp_spi_flash_read_status() (+6 more)

### Community 36 - "App And Bootloader Eide Targets"
Cohesion: 0.29
Nodes (7): App and Bootloader EIDE Targets, Build and Flash Validation, VS Code EIDE Build Workflow, Rationale: Build and Flash Because No Unit Tests Exist, EIDE Bootloader and App Targets, Separate Submitted Bootloader and App Project Folders, Rationale: Final Packaging May Require Separate Project Folders

### Community 37 - "Gd32F470 Development Kit V2 Peripheral"
Cohesion: 0.4
Nodes (5): GD32F470 Development Kit V2 Peripheral Config Export, GD32F470VET6 MCU, MCU and System Base Configuration, GD32F470 Development Kit V2 Schematic PDF, CIMC GD32F470 Dev Kit User Manual PDF

### Community 38 - "Timer6 Irq To Bsp Timer"
Cohesion: 1.0
Nodes (2): TIMER6 IRQ to BSP Timer Handler Path, Firmware Library TIMER API Reference

### Community 39 - "Lang Format C Style"
Cohesion: 1.0
Nodes (1): .clang-format C Style

### Community 40 - "Adc Dac And Tl431 Reference"
Cohesion: 1.0
Nodes (1): ADC DAC and TL431 Reference Network

## Knowledge Gaps
- **48 isolated node(s):** `docs Hardware Manuals and Contest Documents`, `build Generated Artifacts`, `VS Code EIDE Build Workflow`, `.clang-format C Style`, `rg Verification Workflow` (+43 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Timer6 Irq To Bsp Timer`** (2 nodes): `TIMER6 IRQ to BSP Timer Handler Path`, `Firmware Library TIMER API Reference`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Lang Format C Style`** (1 nodes): `.clang-format C Style`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Adc Dac And Tl431 Reference`** (1 nodes): `ADC DAC and TL431 Reference Network`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Driver README Driver Layer` connect `Recommended Bsp Development Order` to `Gd32F470 Eide Firmware Project`?**
  _High betweenness centrality (0.002) - this node is a cross-community bridge._
- **Are the 21 inferred relationships involving `move_window()` (e.g. with `sync_window()` and `get_fat()`) actually correct?**
  _`move_window()` has 21 INFERRED edges - model-reasoned connections that need verification._
- **Are the 21 inferred relationships involving `find_volume()` (e.g. with `get_ldnumber()` and `lock_fs()`) actually correct?**
  _`find_volume()` has 21 INFERRED edges - model-reasoned connections that need verification._
- **Are the 17 inferred relationships involving `dir_register()` (e.g. with `fill_first_frag()` and `fill_last_frag()`) actually correct?**
  _`dir_register()` has 17 INFERRED edges - model-reasoned connections that need verification._
- **Are the 16 inferred relationships involving `fmc_ready_wait()` (e.g. with `fmc_page_erase()` and `fmc_sector_erase()`) actually correct?**
  _`fmc_ready_wait()` has 16 INFERRED edges - model-reasoned connections that need verification._
- **Are the 15 inferred relationships involving `ld_word()` (e.g. with `get_fat()` and `ld_clust()`) actually correct?**
  _`ld_word()` has 15 INFERRED edges - model-reasoned connections that need verification._
- **What connects `docs Hardware Manuals and Contest Documents`, `build Generated Artifacts`, `VS Code EIDE Build Workflow` to the rest of the system?**
  _48 weakly-connected nodes found - possible documentation gaps or missing edges._