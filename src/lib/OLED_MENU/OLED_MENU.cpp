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

#ifdef HAS_I2C_OLED_MENU

#include "targets.h"
#include "OLED_MENU.h"
#include "string.h"
#include "XBMStrings.h" // Contains all the express logos and animation for UI

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, GPIO_PIN_OLED_RST, GPIO_PIN_OLED_SCK, GPIO_PIN_OLED_SDA);

extern void setRGBColor(uint8_t color);
extern void menuSetRate(uint32_t rate);
extern void menuSetTLM(uint32_t TLM);
extern void menuSetPow(uint32_t pow);
extern void uartConnected(void);
extern void OLED_MENU_start(void);
extern void weakupMenu(void);
extern void menuBinding(void);
extern void uartDisconnected(void);
extern void menuWifiUpdate(void);
void shortPressCallback(void);
void longPressCallback(void);

#define LOCKTIME 8000

volatile bool MenuUpdateReq = false;

const char *OLED_MENU::getOptionString(int index)
{
    switch(index)
    {
        case 0: return "Pkt.rate";
        case 1: return "TLM Ratio";
        case 2: return "Power";
        case 3: return "RGB";
        case 4: return "";
        case 5: return "";
        default:return "Error";
    }
}

menuShow_t OLED_MENU::currentItem[] ={
    {
        0,
        0,
        {
            {0,  20,  getOptionString},
            {80, 20,  getRateString}
        },
        {79,12,31,9},
        pktRateLPCB
    },
    {
        1,
        0,
        {
            {0,  30,  getOptionString},
            {80, 30,  getTLMRatioString}
        },
        {79,21,31,10},
        tlmLPCB
    },
    {
        2,
        0,
        {
            {0,  40,  getOptionString},
            {80, 40,  getPowerString}
        },
        {79,32,30,10},
        powerLPCB
    },
    {
        3,
        0,
        {
            {0,  50,  getOptionString},
            {80, 50,  getRgbString}
        },
        {79,42,30,10},
        rgbLPCB
    },
    {
        4,
        0,
        {
            {0, 62,  getOptionString},
            {0, 62,  getBindString}
        },
        {0,53,30,11},
        bindLPCB
    },
    {
        5,
        0,
        {
            {80, 62,  getOptionString},
            {80, 62,  getUpdateiString}
        },
        {79,53,30,11},
        updateLPCB
    },
};


uint32_t OLED_MENU::lastProcessTime=0;
uint8_t  OLED_MENU::currentIndex = 0;
uint8_t  OLED_MENU::showBaseIndex = 0;
uint8_t  OLED_MENU::screenLocked = 0;
uint8_t  OLED_MENU::wifiupdateSta = 0;
uint8_t  OLED_MENU::BindingSta = false;
char*    OLED_MENU::Hashcode;
bool     OLED_MENU::FirstTimeBootFlag = false;

void shortPressCallback(void)
{
    OLED_MENU::shortPressCB();
}


void longPressCallback(void)
{
    OLED_MENU::longPressCB();
}

void OLED_MENU::Init(void)
{
    u8g2.begin();
}

void OLED_MENU::displayLockScreen()
{ 
    u8g2.clearBuffer();
    u8g2.drawXBM(0, 0, 64, 64, elrs64);  
    u8g2.setFont(u8g2_font_HelvetiPixelOutline_tr);
    u8g2.drawUTF8(76,30, "ELRS"); 
    u8g2.drawUTF8(78,50,SCREEN_FR_STRING); 
    u8g2.sendBuffer();
}



void OLED_MENU::Bind_prompt()
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawGlyph(58, 30, 0x23f3);	/* dec 9731/hex 2603 Snowman */
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawUTF8(38,50, "Binding...");
    u8g2.sendBuffer(); 
}

