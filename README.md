## Learning Fundamentals Embedded System using Arm Based Microcontroller (STM32)

This repo is my summary of learning Book Embedded Systems
Fundamentals with Arm® Cortex®-M based Microcontrollers: A Practical Approach by Alexander G.Dean . Eventhough the book uses the development board Nucleo-f109, I modified the examples and instructions using cheap clone STM32F103 MCU board (bluepill) .

In Addition, I also use and create manually the *linker script* and startup code and I explain it in another repo :


I used CMSIS from STM32 Downloadable source code (STM32CubeF1) and added it to this repo. 

For debugging and developing I use vscode text editor with the help of extension : 
- Cortex Debug
For compiling I use arm tool chain and the build tool is choose is CMake.



SO, Prerequisite to run this code :
a. make, you have installed make on your pc
b. CMake
c. Vscode , installing Cortex Debug Extension
d. Download STM32CubeCLT (SInce we use arm-toolchain and device description from there)
e. Download STM32CubeF1 (bundled software component consisting HAL, CMSIS, RTOS etc.)


Reference for vscode setup for embedded development (arm based) : 
a.  https://mcuoneclipse.com/2021/05/01/visual-studio-code-for-c-c-with-arm-cortex-m-part-1/ 


## Using GPIO

`project1AccessingGPIO` directory contain the firmware to configuring GPIO as output to turn on and off led , and as input. Used to create manually with the help of CMSIS.


## ARM Assembly


## Basic Concurrency

### Using Timer


## Interupts anc Processor core

## C in Assembly Lang

## Analog Interfacing

## Timers

## Serial Communications

## Direct Memory Access