# Contest Export and Firmware Check

This note covers two final-submission tasks without changing firmware C code:

1. Generate App and Bootloader EIDE project folders from the current dual-target project.
2. Check official V1/V2 firmware `.bin` files against the Bootloader USART receive rules.

## 1. Thin App/Bootloader Project Export

Use:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\export_contest_eide_projects.ps1 -OutputRoot export\contest_projects -Force
```

Output:

```text
export\contest_projects\App_Project
export\contest_projects\Bootloader_Project
```

Each folder contains:

- A one-target `.eide\eide.yml`.
- A small `firmware.code-workspace`.
- Small EIDE/VS Code metadata files.
- NTFS junctions to the shared source folders: `Driver`, `Function`, `Protocol`, `common`, `drivers`, `src`, `startup`, `tools`, and `doc`.
- `source_manifest.txt` for final materialization review.

The export intentionally does not copy large source/vendor directories. This keeps the repo clean and avoids accidental large generated submission packages.

Important: do not blindly zip these folders with a tool that follows junctions. If the final contest platform requires self-contained project folders, use `source_manifest.txt` as the reviewed source list and materialize only those files after confirming the final rule.

To produce manifest files only:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\export_contest_eide_projects.ps1 -OutputRoot export\contest_project_manifests -Mode ManifestOnly -Force
```

## 2. Official Firmware Check

The current Bootloader USART firmware receive logic expects:

| Field | Rule |
| --- | --- |
| Package magic | first 4 bytes must be `5A A5 C3 3C` (`0x5AA5C33C`) |
| App payload size | `1..0x20000` bytes |
| Stack pointer | `0x20000000..0x20030000` |
| Reset entry | Thumb/odd address |
| Reset entry range | aligned address in `0x08011000..0x08030FFF` |

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_official_firmware.ps1 `
  -Path "E:\XIMENZI\2026年CIMC工业嵌入式系统开发 初赛 赛题 (2)\固件文件\V1版本固件.bin" `
        "E:\XIMENZI\2026年CIMC工业嵌入式系统开发 初赛 赛题 (2)\固件文件\V2版本固件.bin"
```

Expected result for acceptable files: both rows show `PASS`.

If you want to inspect a raw App binary that does not include the official package magic, add `-AllowRawApp`. That mode is only diagnostic; the USART Bootloader path will reject raw App files unless they are packaged with the `5AA5C33C` prefix.
