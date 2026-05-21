#include <stdint.h>
#include <stm32f101xb.h>
#include "clk.h"
#include "spi.h"
#include "sdcard.h"
#include "sdcard_spi.h"
#include "queue.h"
#include "led.h"
#include "uart.h"
#include "i2c.h"
#include <string.h>
#include "vl53l1x/core/VL53L1X_api.h"
#include "timer.h"
#include "vl53l0x_api.h"

volatile uint32_t mticks = 0;

void SysTick_Handler(void) {
    mticks++; // Increment every 1ms
}

void Delay(uint32_t ms) {
    uint32_t start =   mticks;
    while((mticks - start) < ms);
}


uint8_t checkArray(int rxArr[], int txArr[], int size) {
    for(int i=0; i<3;i++){
        if(rxArr[i] != txArr[i]) {
            return 0;
        }
    }
    return 1;
}

void HardFault_Handler() {
    uint32_t active_irq  = SCB->ICSR & 0x1FF;  // bits[8:0] = active exception number
    // Subtract 16 to get the peripheral IRQ number
    // e.g. active_irq=22 → IRQ #6 → TIM1_UP_IRQn on STM32F103

    uint32_t ipending0 = NVIC->ISPR[0]; // pending IRQs  0-31
    uint32_t ipending1 = NVIC->ISPR[1]; // pending IRQs 32-63
    uint32_t iactive0  = NVIC->IABR[0]; // active IRQs   0-31
    uint32_t iactive1  = NVIC->IABR[1]; // active IRQs  32-63
}
VL53L0X_Dev_t       vl53l0x_dev;
VL53L0X_DEV         Dev = &vl53l0x_dev;

void VL53L0X_Init(Uart* uart) {
    VL53L0X_Error status = 0;
    char buf[64];
    // Set I2C address (default 0x52)
    Dev->I2cDevAddr = 0x29;

    // Device init
    status = VL53L0X_DataInit(Dev);
    sprintf(buf, "Status Data Init = %d\r\n", status);
    uart_transmit_string(uart,buf);

    status = VL53L0X_StaticInit(Dev);
    sprintf(buf, "Status StaticInit = %d\r\n", status);
    uart_transmit_string(uart,buf);
    uint32_t refSpadCount = 0;
    uint8_t isApertureSpads = 0;
    status = VL53L0X_PerformRefSpadManagement(Dev, &refSpadCount, &isApertureSpads);
    if (status == VL53L0X_ERROR_NONE) {
    // Successfully updated5
        uart_transmit_string(uart,"Sucess spad,\n\r");
    }
    // One-time calibration (or load saved values)
    uint8_t VhvSettings = 0, PhaseCal = 0;
    status = VL53L0X_PerformRefCalibration(Dev, &VhvSettings, &PhaseCal);
    if (status != VL53L0X_ERROR_NONE) {
        printf("Calibration API critical error code: %d\n", status);
    }

    status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(Dev, 33000); // Set higher, e.g., 50ms (50000)

    status = VL53L0X_SetInterMeasurementPeriodMilliSeconds(Dev, 50); 
    // Set continuous mode
    status = VL53L0X_SetDeviceMode(Dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);


    // Start measurement
    status = VL53L0X_StartMeasurement(Dev);
}

uint16_t VL53L0X_ReadDistance_mm(VL53L0X_RangingMeasurementData_t *measurement) {
    VL53L0X_Error status;
    uint8_t dataReady = 0;
    
    while (dataReady == 0) {
        status = VL53L0X_GetMeasurementDataReady(Dev, &dataReady);
        Delay(2);
    }

    // Get measurement
    status = VL53L0X_GetRangingMeasurementData(Dev, measurement);

    status = VL53L0X_ClearInterruptMask(Dev, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);

    if (status == VL53L0X_ERROR_NONE && measurement->RangeStatus == 0) {
        return measurement->RangeMilliMeter;
    }

    return 0xFFFF; // error
}

#define DEV_ADDR 0x29

