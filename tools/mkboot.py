#!/usr/bin/env python3
from pathlib import Path

msg=b"Hello from a guest running on UN_Cygnus!\r\n"
code=bytearray()
for ch in msg:
    code += bytes([0xB4,0x0E,0xB0,ch,0xCD,0x10])
code += b"\xF4"
sector=code+bytes(max(0,510-len(code)))+b"\x55\xAA"
Path('build').mkdir(exist_ok=True)
Path('build/hello.img').write_bytes(sector[:512])
