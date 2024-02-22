/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#ifndef MAIN_BottleFiller_H_
#define MAIN_BottleFiller_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include <esp_http_server.h>
#include "esp_ota_ops.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include <iostream>
#include <string>
#include <sstream>
#include <functional> //function pass
#include <chrono>
#include <iomanip>
#include <ranges>
#include <map>
#include <vector>

#include "settings-manager.h"
#include "filler-config.h"
#include "input.h"

#include "nlohmann_json.hpp"

using namespace std;
using namespace std::chrono;
using std::cout;
using std::endl;
using json = nlohmann::json;

class BottleFiller
{
private:
    static void startAutoFill(void *arg);
    static void reboot(void *arg);
    static void factoryReset(void *arg);

    void startManualFill(uint8_t fillerId);
    void startManualFill(uint8_t fillerId, uint32_t time);
    void stopManualFill(uint8_t fillerId);

    static void interruptLoop(void *arg);

    void readSystemSettings();
    void savePIDSettings();
    void saveSystemSettingsJson(json config);
    void start(uint8_t fillerId);

    string bootIntoRecovery();

    string processCommand(string payLoad);

    void readFillerSettings();
    void saveFillerSettings(json jFillers);
    void setFillerSettings(json jFillers);
    void addDefaultFillers();
    void initFillers();
    void initInputs();

    httpd_handle_t startWebserver(void);
    void stopWebserver(httpd_handle_t server);
    static esp_err_t indexGetHandler(httpd_req_t *req);
    static esp_err_t logoGetHandler(httpd_req_t *req);
    static esp_err_t manifestGetHandler(httpd_req_t *req);
    static esp_err_t otherGetHandler(httpd_req_t *req);
    static esp_err_t apiPostHandler(httpd_req_t *req);
    static esp_err_t apiOptionsHandler(httpd_req_t *req);

    // small helpers
    static string to_iso_8601(std::chrono::time_point<std::chrono::system_clock> t);

    std::map<uint8_t, FillerConfig *> fillers; // we support up to 6 fillers
    vector<Input> inputs;

    SettingsManager *settingsManager;
    httpd_handle_t server;

    // execution
    bool run = false;
    bool controlRun = false; // true when a program is running
    bool interruptRun = false;

    // IO
    std::vector<bool> pumpOn;
    uint8_t gpioHigh = 1;
    uint8_t gpioLow = 0;
    uint16_t maxDuty = 8192;
    bool invertOutputs;

public:
    BottleFiller(SettingsManager *settingsManager); // constructor
    void Init();

    string Hostname;

    // callbacks
    std::function<json()> GetWifiSettingsJson;
    std::function<void(json)> SaveWifiSettingsJson;
    std::function<json()> ScanWifiJson;
};

#endif /* MAIN_BottleFiller_H_ */