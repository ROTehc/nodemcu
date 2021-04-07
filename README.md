# NodeMCU

[![oneM2M](https://img.shields.io/badge/oneM2M-f00)](https://www.onem2m.org)

This repository contains the code for nodeMCU devices for ETSI OneM2M Hackaton in Málaga.

## Prerequisites

-   NodeMCU v1.0
-   [LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C) Arduino library

## Headers

### secrets.h

```c
#define SECRET_SSID "xxxx"
#define SECRET_PASS "xxxx"
```

-   `SECRET_SSID`: WiFi network's SSID.
-   `SECRET_PASS`: WiFi network's password.

### config.h

```c
#define RESOURCE_NAME "DeviceStreetxxxx"

#define LONGITUDE xxxx
#define LATITUDE xxxx

#define REQUEST_PERIOD xxxx
#define MAX_ATTEMPTS xxxx
#define MAX_CIN xxxx

#define CSE_SCHEMA "xxxx"
#define CSE_ADDRESS "xxx.xxx.xxx.xxx"
#define CSE_PORT xxxx

#define CSE_ORIGINATOR "xxxx"
#define CSE_ENDPOINT "xxxx"
#define CSE_RELEASE xxxx
```

-   `RESOURCE_NAME`: "Device" followed by the name of the street where the device will be installed followed by a number.
-   `LONGITUDE`: Device's longitude.
-   `LATITUDE`: Device's latitude.
-   `REQUEST_PERIOD`: Milliseconds between data readings posts to the CSE. Default: 5000.
-   `MAX_ATTEMPTS`: Maximum number of attemps to connect to the WiFi network. Default: 20.
-   `MAX_CIN`: Maximum number of content instances ("cin") for the DATA container. Default: 5.
-   `CSE_SCHEMA`: Either "http" or "https".
-   `CSE_ADDRESS`: CSE's (IP) address.
-   `CSE_PORT`: CSE's port.
-   `CSE_ORIGINATOR`: X-M2M-Origin header prefix. Default: "Cae-Smart".
-   `CSE_ENDPOINT`: Base CSE endpoint. Default: "\cse-in".
-   `CSE_RELEASE`: X-M2M-RVI header. Default: 3
