// ****  i2c functions *******

/*
 * Sends an i2c command out to a follower when running in leader mode
 */
void sendi2c(uint8_t model, uint8_t deviceIndex, uint8_t cmd, uint8_t devicePort, int value){
      uint16_t valueTemp;
      uint8_t messageBuffer[4];

      valueTemp = (uint16_t)value;
      messageBuffer[2] = valueTemp >> 8;
      messageBuffer[3] = valueTemp & 0xff;

      Wire.beginTransmission(model + deviceIndex);
      messageBuffer[0] = cmd; 
      messageBuffer[1] = (uint8_t)devicePort;
      Wire.write(messageBuffer, 4);
      Wire.endTransmission();
}

// ***** i2c ******

//
// handle Rx Event (incoming I2C data)
//
void receivei2cEvent(size_t count)
{
    //Wire.read(i2c_databuf, Wire.available());  // copy Rx data to databuf
    //i2c_received = count;           // set received flag to count, this triggers print in main loop
    Serial.printf("i2c read: (%d)\n", count);
}

//
// handle Tx Event (outgoing I2C data)
//
void requesti2cEvent(void)
{
  //Wire.write(i2c_databuf, MEM_LEN); // fill Tx buffer (send full mem)
  //Serial.printf("i2c write (%d)\n", MEM_LEN);
  Serial.print("i2c Read request \n");
}

// i2c print scan status
//
void print_scan_status(uint8_t target, uint8_t all)
{
    switch(Wire.status())
    {
    case I2C_WAITING:  
        Serial.printf("Addr: 0x%02X ACK\n", target);
        found = 1;
        break;
    case I2C_ADDR_NAK: 
        if(all) Serial.printf("Addr: 0x%02X\n", target); 
        break;
    default:
        break;
    }
}

// i2c trigger after Tx complete (outgoing I2C data)
//
void i2cTransmitDone(void)
{
    Serial.print("OK\n");
}

// i2c trigger after Rx complete (incoming I2C data)
//
void i2cRequestDone(void)
{
    Wire.read(i2c_databuf, Wire.available());
    Serial.printf("'%s' OK\n",i2c_databuf);
}
