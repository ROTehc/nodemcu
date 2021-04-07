# NodeMCU

[![oneM2M](https://img.shields.io/badge/oneM2M-f00)](https://www.onem2m.org)

This repository contains the code for nodeMCU devices for ETSI OneM2M Hackaton in Málaga.

## Prerequisites

-   NodeMCU v1.0
-   [LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C) Arduino library

## Headers

### secrets.h

```c
#define SECRET_SSID "xxxxxx"
#define SECRET_PASS "xxxxxx"

#define SECRET_HOST "http://xxx.xxx.xxx.xxx"
#define SECRET_PORT "xxxx"
```

### config.h

```c
#define RESOURCE_NAME "DeviceStreetXXX"

#define LONGITUDE xxxxxx
#define LATITUDE xxxxxx

#define REQUEST_PERIOD xxxxxx
#define MAX_ATTEMPTS xxxxxx
#define MAX_CIN_AGE_DAYS xxxxxx

#define CSE_ORIGINATOR "xxxxxx"
#define CSE_ENDPOINT "xxxxxx"
#define CSE_RELEASE xxxxxx
```
