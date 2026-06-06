# Protocol Layer

This folder contains protocol helpers required by the CIMC contest structure.

- `protocol_ascii_hex.*`: binary byte stream and ASCII hex string conversion.
- `protocol_crc16.*`: CRC16-Modbus calculation.
- `protocol_frame.*`: base parser and builder for contest frames.

Contest frames are structured as binary fields first, then sent over UART as ASCII hex characters.
