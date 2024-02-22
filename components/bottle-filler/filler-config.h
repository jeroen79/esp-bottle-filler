#ifndef _FillerConfig_H_
#define _FillerConfig_H_

#include "nlohmann_json.hpp"

using namespace std;
using json = nlohmann::json;

enum FillerStatus
{
    Idle = 0,
    Filling = 1,
    Aborting = 2
};

class FillerConfig
{
public:
    uint8_t id; // non persistant, will we filled during load/save
    string name;
    gpio_num_t pumpPin;
    gpio_num_t autoPin;
    gpio_num_t manualPin;
    uint8_t channel;
    uint8_t autoFillSpeed;   // 0 to 100%
    uint8_t manualFillSpeed; // 0 to 100%
    uint32_t fillTime;       // in ms
    FillerStatus status;

    json to_json()
    {
        json jFillerConfig;
        jFillerConfig["id"] = this->id;
        jFillerConfig["name"] = this->name;
        jFillerConfig["pumpPin"] = this->pumpPin;
        jFillerConfig["autoPin"] = this->autoPin;
        jFillerConfig["manualPin"] = this->manualPin;
        jFillerConfig["autoFillSpeed"] = this->autoFillSpeed;
        jFillerConfig["manualFillSpeed"] = this->manualFillSpeed;
        jFillerConfig["fillTime"] = this->fillTime;

        return jFillerConfig;
    };

    void from_json(json jsonData)
    {
        this->id = jsonData["id"];
        this->name = jsonData["name"];
        this->pumpPin = (gpio_num_t)jsonData["pumpPin"].get<uint>();
        this->autoPin = (gpio_num_t)jsonData["autoPin"].get<uint>();
        this->manualPin = (gpio_num_t)jsonData["manualPin"].get<uint>();
        this->autoFillSpeed = jsonData["autoFillSpeed"].get<uint>();
        this->manualFillSpeed = jsonData["manualFillSpeed"].get<uint>();
        this->fillTime = jsonData["fillTime"].get<uint>();
        this->status = Idle;
    };

protected:
private:
};

#endif
