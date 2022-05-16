# Tech Ed Satellite Simulator - ESP-IDF Version
This is the original satellite simulator, and it is only compatible with microcontrollers based on the ESP32 chipset. This should work with most ESP32 development boards, though we have not tested compatibility with ESP8266 boards. This repository must be compiled and flashed using Espressif’s IDF, which some of us experienced difficulties installing. One method that worked for us was installing the IDF through Visual Studio Code’s ESP-IDF extension, which also implements the IDF’s build, flash, and monitor tools. If you have an ESP32 microcontroller and can successfully install the IDF, we recommend this version as it has more functionality.

# Usage
1. Clone this repo. `git clone https://github.com/Idaho-ADCS/satellite-simulator-esp-idf`
2. Install ESP-IDF through [Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) or an [executable](https://dl.espressif.com/dl/esp-idf/)
3. Run the following commands:
````
cd front/web-demo
npm install
npm run build
````
5. Set target to appropriate ESP32 chipset
6. Set communication port
7. Build, flash, and monitor
