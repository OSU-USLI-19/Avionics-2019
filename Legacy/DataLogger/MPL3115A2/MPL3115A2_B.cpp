/**************************************************************************/
/*!
@file		USLI_MPL3115A2.cpp
@original   K.Townsend (Adafruit Industries)
@update		K. O'Brien (OSU USLI)
@license	BSD (see license.txt)

Driver for the Adafruit MPL3115A2 barometric pressure sensor breakout
----> https://www.adafruit.com/products/1893
v1.0 - First release

TO DO:
-rewrite read8/write8 to work with global twi control
-rewrite getPressure to read data after DRDY INT received (data ready known)
-change getAlt to reset to baro mode after read
-set-up and test DRDY INT
-test with different prescale values
-set data read rate to desired value

*/
/**************************************************************************/


/**************************************************************************/
/*!
	@brief  Instantiates a new MPL3115A2 class
*/
/**************************************************************************/
MPL3115A2::MPL3115A2() {

}


/**************************************************************************/
/*!
	@brief  Setups the HW (reads coefficients values, etc.)
*/
/**************************************************************************/
boolean MPL3115A2::begin() {
	//start twi transmission
	Wire.begin();
	//UPDATE: init i2c globally and remove from sensor init

	//read from expected address, check return
	uint8_t whoami = read8(MPL3115A2_WHOAMI);
	if (whoami != 0xC4) {
		return false;
	}

	//UPDATE: ADD RESET BEFORE CONFIGURATION TO CLEAR FLAGS/DATA

	//ADDED 2/6 KO
	//turn on data ready interruput, route to INT1
	//write8(MPL3115A2_CTRL_REG4, MPL3115A2_CTRL_REG4_INT_EN_DRDY);
	//write8(MPL3115A2_CTRL_REG5, MPL3115A2_CTRL_REG5_INT_EN_DRDY);

	//ADDED 2/6 KO
	//configure INT1 for push-pull, active high
	//write8(MPL3115A2_CTRL_REG3, MPL3115A2_CTRL_REG3_IPOL1);

	//ADDED 2/6 KO
	//set data config. reg
	//generate data rdy flag; pressure on; temp off
	//write8(MPL3115A2_PT_DATA_CFG, (MPL3115A2_PT_DATA_CFG_PDEFE |
	//	MPL3115A2_PT_DATA_CFG_DREM));

	//ADDED 2/6 KO
	//set status reg1
	//active mode; 16 prescale (66ms read); baro mode
	//write8(MPL3115A2_CTRL_REG1, (MPL3115A2_CTRL_REG1_SBYB |
	//	MPL3115A2_CTRL_REG1_OS16 | MPL3115A2_CTRL_REG1_ALT));

	//set status reg1
	//active mode; 128 prescale; baro mode
	write8(MPL3115A2_CTRL_REG1, (MPL3115A2_CTRL_REG1_SBYB |
		MPL3115A2_CTRL_REG1_OS128 | MPL3115A2_CTRL_REG1_ALT));

	//set data config. reg
	//generate data rdy flag; pressure on; temp on
	write8(MPL3115A2_PT_DATA_CFG, (MPL3115A2_PT_DATA_CFG_TDEFE |
		MPL3115A2_PT_DATA_CFG_PDEFE | MPL3115A2_PT_DATA_CFG_DREM));


	//UPDATE: configure init for data ready mode
	return true;
}

