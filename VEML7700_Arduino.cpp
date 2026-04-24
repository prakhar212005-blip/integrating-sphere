{\rtf1\ansi\ansicpg1252\cocoartf2869
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\fswiss\fcharset0 Helvetica;}
{\colortbl;\red255\green255\blue255;}
{\*\expandedcolortbl;;}
\paperw11900\paperh16840\margl1440\margr1440\vieww11520\viewh8400\viewkind0
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\fs24 \cf0 #include <Wire.h>\
#include <Arduino.h>\
\
// VEML7700 I2C Address (0x10)\
#define VEML7700_ADDR 0x10\
\
// Register Addresses\
#define ALS_CONF 0x00\
#define ALS_DATA 0x04\
\
void VEML7700_Init() \{\
  Wire.begin();\
  \
  // Configuration: Gain 1/8, IT 100ms\
  // Matches your STM32 setup: 0x1000 (LSB: 0x00, MSB: 0x10)\
  Wire.beginTransmission(VEML7700_ADDR);\
  Wire.write(ALS_CONF); \
  Wire.write(0x00); // LSB\
  Wire.write(0x10); // MSB\
  Wire.endTransmission();\
  \
  delay(5); // Datasheet power-up delay\
\}\
\
float VEML7700_ReadLux() \{\
  uint16_t rawData = 0;\
\
  // Point to ALS Data Register\
  Wire.beginTransmission(VEML7700_ADDR);\
  Wire.write(ALS_DATA);\
  Wire.endTransmission(false);\
\
  // Read 2 bytes (LSB first for VEML)\
  Wire.requestFrom(VEML7700_ADDR, 2);\
  if (Wire.available() == 2) \{\
    uint8_t lsb = Wire.read();\
    uint8_t msb = Wire.read();\
    rawData = (msb << 8) | lsb;\
  \}\
\
  // Calculate Lux using your verified resolution factor (0.5760)\
  return rawData * 0.5760f;\
\}}