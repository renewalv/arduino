#ifndef CAMERA_WIFI_H
#define CAMERA_WIFI_H

#include <Arduino.h>


#define WEB_GRAPHICS
#ifdef WEB_GRAPHICS
#include <WiFi.h>
#include <Preferences.h> //https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.cpp
Preferences preferences;
const char *ssid;
const char *password;
void startCameraServer();
void setupLedFlash(int pin);
#endif

void connectToWiFi(const char *ssid, const char *password)
{
    // WiFi.mode(WIFI_AP_STA); ? WiFi.mode(WIFI_STA);

    //todo later: implement WiFiMulti
    /*
    #include <WiFiMulti.h>
    WiFiMulti wifiMulti;
    void setupWiFiMulti()
    {
        Serial.begin(115200);
        delay(10);

        wifiMulti.addAP("ssid_from_AP_1", "your_password_for_AP_1");
        wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
        wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

        Serial.println("Connecting Wifi...");
        if(wifiMulti.run() == WL_CONNECTED) {
            Serial.println("");
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
        }
    }
    */
   
    WiFi.begin(ssid, password);
    Serial.printf("Connecting to WiFi: %s\n", ssid);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 40)
    {
        delay(500);
        log_d(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        log_i("\nWiFi connected\n");
        log_i("IP address: %s", WiFi.localIP().toString().c_str());
    }
    else
    {
        log_e("\nFailed to connect to WiFi.");
    }
}

void initWiFi()
{
    preferences.begin("wifi", false);
    String savedSSID = preferences.getString("ssid", "");
    String savedPASS = preferences.getString("password", "");
    bool savedModeSTA = preferences.getBool("mode", false);
    wifi_mode_t savedMODE = savedModeSTA ? WIFI_MODE_STA : WIFI_MODE_AP;
    
    switch (savedMODE)
    {
    case WIFI_MODE_STA:
        Serial.println("WiFi Mode: STA");
        if (savedSSID.length() > 0)
        {
            Serial.println("Found saved WiFi credentials.");
            if (savedPASS.length() > 0)
            {
                connectToWiFi(savedSSID.c_str(), savedPASS.c_str());
            }
            else
            {
                connectToWiFi(savedSSID.c_str(), "");
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                return;
            }
            else
            {
                log_w("Stored credentials failed. Please enter new credentials.");
            }
        }
        else
        {
            log_i("No WiFi credentials found. Please enter:");
        }

        while (Serial.available())
        {
            Serial.read();
        }
        Serial.println("Enter SSID: ");
        while (Serial.available() == 0)
        {
            delay(10);
        }
        String inputSSID = Serial.readStringUntil('\n');
        inputSSID.trim();

        log_i("Enter Password: ");
        while (Serial.available() == 0)
        {
            delay(10);
        }
        String inputPASS = Serial.readStringUntil('\n');
        inputPASS.trim();

        connectToWiFi(inputSSID.c_str(), inputPASS.c_str());

        if (WiFi.status() == WL_CONNECTED)
        {
            preferences.putString("ssid", inputSSID);
            preferences.putString("password", inputPASS); // todo remake with fs file reading
            log_i("WiFi credentials saved.");
        }
        else
        {
            log_e("Failed to connect. Credentials not saved.");
        }
        break;

    case WIFI_MODE_AP:
        log_i("WiFi Mode: AP");
        if (savedSSID.length() <= 0)
        {
            ssid = "ESP32-CAM";
        }
        else
        {
            ssid = savedSSID.c_str();
        }

        if (savedPASS.length() <= 0)
        {
            password = "";
        }
        else
        {
            password = savedPASS.c_str();
        }
        
        if (!WiFi.softAP(ssid, password)) {
            log_e("Soft AP creation failed.");
            while(1); // todo add reboot or other action
        }
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);
        break;
    }

    preferences.end();
}

#endif // CAMERA_WIFI_H