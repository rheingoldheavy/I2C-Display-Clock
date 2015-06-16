/*

  A sketch that displays the time on the I2C Display Add-on board, as read
  from an MCP7940N RTC.

  Hold down the clock button to allow the time set event to occur during powerup.

  Datasheet: http://ww1.microchip.com/downloads/en/DeviceDoc/20005010F.pdf

 */

#include <I2C.h>

// RTC I2C address and registers
const int MCP7940_I2C  = 0x6F;  // I2C Address for the RTC
const int REG_RTCSEC   = 0x00;  // Register Address: Time Second
const int REG_RTCMIN   = 0x01;  // Register Address: Time Minute
const int REG_RTCHOUR  = 0x02;  // Register Address: Time Hour
const int REG_RTCWKDAY = 0x03;  // Register Address: Date Day of Week
const int REG_RTCDATE  = 0x04;  // Register Address: Date Day
const int REG_RTCMTH   = 0x05;  // Register Address: Date Month
const int REG_RTCYEAR  = 0x06;  // Register Address: Date Year
const int REG_CONTROL  = 0x07;  // Register Address: RTC Feature Control
const int REG_OSCTRIM  = 0x08;  // Register Address: Oscillator Digital Trim

// Display Driver I2C address and registers
const int  AS1115_I2C              = 0x00; // Display Manager IC I2C Address
const int  REGISTER_SHUTDOWN_MODE  = 0x0C; // Register Address: Display Shutdown
const int  REGISTER_DECODE         = 0x09; // Register Address: Digit decode / no decode
const int  REGISTER_SCAN_LIMIT     = 0x0B; // Register Address: Digit enable
const int  REGISTER_INTENSITY      = 0x0A; // Register Address: Global brightness

const int  DIGIT_0                 = 0x01; // Left most seven segment digit
const int  DIGIT_1                 = 0x02; // Second seven segment digit
const int  DIGIT_2                 = 0x03; // Third seven segment digit
const int  DIGIT_3                 = 0x04; // Right most seven segment digit
const int  BTM_RED                 = 0x05; // Bottom Red bar graph multiplexed with L1/L2/L3
const int  BTM_GRN                 = 0x06; // Bottom Green bar graph
const int  TOP_RED                 = 0x07; // Top Red bar graph
const int  TOP_GRN                 = 0x08; // Top Green bar graph

int         clkButton               = 6;    // A variable representing the clock button pin
long        errorCount              = 0;    // A variable for holding how many errors occur
byte        errorStatus             = 0;    // A variable to hold the I2C command error code
byte        timeStamp[7];                   // Timestamp array position is the same as the register address.

void setup()
{

  pinMode      (clkButton,  INPUT);
  pinMode      (A0,         OUTPUT);   // Configure the Display multiplexer pin as output
  digitalWrite (A0,         LOW);      // Enable the L1/L2/L3, not bottom row red LED


  I2c.begin    ();             // Initialize the I2C library
  I2c.pullup   (0);            // Disable the internal pullup resistors
  I2c.setSpeed (0);            // Enable 100kHz I2C Bus Speed
  I2c.timeOut  (250);          // Set a 250ms timeout before the bus resets

  Serial.begin(9600);

  if (digitalRead(clkButton) == HIGH) setTime();  // Set the time if the CLOCK button is pressed during startup
  init_MCP7940();
  init_AS1115();

}

void loop()
{
  getTime();
  displayTime();

}

void getTime()
{

  I2c.read(MCP7940_I2C, REG_RTCSEC,  7);  // Gets all seven bytes of Clock and Date

  // Load Seconds, Minutes and Hours from buffer, mask unneeded bits, and convert from BCD
  timeStamp[0] = convertFromBcd(I2c.receive() & 0x7F);  
  timeStamp[1] = convertFromBcd(I2c.receive() & 0x7F);
  timeStamp[2] = convertFromBcd(I2c.receive() & 0x3F);

  // DEBUG CODE TO OUTPUT TIME DATA TO SERIAL WINDOW
  /*
  Serial.print    ("Current Time: ");
  if              (timeStamp[2] / 10 == 0) Serial.print ("0");
  Serial.print    (timeStamp[2]);
  Serial.print    (":");
  if              (timeStamp[1] / 10 == 0) Serial.print ("0");
  Serial.print    (timeStamp[1]);
  Serial.print    (":");
  if              (timeStamp[0] / 10 == 0) Serial.print ("0");
  Serial.println  (timeStamp[0]);
  Serial.println ();
  */
}

void displayTime()
{

  // Set digit 0 to the 10's place of the current hour
  errorStatus = I2c.write(AS1115_I2C, DIGIT_0, timeStamp[2] / 10);
  if (errorStatus != 0) errorHandler();

  // Set digit 1 to the 1's place of the current hour
  errorStatus = I2c.write(AS1115_I2C, DIGIT_1, timeStamp[2] % 10);
  if (errorStatus != 0) errorHandler();

  // Set digit 2 to the 10's place of the current minute
  errorStatus = I2c.write(AS1115_I2C, DIGIT_2, timeStamp[1] / 10);
  if (errorStatus != 0) errorHandler();

  // Set digit 3 to the 1's place of the current minute
  errorStatus = I2c.write(AS1115_I2C, DIGIT_3, timeStamp[1] % 10);
  if (errorStatus != 0) errorHandler();

  // Set bottom green to the binary visualization of the current seconds value
  errorStatus = I2c.write(AS1115_I2C, BTM_GRN, timeStamp[0]);
  if (errorStatus != 0) errorHandler();

}

