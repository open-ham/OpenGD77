The Baofeng DM-5R and RD-5R have a FM broadcast receiver chip in them.

However the markings on the chip are not of the RDA5802, and the chip does not appear to be a RDA5802
Hence this datasheet has only been added as a general reference to broadcast FM receiver chips.

The chip in that is in the radio is connected to the same I2C bus as the AT1846 and the EEPROM, and is on the same address as 
the RDA5802, but the chip does not resond with the correct ID code.

I think it returns ID code 0x02 when it should return 0x58 

Also reading some other registers in the chip, do not return the default values that a RDA5802 would return.
