/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#include "esp_log.h"
#include "driver/gpio.h"

#include "hal/efuse_hal.h"

#include "wifi-connect.h"
#include "bottle-filler.h"
#include "settings-manager.h"

using std::cout;
using std::endl;
using std::runtime_error;

WiFiConnect *wifiConnector;
string MacAddress;

BottleFiller *bottleFiller;
SettingsManager *settingsManager;

static const char *TAG = "Main";

/* Inside .cpp file, app_main function must be declared with C linkage */
extern "C" void app_main(void)
{
    cout << "BottleFiller booting up!" << endl;

    try
    {
        // We get the mac address from the efuse, this is our unique idenfitier
        uint8_t macArray[6];
        char macChars[18] = {0};
        efuse_hal_get_mac(macArray);
        sprintf(macChars, "%X:%X:%X:%X:%X:%X", macArray[0], macArray[1], macArray[2], macArray[3], macArray[4], macArray[5]);
        string macAddress = macChars;
        ESP_LOGI(TAG, "MacAddress: %s", macChars);

        // Our settings manager is helper for the nvs_get/write
        settingsManager = new SettingsManager();
        settingsManager->Init();

        // WifiConnect is a helper that connects to wifi or starts a wifi ap
        wifiConnector = new WiFiConnect(settingsManager);
        wifiConnector->Connect();

        // We use the webinterface of our app to save the wifi settings, so we need to pipe the get/save functions through
        auto fpGetSettingsJson = std::bind(&WiFiConnect::GetSettingsJson, wifiConnector);
        auto fpSaveSettingsJson = std::bind(&WiFiConnect::SaveSettingsJson, wifiConnector, std::placeholders::_1);
        auto fpScanJson = std::bind(&WiFiConnect::Scan, wifiConnector);

        // BottleFiller is the main component to regulate all brewing stuff
        bottleFiller = new BottleFiller(settingsManager);
        bottleFiller->Hostname = wifiConnector->Hostname;
        bottleFiller->GetWifiSettingsJson = fpGetSettingsJson;
        bottleFiller->SaveWifiSettingsJson = fpSaveSettingsJson;
        bottleFiller->ScanWifiJson = fpScanJson;
        bottleFiller->Init();

        ESP_LOGI(TAG, "BottleFiller booted");
    }
    catch (const runtime_error &e)
    {
        cout << "Exception caught: " << e.what() << endl;
    }

    cout << "app_main done" << endl;
}