void setTime()
{

  // These are the values that will be written to the MCP7940
  timeStamp[0]  = convertToBcd( 0);    // SECONDS
  timeStamp[1]  = convertToBcd(46);    // MINUTES
  timeStamp[2]  = convertToBcd( 8);    // HOURS
  timeStamp[3]  = convertToBcd( 1);    // DAY OF WEEK (arbitrary value 1 - 7)
  timeStamp[4]  = convertToBcd(15);    // DAY
  timeStamp[5]  = convertToBcd(06);    // MONTH
  timeStamp[6]  = convertToBcd(15);    // YEAR

  // Write our time stamp to the time/date registers
  errorStatus = I2c.write(MCP7940_I2C, REG_RTCSEC, timeStamp, 7);
  if (errorStatus != 0) errorHandler();

}

byte convertToBcd(byte byteDecimal) {
  
  // In Binary Coded Decimal (BCD), the 10's place is in the MSByte 
  // and the 1's place is in the LSByte
  return (byteDecimal / 10) << 4 | (byteDecimal % 10);

}

byte convertFromBcd(byte byteBCD) {

  // In Binary Coded Decimal (BCD), the 10's place is in the MSByte 
  // and the 1's place is in the LSByte
  byte byteMSB = 0;
  byte byteLSB = 0;

  byteMSB      = (byteBCD & B11110000) >> 4;
  byteLSB      = (byteBCD & B00001111);

  return       ((byteMSB * 10) + byteLSB);
}

void init_AS1115()
{
  // Enable Digits 0-7
  errorStatus = I2c.write(AS1115_I2C, REGISTER_SCAN_LIMIT,    0x07);
  if (errorStatus != 0) errorHandler();

  // Set digits 0-3 to decode as fonts
  errorStatus = I2c.write(AS1115_I2C, REGISTER_DECODE,        0x0F);
  if (errorStatus != 0) errorHandler();

  // Set the intensity level; 0x00 = LOW, 0x0F = HIGH
  errorStatus = I2c.write(AS1115_I2C, REGISTER_INTENSITY,     0x08);
  if (errorStatus != 0) errorHandler();

  // Turn on the time delimiter colon
  errorStatus = I2c.write(AS1115_I2C, BTM_RED, 0x60);
  if (errorStatus != 0) errorHandler();

  // Begin normal operation
  errorStatus = I2c.write(AS1115_I2C, REGISTER_SHUTDOWN_MODE, 0x01);
  if (errorStatus != 0) errorHandler();
}

void init_MCP7940() {

  int    trimVal       = 0x45;    // The amount of clock cycles to add/subtract.  See datasheet
  int    twelveHour    = 0x00;    // 0 = 24 Hour Clock Mode     / 1 = 12 Hour Clock Mode
  int    crsTrim       = 0x00;    // 0 = Disable Course Trim    / 1 = Enable Course Trim
  int    batEnable     = 0x01;    // 0 = Disable Battery Backup / 1 = Enable Battery Backup
  int    startClock    = 0x01;    // 0 = Start Oscillator       / 1 = Stop Oscillator

  I2c.write          (MCP7940_I2C,  REG_CONTROL, 0x00);       // Ensure the Control register starts as 0x00
  I2c.write          (MCP7940_I2C,  REG_OSCTRIM, trimVal);    // Clock Cycle Trim Value - see datasheet
  ConfigureRegister  (REG_RTCHOUR,  twelveHour,  6);          // 12 Hour Clock:      0 = NO / 1 = YES
  ConfigureRegister  (REG_CONTROL,  crsTrim,     2);          // Course Trim Enable: 0 = NO / 1 = YES
  ConfigureRegister  (REG_RTCWKDAY, batEnable,   3);          // Battery Enable:     0 = NO / 1 = YES
  ConfigureRegister  (REG_RTCSEC,   startClock,  7);          // Start Oscillator:   0 = NO / 1 = YES
}

void ConfigureRegister(int registerAddress, int value, int positions) {

  //  registerAddress: the single byte register that you will be writing to
  //  value:           a single bit of data, 1 or 0, that you want set or cleared in the register
  //  positions:       the location of that single bit of data within the register, 0 indexed

  int registerValue = 0x00;    // Holds each config value when read from the register
  I2c.read (MCP7940_I2C, registerAddress, 1);
  registerValue    = I2c.receive();
  if (value == 0x00) I2c.write (MCP7940_I2C, registerAddress, bitClear (registerValue, positions));
  if (value == 0x01) I2c.write (MCP7940_I2C, registerAddress, bitSet   (registerValue, positions));

}


void errorHandler()
{

  // We wind up here if an I2C error code was returned.  Increment the error count
  // then display the error code and the current value of errorCount.
  errorCount++;
  Serial.print ("Error Code: 0x");
  Serial.print (errorStatus, HEX);
  Serial.print ("  Error Count Is Now: ");
  Serial.println (errorCount);

  errorStatus = 0;
}
