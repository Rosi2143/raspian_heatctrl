//https://raspberry-projects.com/pi/programming-in-c/i2c/using-the-i2c-interface

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>           //Needed for I2C port
#include <fcntl.h>           //Needed for I2C port
#include <sys/ioctl.h>     //Needed for I2C port
#include <linux/i2c-dev.h> //Needed for I2C port
#include <stdbool.h>

/* command line commands
set up continuous measurement
sudo i2cset -y 1 0x4C 0xac 0x00 b

start measurements
sudo i2cset -y 1 0x4C 0xee

read the temperature
sudo i2cget -y 1 0x4C 0xaa

 */

// defines from datasheet
#define DS1631_START_CONVERT_T      0x51
#define DS1631_STOP_CONVERT_T       0x22
#define DS1631_READ_TEMPERATURE     0xAA
#define DS1631_ACCESS_TH            0xA1
#define DS1631_ACCESS_TL            0xA2
#define DS1631_ACCESS_CONFIG        0xAC
#define DS1631_SOFTWARE_POR         0x54

#define DS1631_CONFIG_CONVERSTION_DONE_FLAG             (1 << 7)
    #define DS1631_CONFIG_CONVERSTION_IN_PROGRESS       0
    #define DS1631_CONFIG_CONVERSTION_COMPLETE          1
#define DS1631_CONFIG_TEMP_HIGH_FLAG                    (1 << 6)
    #define DS1631_CONFIG_TEMP_HIGH_OVERFLOW_INACTIVE   0
    #define DS1631_CONFIG_TEMP_HIGH_OVERFLOW_ACTIVE     1
#define DS1631_CONFIG_TEMP_LOW_FLAG                     (1 << 5)
    #define DS1631_CONFIG_TEMP_LOW_OVERFLOW_INACTIVE    0
    #define DS1631_CONFIG_TEMP_LOW_OVERFLOW_ACTIVE      1
#define DS1631_CONFIG_NVM_BUSY_FLAG                     (1 << 4)
    #define DS1631_CONFIG_NVM_NOT_BUSY                  0
    #define DS1631_CONFIG_NVM_BUSY                      1
#define DS1631_CONFIG_RESOLUTION_BIT1                   (1 << 3)
#define DS1631_CONFIG_RESOLUTION_BIT0                   (1 << 2)
    #define DS1631_CONFIG_09BIT_094MS                   0
    #define DS1631_CONFIG_10BIT_188MS                   1
    #define DS1631_CONFIG_11BIT_375MS                   2
    #define DS1631_CONFIG_12BIT_750MS                   3
#define DS1631_CONFIG_TOUT_POLARITY                     (1 << 1)
    #define DS1631_CONFIG_ACTIVE_LOW                    0
    #define DS1631_CONFIG_ACTIVE_HIGH                   1
#define DS1631_CONFIG_1SHOT_CONVERSION                  (1 << 0)
    #define DS1631_CONFIG_CONTINUOUS_MODE               0
    #define DS1631_CONFIG_ONE_SHOT_MODE                 1

int file_i2c;

int addr = 0x48; //<<<<<The I2C address of the slave

/************************************
 * general I2C commands
 ************************************/
/*!
 * \brief: send a one byte command
 * \param command - byte value of the command
 */
void i2c_write_byte(unsigned char const *buffer, const int8_t length)
{
    if (write(file_i2c, buffer, length) != length) //write() returns the number of bytes actually written, if it doesn't match then an error occurred (e.g. no response from the device)
    {
        /* ERROR HANDLING: i2c transaction failed */
        printf("Failed to write to the i2c bus.\n");
    }
}

/*!
 * \brief: reads a one byte response
 * \return one byte value returned by last command
 */
bool i2c_read_byte(unsigned char *buffer, const int8_t length)
{
    int16_t result = 0;
    if (length > 2)
    {
        return 0; // there is no reply for DS1621 with more than 2 bytes
    }

    if (read(file_i2c, buffer, length) != length) //read() returns the number of bytes actually read, if it doesn't match then an error occurred (e.g. no response from the device)
    {
        //ERROR HANDLING: i2c transaction failed
        printf("Failed to read from the i2c bus.\n");
        return false;
    }
    else
    {
        printf("Data read 0: %x\n", buffer[0]);
        if (length == 2)
            printf("Data read 1: %x\n", buffer[1]);
    }
    return true;
}

/**************************************
 * DS1631 protcol implementation
 **************************************/
/*!
 * \brief start the conversion of temperature
 *  sudo i2cset -y 1 0x4C 0xee
 */
void ds1631_start_convert()
{
    unsigned char buffer[1] = {0};
    buffer[0] = DS1631_START_CONVERT_T;
    i2c_write_byte(buffer, 1);
}

/*!
 * \brief read temperature temperature
 * sudo i2cget -y 1 0x4C 0xaa
 * 
 * \return temperature in °C
 */
