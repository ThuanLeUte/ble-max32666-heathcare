# ble-max32666-heathcare
BLE - MAX32666 - MAX32664C - MAX86141 - MAX30208

We have the application circuit for wrist biosensing with the microcontroller MAX32666. In this case we are using the MAX32666FTHR(see reference [1])., and the development software tool must be the Eclipse ARMCortex Toolchain(see reference [2]).
We need a BLE code expert to development an embedded software for the MAX32666 to send advertisings from the MAX32666 to a smartphone and receive instructions.
We are look for a library for the MAXM86146 and the MAX32664C connected with MAX86141(this configuration is integrated inside the MAXM86146). Technically it should be the same functions.
The other library is for the temperature sensor MAX30208 to get temperature readings for the sensor.

The requirements of this project are:
1) Library must be written in C language.
2) Library should use the IC2 protocol to communicate between MAX32665/MAX32666(Host Microcontroller) and the MAXM86146/MAX32664C(Sensor Hub), and MAX30208.
3) I2C port in the MAX32666 should be P0_6 SCL and P0_7 SDA.
4) The MAX32666 should be the Master and the MAXM86146/ MAX32664C and MAX30208 should be the slave.
5) Slave addresses could be defined by the user.
6) MAX30208 should reads the temperature in Celsius, also additional Fahrenheit.
7) Interface to Host microcontroller must follow the features presented on MAX32664C datasheet page 21, and MAXM86146 datasheet page 14.
8) MFIO and RESET pins can be selected by the user.
9) Library should have functions based on the MAX32664 User Guide https://pdfserv.maximintegrated.com/en/an/AN6806.pdf and https://pdfserv.maximintegrated.com/en/an/an6924-MAX32664C-user-guide.pdf
10) Functions implemented should be Initialized the Sensor Hub, Sensor Hub status, read and write data to register, get counts from the LEDs, configure LEDs and Pds and configure the HUB sensor in general, read accelerometer data, read Input/Output FIFOs, set operation modes, get SpO2 and Heart Rate data, and all possible features of the Sensor Hub should be implemented.
11) Development sofware for the MAXM86146 could be used as reference to development the library https://www.maximintegrated.com/en/design/software-description.html/swpart=SFW0013460A
12) Sensor Hub library MAX32664 could be used as reference only https://os.mbed.com/users/gmehmet/code/Maxim_Sensor_Hub_Communications_Library/
13) HRM and SpO2 data must be readed by the MAX32666 from the MAXM86146/ MAX32664C.
14)Use the BLE libraries and examples available on the Eclipse ARMCortex Toolchain.
15) Create app_main, main and stack_app C files.
16) Friendly and easy way to add GATT BLE services and create profiles.
17) Implement functions which get data directly from local/global variables for each parameter on the GATT services.
18) Services required are Heart Rate, Body temperature, Blood pressure, battery level, device info.
19) Possibility to set as Central or Peripheral to create a Piconet.
20) Key message security
21) It will be test with a generic BLE app like LightBlue
https://play.google.com/store/apps/details?id=com.punchthrough.lightblueexplorer
22) Review schematics main board and sensor board.

Required files output:
• MAX32665/MAX32666 BLE sotware, .c and .h files.
• MAXM86146/MAX32664C library for the MAX32665/MAX32666 (for example: SensorHUB.c and SensorHUB.h files).
• MAX30208 library.
• “How to use the software” document.
• Examples of the library functions implemented on Eclipse ARMCortex toolchain. Examples could be advertising Gatt services required, piconet example.

References:
[1] MAX32666FTHR Datasheet: https://datasheets.maximintegrated.com/en/ds/MAX32666FTHR.pdf
[2] ARMCortexToolchain: https://www.maximintegrated.com/en/design/software-description.html/swpart=SFW0001500A
[3] MAXM86146 Datasheet: https://datasheets.maximintegrated.com/en/ds/MAXM86146.pdf
[4] MAX32664 Datasheet: https://datasheets.maximintegrated.com/en/ds/MAX32664.pdf
[5] https://datasheets.maximintegrated.com/en/ds/MAX30208.pdf