/**************************************************************************/
//							readReadyPressure()
//	Gets the floating-point pressure level in kPa
//	called in DRDY ISR: new pressure data ready in PMSB when called
//	return pressure reading as a floating point value
//	2/8 KO
/**************************************************************************/
float MPL3115A2::readReadyPressure() {
	//20 bit pressure readings
	uint32_t pressure;

	//clear all interupts 

	//wait for standby (one-shot operation)
	//while (read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST) delay(10);
	//UPDATE: remove for INT operation - time off DRDY INT

	//set sensor for barometer mode
	//_ctrl_reg1.bit.ALT = 0;
	//write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);
	//UPDATE: remove for baro only operation - should already be in baro

	//initiate measurement (one-shot operation)
	//_ctrl_reg1.bit.OST = 1;
	//write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);
	//UPDATE: remove for INT operation - reads occur on ST timing

	//wait for pressure data ready (one-shot operation)
	//leave in to check for good data and clear flags
	uint8_t sta = 0;
	 do{
 		sta = read8(MPL3115A2_REGISTER_STATUS);
		//delay(10);	fuck it, constant read
	 } while (!(sta & MPL3115A2_REGISTER_STATUS_PDR));

	//initiate data read, ping PMSB
	_i2c->beginTransmission(MPL3115A2_ADDRESS);		//start transmission to device 
	_i2c->write(MPL3115A2_REGISTER_PRESSURE_MSB);	//request from pressure MSB
	_i2c->endTransmission(false);					//end transmission

	//request 3 bytes of pressure data
	_i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)3);//send data n-bytes read
	pressure = _i2c->read();	//receive DATA (MSB)
	pressure <<= 8;				//shift on
	pressure |= _i2c->read();	//receive DATA (CSB)
	pressure <<= 8;				//shift on
	pressure |= _i2c->read();	//receive DATA (LSB-upper nibble)
	pressure >>= 4;				//shift out lower nibble
	//UPDATE: put shifts and reads in same command

	float baro = pressure;
	baro /= 4.0;				//scale down fraction bits

	//TODO: reset DRDY flags

	return baro;
}

/**************************************************************************/
/*!
    @brief  Gets the floating-point pressure level in kPa
    @return altitude reading as a floating point value
	UPDATE: READ PRESSURE FROM DATA REGISTERS AFTER DRDY INT
			REMOVE PING AND WAIT
*/
/**************************************************************************/
float MPL3115A2::getPressure() {
  //20 bit pressure readings
  uint32_t pressure;

  //wait for standby (one-shot operation)
  while(read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST) delay(10);
  //UPDATE: remove for INT operation - time off DRDY INT

  //set sensor for barometer mode
  _ctrl_reg1.bit.ALT = 0;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);
  //UPDATE: remove for baro only operation - should already be in baro

  //initiate measurement (one-shot operation)
  _ctrl_reg1.bit.OST = 1;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);
  //UPDATE: remove for INT operation - reads occur on ST timing

  //wait for pressure data ready (one-shot operation)
  uint8_t sta = 0;
  while (! (sta & MPL3115A2_REGISTER_STATUS_PDR)) {
    sta = read8(MPL3115A2_REGISTER_STATUS);
    delay(10);
  }
  //UPDATE: change to do-while if used, shouldn't be needed for INT op

  //initiate data read, ping PMSB
  _i2c->beginTransmission(MPL3115A2_ADDRESS);	//start transmission to device 
  _i2c->write(MPL3115A2_REGISTER_PRESSURE_MSB); //request from pressure MSB
  _i2c->endTransmission(false);					//end transmission
  
  //request 3 bytes of pressure data
  _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)3);//send data n-bytes read
  pressure = _i2c->read();	//receive DATA (MSB)
  pressure <<= 8;			//shift on
  pressure |= _i2c->read();	//receive DATA (CSB)
  pressure <<= 8;			//shift on
  pressure |= _i2c->read(); //receive DATA (LSB-upper nibble)
  pressure >>= 4;			//shift out lower nibble
  //UPDATE: put shifts and reads in same command

  float baro = pressure;
  baro /= 4.0;				//scale down fraction bits
  return baro;
}