float ds1631_read_temperature()
{
    unsigned char buffer[2] = {0};
    buffer[0] = DS1631_READ_TEMPERATURE;
    i2c_write_byte(buffer, 1);
    int16_t temperature = 0;
    if (i2c_read_byte(buffer, 2))
    {
        temperature = buffer[1] + (buffer[0] << 8);
        printf("Data read: %x\n", temperature);
        printf("current temperature: %d.%d°C\n", temperature / 256, temperature % 256);
    }
    return (temperature / 256 + (temperature % 256) / 1000);
}

/*!
 * \brief read the config register and print the result
 */
void ds1631_read_config()
{
    //sudo i2cget -y 1 0x4C 0xac
    unsigned char buffer[1] = {0};
    buffer[0] = DS1631_ACCESS_CONFIG;
    i2c_write_byte(buffer, 1);
    int8_t config = 0;
    if (i2c_read_byte(buffer, 1))
    {
        config = buffer[0];
        printf("Config: %d\n", config);
        if (config & DS1631_CONFIG_CONVERSTION_DONE_FLAG)
        {
            printf("config: Conversion done");
        }
        else
        {
            printf("config: Conversion in progress");
        }

        if (config & DS1631_CONFIG_TEMP_HIGH_FLAG)
        {
            printf("config: HighTemp overflow active");
        }
        else
        {
            printf("config: HighTemp overflow inactive");
        }

        if (config & DS1631_CONFIG_TEMP_LOW_FLAG)
        {
            printf("config: LowTemp overflow active");
        }
        else
        {
            printf("config: LowTemp overflow inactive");
        }

        if (config & DS1631_CONFIG_NVM_BUSY_FLAG)
        {
            printf("config: NvM write in progress");
        }
        else
        {
            printf("config: NvM write done");
        }

        int8_t ResolutionAndTime = config & (DS1631_CONFIG_RESOLUTION_BIT1 | DS1631_CONFIG_RESOLUTION_BIT0);
        ResolutionAndTime = ResolutionAndTime >> 2;
        if (ResolutionAndTime == DS1631_CONFIG_09BIT_094MS)
        {
            printf("config: accuracy 8Bit, cycle 93.75ms");
        }
        else if (ResolutionAndTime == DS1631_CONFIG_10BIT_188MS)
        {
            printf("config: accuracy 9Bit, cycle 187.5ms");
        }
        else if (ResolutionAndTime == DS1631_CONFIG_11BIT_375MS)
        {
            printf("config: accuracy 11Bit, cycle 375ms");
        }
        else if (ResolutionAndTime == DS1631_CONFIG_12BIT_750MS)
        {
            printf("config: accuracy 12Bit, cycle 750ms");
        }
    }
}

/*!
 * \brief read the upper temperature limit
 * 
 * \param temperature of upper limit in °C
 */
float ds1631_read_upper_temp_trip_point()
{
    //sudo i2cget -y 1 0x4C 0xa1
    unsigned char buffer[2] = {0};
    buffer[0] = DS1631_ACCESS_TH;
    i2c_write_byte(buffer, 1);
    int16_t templimit = 0;
    if (i2c_read_byte(buffer, 2))
    {
        templimit = buffer[1] + (buffer[0] << 8);
        printf("Data read: %x\n", templimit);
        printf("UpperLimit: %d.%d°C\n", templimit / 256, templimit % 256);
    }

    return (templimit / 256 + (templimit % 256) / 1000);
}

/*!
 * \brief write the upper temperature limit
 */
bool ds1631_write_upper_temp_trip_point(float tempLimit)
{
    //sudo i2cget -y 1 0x4C 0xa1
    unsigned char buffer[3] = {0};
    buffer[0] = DS1631_ACCESS_TH;
    buffer[1] = int8_t(tempLimit);
    buffer[2] = 0;
    i2c_write_byte(buffer, 3);
}

/*!
 * \brief read the lower temperature limit
 */
void ds1631_read_lower_temp_trip_point()
{
    //sudo i2cget -y 1 0x4C 0xa1
    unsigned char buffer[2] = {0};
    i2c_write_byte(DS1631_ACCESS_TL);
    int16_t templimit = 0;
    if (i2c_read_byte(buffer, 2))
    {
        templimit = buffer[1] + (buffer[0] << 8);
        printf("Data read: %x\n", templimit);
        printf("LowerLimit: %d.%d°C\n", templimit / 256, templimit % 256);
    }
}

int main(void)
{
    //----- OPEN THE I2C BUS -----
    char *filename = (char *)"/dev/i2c-1";
    if ((file_i2c = open(filename, O_RDWR)) < 0)
    {
        //ERROR HANDLING: you can check errno to see what went wrong
        printf("Failed to open the i2c bus");
        return 1;
    }

    if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
    {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        //ERROR HANDLING; you can check errno to see what went wrong
        return 1;
    }

    //----- WRITE BYTES -----
    ds1631_start_convert();

    ds1631_read_temperature();

    ds1631_read_config();

    ds1631_read_upper_temp_trip_point();
    ds1631_read_lower_temp_trip_point();
}