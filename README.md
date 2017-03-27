## CoolEye

This is a simple C++ library for "CoolEye" IR sensor to read thermal imaging on Raspberry Pi.

## Prerequisites

1. Because this library is based on [wiringPi](https://github.com/WiringPi/WiringPi) Project, so you need to install it first.

   * http://wiringpi.com/download-and-install/

2. Ensure SPI Interface is enabled on RPi.

   * http://www.raspberrypi-spy.co.uk/2014/08/enabling-the-spi-interface-on-the-raspberry-pi/

## Install

```bash
$ git clone https://github.com/addison822/CoolEye
$ cd CoolEye/
```

  * For dynamic library
  ```bash
  $ sudo make install
  ```
  * For static library
  ```bash
  $ sudo make install-static
  ```
  
## Usage

* test.cpp

```c++
#include <cooleye.h>
   
double frame[32][32];
   
int main(){
    //The default SPI channel is 0,
    //you can also indicate channel 1 
    //by using inttialCoolEye(1);
    initialCoolEye();
       
    readFrame(frame);
       
    return 0;
}
```

* Compile

```bash
$ g++ test.cpp -lwiringPi -lcooleye -o test
```

## Uninstall

```bash
$ cd CoolEye/
$ make uninstall
```
