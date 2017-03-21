#ifndef COOLEYE_H
#define COOLEYE_H

void initialCoolEye(int channel=0); //Default SPI Channel is 0
bool isNewFrameAvailable();
bool readFrame(double frame[32][32]);
bool readPixelOffset(int pixel_offset[32][32]);
bool readPixelSensitivity(int pixel_sensitivity[32][32]);
bool readAllParameter(int parameter_list[512]);
void normalizeFrame(double frame[32][32]);
float readAmbientTemperature();

#endif