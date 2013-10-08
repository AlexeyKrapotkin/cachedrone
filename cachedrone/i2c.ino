
static uint32_t neutralizeTime = 0;

void i2c_init(void) {
    PORTC |= 1<<4; PORTC |= 1<<5;
    TWSR = 0;                                   
    TWBR = ((F_CPU / I2C_SPEED) - 16) / 2;   
    TWCR = 1<<TWEN;                              
}

void i2c_stop(void) {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

void i2c_rep_start(uint8_t address) {
  TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN) ; 
  waitTransmissionI2C();                       
  TWDR = address;                              
  TWCR = (1<<TWINT) | (1<<TWEN);
  waitTransmissionI2C();                       
}

void i2c_write(uint8_t data ) {
  TWDR = data;
  TWCR = (1<<TWINT) | (1<<TWEN);
  waitTransmissionI2C();
}

uint8_t i2c_read(uint8_t ack) {
  TWCR = (1<<TWINT) | (1<<TWEN) | (ack? (1<<TWEA) : 0);
  waitTransmissionI2C();
  uint8_t r = TWDR;
  if (!ack) i2c_stop();
  return r;
}

uint8_t i2c_readAck() {
  return i2c_read(1);
}

uint8_t i2c_readNak(void) {
  return i2c_read(0);
}

void waitTransmissionI2C() {
  uint16_t count = 255;
  while (!(TWCR & (1<<TWINT))) {
    count--;
    if (count==0) {  
      TWCR = 0;                  
      neutralizeTime = micros(); 
      i2c_errors_count++;
      break;
    }
  }
}

size_t i2c_read_to_buf(uint8_t add, void *buf, size_t size) {
  i2c_rep_start((add<<1) | 1);  // I2C read direction
  size_t bytes_read = 0;
  uint8_t *b = (uint8_t*)buf;
  while (size--) {
    /* acknowledge all but the final byte */
    *b++ = i2c_read(size > 0);
    /* TODO catch I2C errors here and abort */
    bytes_read++;
  }
  return bytes_read;
}

size_t i2c_read_reg_to_buf(uint8_t add, uint8_t reg, void *buf, size_t size) {
  i2c_rep_start(add<<1); // I2C write direction
  i2c_write(reg);        // register selection
  return i2c_read_to_buf(add, buf, size);
}

/* transform a series of bytes from big endian to little
   endian and vice versa. */
void swap_endianness(void *buf, size_t size) {
  /* we swap in-place, so we only have to
  * place _one_ element on a temporary tray
  */
  uint8_t tray;
  uint8_t *from;
  uint8_t *to;
  /* keep swapping until the pointers have assed each other */
  for (from = (uint8_t*)buf, to = &from[size-1]; from < to; from++, to--) {
    tray = *from;
    *from = *to;
    *to = tray;
  }
}

void i2c_getSixRawADC(uint8_t add, uint8_t reg) {
  i2c_read_reg_to_buf(add, reg, &rawADC, 6);
}

void i2c_writeReg(uint8_t add, uint8_t reg, uint8_t val) {
  i2c_rep_start(add<<1); // I2C write direction
  i2c_write(reg);        // register selection
  i2c_write(val);        // value to write in register
  i2c_stop();
}

uint8_t i2c_readReg(uint8_t add, uint8_t reg) {
  uint8_t val;
  i2c_read_reg_to_buf(add, reg, &val, 1);
  return val;
}


