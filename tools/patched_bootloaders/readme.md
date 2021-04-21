This folder contains bootloaders which have been patched so that they do not check the Unique ID of the MK22 MCU and compare it with the signature bytes in the MCU ROM

This bootloader is ONLY intended to be used by developers and anyone who need to reflash their MCU ROM because the signature bytes are corrupted or erased.


On the GD-77 the signature bytes are at 0x7F800 and if these bytes become erased or corrupted the bootloader will not allow updates to the firmware.
In this case the green LED on the top of the radio flashes, rather than remaining permanently on, and as soon as buttons SK1 and SK2 are released the bootloader runs the main firmware.

On the Baofeng DM-1801 the signature bytes are in a completely different location 0x3800 and are in the top of the bootloader ROM section.

The code for both the DM-1801 and GD-77 bootloaders appears to be almost identical, and the patch to no longer check the signature bytes is identical, changing a BEQ to a B assembler instruction

Just to complicate things. There seems to be more than one type of DM-1801 bootloader.
My DM-860 bootloader worked but did not light the green LED at all, so I have installed the bootloader sent to me by F1RBM, which was recovered from his DM-1801
My green LED now illumates correctly.

I don't know why the bootloader in my DM-860 did not illumate the LED, and I don't think there is any point adding it to this repo as the DM-1801 bootloader I uploaded here works fine.
