# vwcanread
Implementation of VW TP2.0 to handle KWP2000 requests for the MED9.1 ECU (Passat B6)

## Resources

### CAN-BUS
- [Data Exchange On The CAN Bus I](http://www.volkspage.net/technik/ssp/ssp/SSP_238.pdf)
- [Data transfer on CAN data bus II](http://www.volkspage.net/technik/ssp/ssp/SSP_269_d1.pdf)
- [MCP2515 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/MCP2515-Stand-Alone-CAN-Controller-with-SPI-20001801J.pdf)
- [MCP2551 Datasheet](http://ww1.microchip.com/downloads/en/devicedoc/21667e.pdf)

### VW TP2.0
- [CAN-based Higher Layer Protocols](https://www.sti-innsbruck.at/sites/default/files/courses/fileadmin/documents/vn-ws0809/03-vn-CAN-HLP.pdf)
- [BOSCH CAN Specification 2.0](http://esd.cs.ucr.edu/webres/can20.pdf)
- [SAE J2819 2008-02](https://doi.org/10.4271/J2819_200802) (paywall)
([archived, login required](http://nefariousmotorsports.com/forum/index.php?action=dlattach;topic=9182.0;attach=17302))
- [SAE J2819 2019-05 (new version)](https://doi.org/10.4271/J2819_201905) (paywall)
- [VW TP2.0 Overview](https://jazdw.net/tp20)
- [TP20, duplicated CAN frames/wrong sequence number?](http://nefariousmotorsports.com/forum/index.php?topic=11588.0) (forum)

### KWP2000
- [372139A-01](https://www.ni.com/pdf/manuals/372139a.pdf)
- [ISO 14230-3](https://www.iso.org/standard/23921.html) (paywall)
([archived](https://web.archive.org/web/20170809120152/http://read.pudn.com/downloads118/ebook/500929/14230-3.pdf))
- [DaimlerChrysler KWP2000 Release 2.2](http://nefariousmotorsports.com/forum/index.php?action=dlattach;topic=4983.0;attach=8149) (login required)
([scribd link](https://www.scribd.com/document/430362928/KWP2000-release2-2))
- [Specification for Programing Control Units on VAG Cars with KWP2000](http://nefariousmotorsports.com/forum/index.php?action=dlattach;topic=4983.0;attach=7703) (login required)
- [SSF 14230-3](http://nefariousmotorsports.com/forum/index.php?action=dlattach;topic=4983.0;attach=8150) (login required)
- [VAG Seed - Key Algorithm Challenge Response via CAN bus](http://nefariousmotorsports.com/forum/index.php?topic=4983.0) (forum)

### ECU
- [MED9.1 Funktionsrahmen](https://drive.google.com/file/d/1A2zKoa_cqnRuVfffiLVcYrXjKeulfdDe/view) (German)
- [Bosch Motronic MED 9.1 OBD System Strategy](https://drive.google.com/file/d/1oLpylsJqpfpw2E_NKdgB-W5O0gEkua9d/view)
- [Specification for Programing Control Units on VAG Cars with KWP2000](http://nefariousmotorsports.com/forum/index.php?action=dlattach;topic=4983.0;attach=7703) (login required)
- [EEPROM Layout](http://nefariousmotorsports.com/forum/index.php?action=dlattach;topic=6159.0;attach=9594) (login required) (German)
- [MPC500 MEMORY MAP](http://nefariousmotorsports.com/forum/index.php?action=dlattach;topic=6159.0;attach=9591) (login required)
- [MED9.1 Reading and Writing to the Serial EEPROM and RAM Mirror](http://nefariousmotorsports.com/forum/index.php?topic=6159.0) (forum)

## Related Projects
- https://github.com/seishuku/teensycanbusdisplay
- https://github.com/xerootg/esp32_tp20_datalogger

## License

GNU GPLv3

```
Copyright (C) 2019 Layton Nelson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
```
