/* 
 * This file is part of the ExpressLRS distribution (https://github.com/ExpressLRS/ExpressLRS).
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// Default header files for Express LRS
#include "targets.h"
// OLED specific header files. 
#include <U8g2lib.h>   // Needed for the OLED drivers, this is a arduino package. It is maintained by platformIO


typedef void (*longPresscallback)(uint8_t i);
typedef const char *(*getShowStr)(int i);

#if defined(Regulatory_Domain_ISM_2400)
#define  SCREEN_FR_STRING  "2.4G"
#endif

#if defined(Regulatory_Domain_FCC_915) || defined(Regulatory_Domain_AU_915)
#define  SCREEN_FR_STRING  "915M"
#endif

#if defined(Regulatory_Domain_EU_868) || defined(Regulatory_Domain_IN_866)
#define  SCREEN_FR_STRING  "868M"
#endif
typedef struct 
{
    uint8_t line;
    uint8_t row;
    getShowStr getStr;
} option_t;

typedef struct box_t
{
    /* data */
    uint8_t x;
    uint8_t y;
    uint8_t length;
    uint8_t hight;
};

typedef struct 
{
    uint8_t index;
    uint8_t value;
    option_t option[2];
    box_t box;
    longPresscallback  lpcb;
}  menuShow_t;

class OLED_MENU
{

public:
    static uint32_t lastProcessTime;
    static uint8_t currentIndex;
    static uint8_t screenLocked;
    static uint8_t wifiupdateSta;
    static uint8_t BindingSta;
    static bool FirstTimeBootFlag;
    static char *Hashcode;
private:
    static uint8_t showBaseIndex;
    static menuShow_t currentItem[];

    enum{
    PKTRATE=0,
    TLMRADIO,
    POWERLEVEL,
    RGBCOLOR,
    BINDING,
    WIFIUPDATE
    };

public:
    void displayLockScreen(void);
    void Bind_prompt(void);
    void Boot_animation(void);
    static void Init(void);
    static void start(void);
    static void updateScreen(const char power ,const char rate, const char tlm,char * commitStr);
    static void WIFIUpdateScreen(void);
    static const char * getPowerString(int power);
    static const char * getRateString(int rate);
    static const char * getTLMRatioString(int ratio);
    static const char * getRgbString(int rgb);
    static const char * getBindString(int bind);
    static const char * getUpdateiString(int update);
    void setCommitString(const uint8_t * commit, char * commitStr);

    static void pktRateLPCB(uint8_t i);
    static void tlmLPCB(uint8_t i);
    static void powerLPCB(uint8_t i);
    static void rgbLPCB(uint8_t i);
    static void bindLPCB(uint8_t i);
    static void updateLPCB(uint8_t i);

    static void shortPressCB(void);
    static void longPressCB(void);
    static void HandleUpdataMenu(void);
    void ScreenLocked(void);
    static void menuUpdata(const char * Hashcode);

    static const char *getOptionString(int index);
};





