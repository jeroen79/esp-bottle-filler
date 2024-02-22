/*
 * esp-bolle-filler
 * Copyright (C) Jeroen Dekien 2024 <dekien@gmail.com>
 *
 */

#ifndef _INPUT_H_
#define _INPUT_H_

#include <iostream>
#include "driver/gpio.h"

using namespace std;

enum InputFunction
{
    AutoFill = 0,
    ManualFill = 1
};

class Input
{
public:
    uint8_t Id;
    gpio_num_t GpioNr;
    uint8_t CurrentLevel;
    uint32_t LastChange;
    uint8_t FillerId;
    InputFunction Function;

protected:
private:
};

#endif // _INPUT_H_
