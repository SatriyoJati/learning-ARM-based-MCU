## Creating baremetal firmware for stm32f103xb

### Creating linkerscript 

This is the file used by linker to link and decide which sections included to output file. 

First open `stm32f103.ld`

`MEMORY` used to describe location and size of blocks of memory in the target. we use stm32f103xb6 board which has 

- 128K Flash, located on 0x08000000
- 20k SRAM, located on 0x20000000

we then define symbol `_estack` which will be used in the reset function that define Stack Pointer location.

we then define `SECTIONS` to map input sections to output sections 

we define `.vectors` then `.text` then `.rodata` then .data in flash. For .data, its LMA is on flash but when running its location will be on SRAM.

### Creating startup code



### Configuring GPIO Peripherals

### End