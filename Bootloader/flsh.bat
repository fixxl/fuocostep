@echo off
avrdude.exe -pm328p -cavrispmkII -Pusb -b 19200 -B 50 -u -Uflash:w:Bootloader_Fuocostep.hex:a -Ulfuse:w:0xe0:m -Uhfuse:w:0xd6:m -Uefuse:w:0xfd:m -v & if errorlevel 1 ECHO FLASCHVORGANG NICHT ERFOLGREICH! & sleep 5
pause