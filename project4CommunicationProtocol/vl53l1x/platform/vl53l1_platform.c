/**
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "vl53l1_platform.h"
#include <string.h>
#include <time.h>
#include <math.h>
#include "i2c.h"
#include "timer.h"


int8_t VL53L1_WriteMulti( uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
	uint8_t status = 0;
	
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */

	
	for(int i=0; i < count ; i++) {
		VL53L1_WrByte(dev,index + i, pdata[i]);
		if (status != 0) break;  // stop early on error

	}
	return status;
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count){
	uint8_t status = 255;
	
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	for(int i=0; i < count ; i++) {
		VL53L1_RdByte(dev,index + i, &pdata[i]);
	}
	return status;
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
	int status = 0;
	uint8_t buf[3] = {
		(uint8_t) (index >> 8),
		(uint8_t) (index & 0xFF),
		data
	};	
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	status = I2C_Write(dev,&buf,3);
	return status;
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
	int status = 0;
	uint8_t buf[4] = {
		(uint8_t) (index >> 8) ,
		(uint8_t) (index & 0xFF),
		(uint8_t) (data >> 8),
		(uint8_t) (data & 0xFF)
	};

	status = I2C_Write(dev, &buf, 4);
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	
	return status;
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
	uint8_t status = 0;
	
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	
	return status;
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
	int status = 0;
	// I2C_Write_Address(dev,0);
	// I2C_Stop();
	uint8_t buf[3] = {
		(uint8_t) (index >> 8),
		(uint8_t) (index & 0xFF)
	};	
	// I2C_Write(dev, &buf, 2);
	
	// I2C_Stop();
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	status = I2C_Read(dev,index, data,1);
	return status;
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
	int status = 0;
	
	uint8_t buf[2] = {
		(uint8_t) (index >> 8),
		(uint8_t) (index & 0xFF)
	};
	// I2C_Write((uint8_t)dev, buf, 2);

	uint8_t dataBuf[2];
	status = I2C_Read((uint8_t)dev, index, dataBuf,2);
	*data = (uint16_t) dataBuf[0] << 8 |  dataBuf[1];
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	
	return status;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
	int status = 0;
	
	uint8_t buf[2] = {
		(uint8_t) (index >> 8),
		(uint8_t) (index & 0xFF)
	};
	// I2C_Write(dev, buf, 2);
	
	uint8_t dataBuf[4];
	status = I2C_Read(dev, index, dataBuf,4);
	*data = ((uint32_t)dataBuf[0] << 24)
		| ((uint32_t)dataBuf[1] << 16)
		| ((uint32_t)dataBuf[2] << 8)
		|  (uint32_t)dataBuf[3];
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	
	return status;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms){
	uint8_t status = 0;

	status = DelayUs(wait_ms * 1000);
	
	if (status)
		return status;
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	
	return status;
}
