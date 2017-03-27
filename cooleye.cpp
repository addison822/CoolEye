#include "cooleye.h"

#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <math.h>
#include <wiringPiSPI.h>

using namespace std;

int CHANNEL; //SPI Channel
unsigned char buffer[2051] = {0};
int pixel_offset[32][32];
int pixel_sensitivity[32][32];
int parameter_list[512];
double PS, PO, SC;

//Stefan Boltzmann constant
const double Boltzmann_constant = 5.670373*0.00000001;

//Emissivity coefficient for human skin is 0.99
const double Emissivity_scaling_coefficient = 0.99;

//Equals SC * Boltzmann constant * Emissivity coefficient
double Tobj_formula_term_1;

void initialCoolEye(int channel){

    //SPI channel is 0 or 1
    CHANNEL = channel%2; 
    
    int fd = wiringPiSPISetupMode(CHANNEL, 1000000, 3);
    if(fd < 0)
        cout << "SPI Setup failed!" << endl;

    readAllParameter(parameter_list);
    PS = (double)parameter_list[4]/1000000;
    SC = (double)parameter_list[7]/1000;
    Tobj_formula_term_1 = SC*Boltzmann_constant*Emissivity_scaling_coefficient;
    //Parameter 5 (Tamb sensor offset) is 2 Bytes
    //and it's indicated as two's complement form
    if(parameter_list[5]&0x80 != 0){
        parameter_list[5] = -((0x10000-parameter_list[5])+1);
    }
    PO = (double)parameter_list[5]/100;

    readPixelOffset(pixel_offset);
    readPixelSensitivity(pixel_sensitivity);

    usleep(100000);
}

bool isNewFrameAvailable(){

    //Send read status command
    buffer[0] = 0x7F;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    buffer[0] = 0x00;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    usleep(2000);
    wiringPiSPIDataRW(CHANNEL, buffer, 2);

    if(buffer[1] == 0x20)
        return true;
    else
        return false;
}

bool readFrame(double frame[32][32]){

    int timeout = 100;
    while(1){

        if(timeout==0){
            cout << "Read Frame Failed!" << endl;
            return false;
        }

        if(isNewFrameAvailable()){

            //Double check
            usleep(30000);
            if(isNewFrameAvailable()){
                //send read frame command
                buffer[0] = 0x7F;
                wiringPiSPIDataRW(CHANNEL, buffer, 1);
                buffer[0] = 0x01;
                wiringPiSPIDataRW(CHANNEL, buffer, 1);
                buffer[0] = 0x00;
                wiringPiSPIDataRW(CHANNEL, buffer, 1);
                usleep(5000);
                wiringPiSPIDataRW(CHANNEL, buffer, 2051);

                int pixel;
                for(int i=0;i<32;i++){
                    for(int j=0;j<32;j++){
                        pixel = buffer[(i*32+j)*2+3]*256 + buffer[(i*32+j)*2+4];
                        frame[i][j] = pixel;
                    }
                }

                //Normalize frame data
                normalizeFrame(frame);

                //Calculate ambient temperature
                double temperature = buffer[1]*256 + buffer[2];
                //Calculate temperature in degrees Celsius
                temperature = temperature*PS + PO;

                //Calcuate object temperature            
                double Tobj_formula_term_2 = pow(temperature+273.15,4);
                for(int i=0;i<32;i++){
                    for(int j=0;j<32;j++){
                        frame[i][j] = pow((frame[i][j]/Tobj_formula_term_1)+Tobj_formula_term_2, 1/4.0)-273.15;
                        //Set data to ambient temperature when data equals to nan or something wrong
                        if(frame[i][j]!=frame[i][j] || frame[i][j]>=50 || frame[i][j]<0)
                            frame[i][j] = temperature;
                    }
                }

                break;
            }
        }
        //Waiting 10 ms
        usleep(10000);
        timeout-=1;
    }

    return true;
}

bool readPixelOffset(int pixel_offset[32][32]){

    buffer[0] = 0x7F;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    buffer[0] = 0x03;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    buffer[0] = 0x01;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    usleep(8000);
    int response = wiringPiSPIDataRW(CHANNEL, buffer, 2049);
    if(response<0)
        return false;

    int offset;
    for(int i=0;i<32;i++){
        for(int j=0;j<32;j++){
            offset = buffer[(i*32+j)*2+1]*256 + buffer[(i*32+j)*2+2];
            pixel_offset[i][j] = offset;
        }
    }

    return true;
}

bool readPixelSensitivity(int pixel_sensitivity[32][32]){

    buffer[0] = 0x7F;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    buffer[0] = 0x04;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    buffer[0] = 0x01;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    usleep(8000);
    int response = wiringPiSPIDataRW(CHANNEL, buffer, 2049);
    if(response<0)
        return false;

    int sensitivity;
    for(int i=0;i<32;i++){
        for(int j=0;j<32;j++){
            sensitivity = buffer[(i*32+j)*2+1]*256 + buffer[(i*32+j)*2+2];
            pixel_sensitivity[i][j] = sensitivity;
        }
    }

    return true;
}

bool readAllParameter(int parameter_list[512]){

    buffer[0] = 0x7F;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    buffer[0] = 0x05;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    buffer[0] = 0x01;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    usleep(6000);
    int response = wiringPiSPIDataRW(CHANNEL, buffer, 1024);
    if(response<0)
        return false;

    int parameter;
    for(int i=0;i<512;i++){
        parameter = buffer[i*2+1]*256 + buffer[i*2+2];
        parameter_list[i] = parameter;
    }

    return true;
}

void normalizeFrame(double frame[32][32]){

    double pixel;
    for(int i=0;i<32;i++){
        for(int j=0;j<32;j++){
            //Offset Correction
            pixel = frame[i][j] - pixel_offset[i][j];
            //Sensitivity Correction
            pixel = pixel / ((double)pixel_sensitivity[i][j]/1000);
            //The normalized pixel
            frame[i][j] = pixel;
        }
    }
}

float readAmbientTemperature(){

    buffer[0] = 0x7F;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    buffer[0] = 0x02;
    wiringPiSPIDataRW(CHANNEL, buffer, 1);
    usleep(2000);
    wiringPiSPIDataRW(CHANNEL, buffer, 4);

    float temperature = buffer[1]*256 + buffer[2];

    //Calculate temperature in degrees Celsius
    temperature = temperature*PS + PO;

    return temperature;
}