int main() {
    CLK_Init();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock/1000);
    TIM2_Init();
    Init_SPI1(); 
    Uart* uartInstance = NULL;
    uartInstance =  UartCreate();
    
    if (uartInstance != NULL){
        init_uart(uartInstance, (uint32_t)115200);
        uart_transmit_string(uartInstance,(uint8_t*)"UART RESET");
        uart_transmit(uartInstance, '\n');
        uart_transmit(uartInstance, '\r');
    }
    Led* led = LedCreate();
    if(led != NULL)
        init_led_gpio(led,PORTC,OUTPUT_PUSHPULL);
    uint8_t initStatus = 0;
    uint32_t timeout = 40;
    Sdcard sdcard1;
    do {
        initStatus = init_sdcard(&sdcard1);
        if (initStatus == 0x01) break;
        // Delay(10);
    } while (--timeout);

    uint8_t buff[512] = {};
    memset(buff, 0x0, sizeof(buff));

    Q_T sdcardQueue;
    Q_Init(&sdcardQueue);

    // dummy data to write
    char dummy[] = "hello, world";
    char step[] = "indian speed";
    Data temp;
    uint8_t read_res;
    uint8_t count_error=0;

    I2C_Init(1);
    uint8_t buf[2] = {0x01, 0x0F};
    uint8_t booted = 0;

    uint8_t byte = 0;

    VL53L0X_Init(&uartInstance);
    VL53L0X_RdByte(Dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &byte);
    uart_print_hex(uartInstance, "data", byte);

    uint8_t ping = 0;

    uint8_t  dataReady = 0;
    uint8_t  rangeStatus;
    uint16_t distance_mm;
    VL53L0X_RangingMeasurementData_t measurement;

    while(1){
        char uart_buf[30]; 
        if(initStatus == 0x01) {
            uint16_t distance_mm = VL53L0X_ReadDistance_mm(&measurement);
            int len = snprintf(uart_buf, sizeof(uart_buf), "Distance: %u mm\r\n", distance_mm);
            uart_transmit_string(uartInstance, &uart_buf);
            snprintf(uart_buf, sizeof(uart_buf), "Range Status: %u \r\n", measurement.RangeStatus);
            uart_transmit_string(uartInstance, &uart_buf);

            uint8_t int_status;
            VL53L0X_RdByte(Dev, VL53L0X_REG_RESULT_INTERRUPT_STATUS, &int_status);
            uart_print_hex(uartInstance, "IntStatus after clear: ", int_status);
            uint16_t sensorId = 0;
            VL53L1X_GetSensorId(DEV_ADDR, &sensorId);


            read_res = read_single_block(0x00, buff);
            if(read_res != 0) {
                uart_transmit_string(uartInstance, (uint8_t *)"Error Read\n\r");
                memset(buff,0,40);
                count_error++;
                if (count_error > 6){
                    initStatus = init_sdcard(&sdcard1);
                    count_error = 0;
                }
            }
            if (buff[0] != 0)
                Q_Enqueue(&sdcardQueue, buff, 40);
            
            if(Q_Dequeue(&sdcardQueue, &temp)){
                for(int i = 0 ; i < 50 ; i++){
                    uart_transmit(uartInstance, temp.data[i]);
                }
            }
            uart_transmit(uartInstance, '\n');
            uart_transmit(uartInstance, '\r');
            if(write_single_block(1024, (uint16_t*)dummy)){
                uart_transmit_string(uartInstance, (uint8_t *) "Error Write\n\r");
                count_error++;
                if (count_error > 6){
                    initStatus = init_sdcard(&sdcard1);
                    count_error = 0;
                }
            }
            blink_fast(led);
            read_res = read_single_block(1024, buff); 
            if (read_res != 0) {
                uart_transmit_string(uartInstance, (uint8_t *) "Error Read\n\r");
                memset(buff,0,40);
                count_error++;
                if (count_error > 6){
                    initStatus = init_sdcard(&sdcard1);
                    count_error = 0;
                }
            }
            if (buff[0] != 0)
                Q_Enqueue(&sdcardQueue, buff, 40);
            
            if(Q_Dequeue(&sdcardQueue, &temp)) {
                for(int i = 0 ; i < 50 ; i++){
                    uart_transmit(uartInstance, temp.data[i]);
                }
            }

            uart_transmit(uartInstance, '\n');
            uart_transmit(uartInstance, '\r');
            if(write_single_block(0x00, (uint16_t*)step)) {
                uart_transmit_string(uartInstance, (uint8_t *) "Error Write\n\r");
                count_error++;
                if (count_error > 6){
                    initStatus = init_sdcard(&sdcard1);
                    count_error = 0;
                }
            }
            blink_fast(led);
        } else {
            blink_slow(led);
            VL53L1X_StopRanging(DEV_ADDR);
            NVIC_SystemReset();
        }

    }

    return 0;
}