void OLED_MENU::Boot_animation(void)
{
    u8g2.clearBuffer();
    u8g2.drawXBM(0, 32, 32, 32, elrs32); 
    u8g2.sendBuffer(); 
    delay(300);
    u8g2.clearBuffer();
    u8g2.drawXBM(20, 20, 40, 40, elrs40); 
    u8g2.sendBuffer(); 
    delay(300);
    u8g2.clearBuffer();
    u8g2.drawXBM(30, 10, 56, 56, elrs56); 
    u8g2.sendBuffer(); 
    delay(300);
    u8g2.clearBuffer();
    u8g2.drawXBM(35, 0, 64, 64, elrs64); 
    u8g2.sendBuffer(); 
}

void OLED_MENU::WIFIUpdateScreen(void)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_HelvetiPixelOutline_tr);
    u8g2.drawUTF8(15,20, "WIFI Update"); 
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawUTF8(14,40, "http://10.0.0.1/"); 
    u8g2.drawUTF8(20,53, "PW: expresslrs"); 
    u8g2.sendBuffer();
    OLED_MENU::wifiupdateSta = 1;
}

void OLED_MENU::ScreenLocked(void)
{
    uint32_t now = millis();
    if(now - OLED_MENU::lastProcessTime > LOCKTIME && OLED_MENU::screenLocked == 0) // LOCKTIME seconds later lock the screen
    {
        if(!FirstTimeBootFlag)
        {
            OLED_MENU_start();
            FirstTimeBootFlag = true;
        }
        displayLockScreen();
        OLED_MENU::screenLocked = 1;
        OLED_MENU::lastProcessTime = now;
        uartConnected();
    }
}

void OLED_MENU::menuUpdata(const char * Hashcode)
{
    u8g2.clearBuffer();
    u8g2.setFontMode(1);  /* activate transparent font mode */
    u8g2.setDrawColor(2); /* color 1 for the box */
    u8g2.drawBox(0,0,128, 10);
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawUTF8(90,8,Hashcode);
    u8g2.drawUTF8(3,8, "ExpressLRS");

    for(int i=0;i<6;i++)
    {
        u8g2.drawUTF8(currentItem[i].option[0].line,currentItem[i].option[0].row,currentItem[i].option[0].getStr(currentItem[i].index));
        
        if(currentIndex == currentItem[i].index)
        {
            u8g2.drawBox(currentItem[i].box.x,currentItem[i].box.y,strlen(currentItem[i].option[1].getStr(currentItem[i].value))*6+1,currentItem[i].box.hight);         
        }

        u8g2.setDrawColor(2);
        u8g2.drawUTF8(currentItem[i].option[1].line,currentItem[i].option[1].row,currentItem[i].option[1].getStr(currentItem[i].value));
    }
    if(currentIndex == 4)
    {
        BindingSta = true;
    }
    else
    {
        BindingSta = false;
    }
    //delay(100);
    u8g2.sendBuffer();
}

void OLED_MENU::updateScreen(const char power ,const char rate, const char tlm,char * commitStr)
{
     currentItem[0].value = rate;
     currentItem[1].value = tlm;
     currentItem[2].value = power;
     Hashcode = commitStr;
    if(OLED_MENU::screenLocked == 0)
    {
        OLED_MENU::menuUpdata(Hashcode);
    }    
}

void OLED_MENU::shortPressCB(void)
{
    if(!screenLocked)
    {
        lastProcessTime = millis();
        currentIndex ++;

        if(currentIndex > 5) //detect last index,then comeback to first
        {
            currentIndex = 0;
            showBaseIndex =0;
        }
        OLED_MENU::menuUpdata(Hashcode);
    }
}

void OLED_MENU::longPressCB(void)
{
    lastProcessTime = millis();
    if(screenLocked)  //unlock screen
    {
        screenLocked = 0;
        uartDisconnected();   
        weakupMenu();   
        while(!digitalRead(GPIO_PIN_BUTTON));  //Wait for the button to spring up
    }
    else
    {  
        currentItem[currentIndex].lpcb(currentIndex);
    }  

    if(OLED_MENU::BindingSta == false)
    {
        if(OLED_MENU::wifiupdateSta == 0)
        {
            OLED_MENU::menuUpdata(Hashcode);
        }
    }
   
}

void OLED_MENU::pktRateLPCB(uint8_t i)
{
    currentItem[i].value++;

    if(currentItem[i].value>3)
    {
        currentItem[i].value = 0;
    }
    menuSetRate(currentItem[i].value);
}

