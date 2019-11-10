// https://www.raspberrypi.org/forums/viewtopic.php?t=1434&start=10

#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h> 
#include <fcntl.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

int main(void)
{
    int file;
    char filename[20];
    int addr = 0x4C; /* The I2C address */
    int32_t temperature;

    // Open a file giving access to i2c interface 0

    sprintf(filename,"/dev/i2c-1");
    if ((file = open(filename,O_RDWR)) < 0) {
        printf("No temperature measurement\n");
        exit(1);
    }

    // Check that the chip is there
    if (ioctl(file,I2C_SLAVE,addr) < 0) {
        printf("No temperature measurement\n");
        exit(1);
    }

    // Set up continuous measurement
    if (i2c_smbus_write_byte_data(file, 0xac, 0x00) < 0) {
        printf("No temperature measurement\n");
        exit(1);
    }

    // Start measurements
    if (i2c_smbus_write_byte(file, 0xee) < 0) {
        printf("No temperature measurement\n");
        exit(1);
    }

    // read the temperature

    if ((temperature = i2c_smbus_read_byte_data(file, 0xaa)) < 0) {
        printf("No temperature measurement\n");
        exit(1);
    }

    printf("Temperature %d degrees Celsius\n", (int8_t)temperature);
    close(file);
    exit(0);
}