/**************************************************************************/
/*!
    @brief  Gets the floating-point altitude value
    @return altitude reading as a floating-point value
	UPDATE: might be used, use one-shot operation, revert back to baro mode after
*/
/**************************************************************************/
float MPL3115A2::getAltitude() {
  //20 bit altitdue data
  int32_t alt;
  
  //wait for standby (one-shot operation)
  while(read8(MPL3115A2_CTRL_REG1) & MPL3115A2_CTRL_REG1_OST) delay(10);

  //set sensor for altitude mode
  _ctrl_reg1.bit.ALT = 1;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);
  //UPDATE: set back to baro mode after read

  //initiate measurement (one-shot opeartion)
  _ctrl_reg1.bit.OST = 1;
  write8(MPL3115A2_CTRL_REG1, _ctrl_reg1.reg);

  //wait for pressure data ready (one-shot opearation)
  uint8_t sta = 0;
  while (! (sta & MPL3115A2_REGISTER_STATUS_PDR)) {
    sta = read8(MPL3115A2_REGISTER_STATUS);
    delay(10);
  }
  //update: change to do-while

  //initate data read, ping PMSB
  _i2c->beginTransmission(MPL3115A2_ADDRESS);		//start transmission to device 
  _i2c->write(MPL3115A2_REGISTER_PRESSURE_MSB);		//request from MSB
  _i2c->endTransmission(false);						//end transmission
  

  _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)3);//send data n-bytes read
  alt  = ((uint32_t)_i2c->read()) << 24;	//receive DATA (MSB) and shift on
  alt |= ((uint32_t)_i2c->read()) << 16;	//receive DATA (CSB) and shift on
  alt |= ((uint32_t)_i2c->read()) << 8;		//receive DATA (LSB) and shift on

  float altitude = alt;
  altitude /= 65536.0;	//remove scaling
  return altitude;
}

/**************************************************************************/
/*!
    @brief  Set the local sea level barometric pressure
    @param pascal the pressure to use as the baseline
*/
/**************************************************************************/
void MPL3115A2::setSeaPressure(float pascal) {
  uint16_t bar = pascal/2;
  _i2c->beginTransmission(MPL3115A2_ADDRESS);
  _i2c->write((uint8_t)MPL3115A2_BAR_IN_MSB);
  _i2c->write((uint8_t)(bar>>8));
  _i2c->write((uint8_t)bar);
  _i2c->endTransmission(false);
}

/**************************************************************************/
/*!
    @brief  Gets the floating-point temperature in Centigrade
    @return temperature reading in Centigrade as a floating-point value
	UPDATE: Don't think this will be used
*/
/**************************************************************************/
float MPL3115A2::getTemperature() {
  int16_t t;

  uint8_t sta = 0;
  while (! (sta & MPL3115A2_REGISTER_STATUS_TDR)) {
    sta = read8(MPL3115A2_REGISTER_STATUS);
    delay(10);
  }
  _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
  _i2c->write(MPL3115A2_REGISTER_TEMP_MSB); 
  _i2c->endTransmission(false); // end transmission
  
  _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)2);// send data n-bytes read
  t = _i2c->read(); // receive DATA
  t <<= 8;
  t |= _i2c->read(); // receive DATA
  t >>= 4;
  
  if (t & 0x800) {
    t |= 0xF000;
  }

  float temp = t;
  temp /= 16.0;
  return temp;
}

/**************************************************************************/
/*!
    @brief  read 1 byte of data at the specified address
    @param a the address to read
    @return the read data byte
	//UPDATE: rewrite using global twi commands
*/
/**************************************************************************/
uint8_t MPL3115A2::read8(uint8_t a) {
  _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
  _i2c->write(a); // sends register address to read from
  _i2c->endTransmission(false); // end transmission
  
  _i2c->requestFrom((uint8_t)MPL3115A2_ADDRESS, (uint8_t)1);// send data n-bytes read
  return _i2c->read(); // receive DATA
}

/**************************************************************************/
/*!
    @brief  write a byte of data to the specified address
    @param a the address to write to
    @param d the byte to write
*/
/**************************************************************************/
void MPL3115A2::write8(uint8_t a, uint8_t d) {
  _i2c->beginTransmission(MPL3115A2_ADDRESS); // start transmission to device 
  _i2c->write(a); // sends register address to write to
  _i2c->write(d); // sends register data
  _i2c->endTransmission(false); // end transmission
}