void OLED_MENU::tlmLPCB(uint8_t i)
{
    currentItem[i].value++;

    if(currentItem[i].value>7)
    {
        currentItem[i].value = 0;
    }
    menuSetTLM(currentItem[i].value);
}

void OLED_MENU::powerLPCB(uint8_t i)
{
    currentItem[i].value++;

    if(currentItem[i].value>7)
    {
        currentItem[i].value = 0;
    }
    menuSetPow(currentItem[i].value);
}

void OLED_MENU::rgbLPCB(uint8_t i)
{
    currentItem[i].value++;

    if(currentItem[i].value>8)
    {
        currentItem[i].value = 0;
    }
    setRGBColor(currentItem[i].value);
}

void OLED_MENU::bindLPCB(uint8_t i)
{   
    menuBinding();
}

void OLED_MENU::updateLPCB(uint8_t i)
{
    menuWifiUpdate();
}


const char * OLED_MENU::getBindString(int bind)
{
    return "[Bind]";
}


const char * OLED_MENU::getUpdateiString(int update)
{
    return "[Update]";
}


const char * OLED_MENU::getRgbString(int rgb)
{
    switch (rgb)
    {
    case 0: return "Cyan";
    case 1: return "Blue";
    case 2: return "White";
    case 3: return "Aqua";
    case 4: return "Red";
    case 5: return "Green";
    case 6: return "Pink";
    case 7: return "Yellow";
    case 8: return "Purple";
    default : return "Purple";
    }
}

/**
 * Returns power level string (Char array)
 *
 * @param values power = int/enum for current power level.  
 * @return const char array for power level Ex: "500 mW\0"
 */
const char *OLED_MENU::getPowerString(int power){
    switch (power)
    {
    case 0: return "10mW";
    case 1: return "25mW";
    case 2: return "50mW";
    case 3: return "100mW";
    case 4: return "250mW";
    case 5: return "500mW";
    case 6: return "1000mW";
    case 7: return "2000mW";
    default: return "Error";
    }
}

/**
 * Returns packet rate string (Char array)
 *
 * @param values rate = int/enum for current packet rate.  
 * @return const char array for packet rate Ex: "500 hz\0"
 */
const char *OLED_MENU::getRateString(int rate){
    switch (rate)
    {
#if defined(Regulatory_Domain_ISM_2400)
    case 0: return "500hz";
    case 1: return "250hz";
    case 2: return "150hz";
    case 3: return "50hz";
#endif 

#if defined(Regulatory_Domain_AU_915) || defined(Regulatory_Domain_EU_868)  || defined(Regulatory_Domain_IN_866) || defined(Regulatory_Domain_FCC_915) || defined(Regulatory_Domain_AU_433) || defined(Regulatory_Domain_EU_433)
      case 0: return "200hz";
      case 1: return "100hz";
      case 2: return "50hz";
      case 3: return "25hz";
#endif
     default: return "50hz";
    }
}

/**
 * Returns telemetry ratio string (Char array)
 *
 * @param values ratio = int/enum for current power level.  
 * @return const char array for telemetry ratio Ex: "1:128\0"
 */
const char *OLED_MENU::getTLMRatioString(int ratio){
    switch (ratio)
    {
    case 0: return "OFF";
    case 1: return "1:128";
    case 2: return "1:64";
    case 3: return "1:32";
    case 4: return "1:16";
    case 5: return "1:8";
    case 6: return "1:4";
    case 7: return "1:2";
    default: return "error";
    }
}

/**
 * Sets the commit string by converting unsigned integers to signed char's (Char array)
 *
 * @param values commit = unsigned 8 byte int array for the commit and char array for commit string.
 * @return void
 */
void OLED_MENU::setCommitString(const uint8_t * commit, char * commitStr){

    for(int i = 0; i < 6; i++ ){
        if (commit[i] < 10) commitStr[i] = commit[i] + 48; // gets us 0 to 9
        else commitStr[i] = commit[i] + 87; // gets us a to f
    }

}
#endif