avrdude.exe -pm328p -cavrispmkII -Pusb -u -Uflash:w:Bootloader_Fuocostep.hex:a -Ulfuse:w:0xe0:m -Uhfuse:w:0xd6:m -Uefuse:w:0xfd:m
pause