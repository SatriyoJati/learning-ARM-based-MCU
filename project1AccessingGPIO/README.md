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

startup code is function that is called first time. Since we declare `ENTRY_POINT(_reset)` in linker script , the function _reset will be first function to be called. 

if we look into main.c there is the definition of `_reset` function , it has gcc attribute , naked means the function won't generate prolouge and epilouge used to save and store registers to stack. Then `noreturn` , will make the function doesnt return.
Inside noreturn function like that we should properly provide loop which I add inside the `main()`. 

first we declare the variables here, with extern because it has declared somewhere (in our case the symbol already declared in linker script).

then we do two for loops :
1. first loop to initialize bss to zeros.
2. second loop is to move data from memory located in `_sidata` to memory located in `_sdata`, it increment until it reach `_edata`.


### Configuring GPIO Peripherals

In order to configure gpio peripheral first you need to enable the clock.

this case APB2ENR register has configuration to enable gpio port x.

in this example I use port C , so to enable it :

`RCC->APB2ENR |= RCC_APB2ENR_IOPCEN`

Then, enable PC13 by set it as open drain with output 50 MHz. By enabling GPIOC CRH Register.

`GPIOC->CRH |= (GPIO_CRH_MODE13 | GPIO_CRH_CNF13_0);`

Then to turn on and off the pin you can set BSRR Register, 

`GPIOC->BSRR &= ~(GPIO_BSRR_BR13);`

`GPIOC->BSRR |= GPIO_BSRR_BS13;`

Then you can separate each steps into functions. 
### End