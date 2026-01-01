/*! \file wifi.cpp */
#include <WiFi.h>
#include "ntp.h"
#include "mywifi.h"

/*! \brief Wifi FSM
    \return true if connected
    \todo reconection logic
*/
bool wifi_loop()
{
    static uint8_t phase = 0;
    switch(phase)
    {
        case 0: //offline
            WiFi.begin(SSID_NAME, SSID_PASSWORD);       //!< set SSID and password in the platformio.ini file
            Serial.print("Connecting to WiFi hotspot...\n");
            phase++;
            break;
        case 1: //connecting
            if(WiFi.status() == WL_CONNECTED)
                phase++;
            break;
        case 2:
            Serial.print("Conected!\n");
            init_ntp();
            phase++;
        default:
            break;
    }
    return phase >= 2;
}

//eof wifi.cpp