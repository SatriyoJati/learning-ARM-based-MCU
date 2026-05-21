/*******************************************************************************
Copyright � 2015, STMicroelectronics International N.V.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of STMicroelectronics nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS ARE DISCLAIMED.
IN NO EVENT SHALL STMICROELECTRONICS INTERNATIONAL N.V. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************************/

/**
 * @file VL53L0X_i2c.c
 *
 * Copyright (C) 2014 ST MicroElectronics
 *
 * provide variable word size byte/Word/dword VL6180x register access via i2c
 *
 */
#include "vl53l0x_platform.h"
#include "vl53l0x_api.h"
#include "timer.h"
#include "stm32f103xb.h"


VL53L0X_Error VL53L0X_LockSequenceAccess(VL53L0X_DEV Dev){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;

    return Status;
}

VL53L0X_Error VL53L0X_UnlockSequenceAccess(VL53L0X_DEV Dev){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;

    return Status;
}

// the ranging_sensor_comms.dll will take care of the page selection
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count){
    uint8_t burst_buf[32];
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;

    if (count > 31) return VL53L0X_ERROR_CONTROL_INTERFACE;
    burst_buf[0] = index;

    for (uint32_t i = 0; i < count; i++) {
        burst_buf[i + 1] = pdata[i];
    }

    if (I2C_Write(Dev->I2cDevAddr, burst_buf, count + 1) != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

// the ranging_sensor_comms.dll will take care of the page selection
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    if (I2C_Read(Dev->I2cDevAddr, index, pdata, count) != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    return Status;
}


VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int status = 0;
	uint8_t buf[2] = {
		// (uint8_t) (index >> 8),
		// (uint8_t) (index & 0xFF),
        index,
		data
	};	
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	status = I2C_Write(Dev->I2cDevAddr,&buf,2);

    if (status !=0){
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;

    uint8_t buf[4] = {
		// (uint8_t) (index >> 8) ,
		// (uint8_t) (index & 0xFF),
        (uint8_t) index,
		(uint8_t) (data >> 8),
		(uint8_t) (data & 0xFF)
	};

	Status = I2C_Write(Dev->I2cDevAddr, &buf, 3);
    if (Status != 0){
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    return Status;
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t buf[5] = {
        index,
        (uint8_t)(data >> 24),
        (uint8_t)(data >> 16),
        (uint8_t)(data >> 8),
        (uint8_t)(data & 0xFF)
    };
    Status = I2C_Write(Dev->I2cDevAddr, buf, 5);
    if (Status != 0){
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    return Status;
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t data;

    Status = VL53L0X_RdByte(Dev, index, &data);
    if (Status == VL53L0X_ERROR_NONE) {
        data = (data & AndData) | OrData;
        Status = VL53L0X_WrByte(Dev, index, data);
    }
    if (Status != 0){
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    return Status;
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	// I2C_Write_Address(dev,0);
	// I2C_Stop();
	uint8_t buf[1] = {
		// (uint8_t) (index >> 8),
		// (uint8_t) (index & 0xFF)
        index
    };	
	// Status = I2C_Write_NoStop(Dev->I2cDevAddr, &buf, 1);
	// if (Status != 0){
    //     Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    // }
    // I2C_RepeatStart();
	// I2C_Stop();
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	Status = I2C_Read(Dev->I2cDevAddr, index, data,1);

    if (Status != 0){
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    return Status;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	
	uint8_t buf[1] = {
		// (uint8_t) (index >> 8),
		// (uint8_t) (index & 0xFF)
        index
	};
	// Status = I2C_Write((uint8_t)Dev->I2cDevAddr, buf, 1);
    // if (Status != 0){
    //     Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    // }

    // I2C_RepeatStart();
	uint8_t dataBuf[2];
	Status = I2C_Read((uint8_t)Dev->I2cDevAddr, index, dataBuf,2);
    if (Status != 0){
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    } else {
	    *data = (uint16_t) dataBuf[0] << 8 |  dataBuf[1];
    }
    return Status;
}

VL53L0X_Error  VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data){
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	
	uint8_t buf[1] = {
		// (uint8_t) (index >> 8),
		// (uint8_t) (index & 0xFF)
        index
	};
	// Status = I2C_Write(Dev->I2cDevAddr, buf, 1);
	// if (Status != 0){
    //     Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    // }
    // I2C_RepeatStart();
	uint8_t dataBuf[4];
	Status = I2C_Read(Dev->I2cDevAddr, index, dataBuf ,4);

    if (Status != 0){
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    } else {
        *data = ((uint32_t)dataBuf[0] << 24)
            | ((uint32_t)dataBuf[1] << 16)
            | ((uint32_t)dataBuf[2] << 8)
            |  (uint32_t)dataBuf[3];
    }


    return Status;
}

VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev){
    VL53L0X_Error status = VL53L0X_ERROR_NONE;

	if (DelayUs(2000) != 0) {
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
	
    return status;
}

