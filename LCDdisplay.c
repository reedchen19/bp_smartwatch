/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#include <msp430f5529.h>
#include "grlib.h"
#include "button.h"
#include "imageButton.h"
#include "radioButton.h"
#include "checkbox.h"
#include "driverlib.h"
#include "touch_F5529LP.h"
#include "LcdDriver/kitronix320x240x16_ssd2119_spi.h"
#include "Images/images.h"
#include <stdio.h>


//Touch screen context
touch_context g_sTouchContext;

Graphics_Button calibrateButton;
Graphics_Button intervalButton;
Graphics_Button doneButton;
Graphics_ImageButton upButton;
Graphics_ImageButton downButton;


// Graphic library context
Graphics_Context g_sContext;

//Flag to know if a demo was run
bool g_ranDemo = false;
// external flag to know if you need to reset screen after waking up
int screen = 1;

void Delay();
void boardInit(void);
void clockInit(void);
void timerInit(void);
void initializeDemoButtons(void);
void drawMainMenu(void);

void calibrate(void);
void runInterval(void);

#if defined(__IAR_SYSTEMS_ICC__)
int16_t __low_level_init(void) {
    // Stop WDT (Watch Dog Timer)
    WDTCTL = WDTPW + WDTHOLD;
    return(1);
}

#endif


int HR = 60; // HR
int SBP = 150; // uncalibrated SBP

int offset; // calibration offset

// used to convert HR and SBP into string
char str_HR[5];
char str_SBP[5];

//used to select alarm
int alarm_index = 0;
const int ALARM_TIMES = 18; // index for below 2 arrays
char display_times[ALARM_TIMES][7] = {" 09:00 ",
                                      " 09:30 ",
                                      " 10:00 ",
                                      " 10:30 ",
                                      " 11:00 ",
                                      " 11:30 ",
                                      " 12:00 ",
                                      " 12:30 ",
                                      " 13:00 ",
                                      " 13:30 ",
                                      " 14:00 ",
                                      " 14:30 ",
                                      " 15:00 ",
                                      " 15:30 ",
                                      " 16:00 ",
                                      " 16:30 ",
                                      " 17:00 ",
                                      " 17:30 "
                                      };
uint8_t hours[ALARM_TIMES] =  {0x09,
                               0x09,
                               0x10,
                               0x10,
                               0x11,
                               0x11,
                               0x12,
                               0x12,
                               0x13,
                               0x13,
                               0x14,
                               0x14,
                               0x15,
                               0x15,
                               0x16,
                               0x16,
                               0x17,
                               0x17
                               }; //used to set RTC alarm
uint8_t minutes[ALARM_TIMES] =  {
                                 0x00,
                                 0x30,
                                 0x00,
                                 0x30,
                                 0x00,
                                 0x30,
                                 0x00,
                                 0x30,
                                 0x00,
                                 0x30,
                                 0x00,
                                 0x30,
                                 0x00,
                                 0x30,
                                 0x00,
                                 0x30,
                                 0x00,
                                 0x30
                                     }; //used to set RTC alarm

bool changed_alarm = true;

// used to create fake HR/SBP
int counter = 0;
int counter2 = 0;
int heart_rates[6] = {64, 72, 84, 77, 69, 90};
int SBPs[6] = {130, 155, 163, 142, 120, 117};




void initLCD(void)
{
    // Initialization routines
    boardInit();
    clockInit();
    timerInit();
    initializeDemoButtons();

    // LCD setup using Graphics Library API calls
    Kitronix320x240x16_SSD2119Init();
    Graphics_initContext(&g_sContext, &g_sKitronix320x240x16_SSD2119);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setFont(&g_sContext, &g_sFontCmss20b);
    Graphics_clearDisplay(&g_sContext);


    //buzzer pin
    GPIO_setAsOutputPin(
        GPIO_PORT_P3,
        GPIO_PIN5
        );
    //led pin
    GPIO_setAsOutputPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );
    // buzzer
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P3,
        GPIO_PIN5
            );
    // led on board
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );

    // Initialize resistive touch screen
    touch_initInterface();

//    puts("Hello, world!");

    runLCD();

}

void clearLCD(void){
    Graphics_clearDisplay(&g_sContext);
}

void runLCD(void){

    char * time = getTime();

    drawMainMenu();

    while(1){

        // create fake HR data
        counter++;
        if (counter>100){
            counter = 0;
            counter2++;
            if (counter2>=6){
                counter2 = 0;
            }
        }
        HR = heart_rates[counter2];
        SBP = SBPs[counter2];


        // convert HR and SBP to string
        sprintf(str_HR, " %d ", HR);
        sprintf(str_SBP, " %d " , SBP+offset);

//        for (i = 0; i<5; i++){ // reset string to all spaces
//            str2_HR[i] = ' ';
//            str2_SBP[i] = ' ';
//        }
//        for (i = 0; i<3; i++){ // set middle of string to number (the extra spaces cover over the previous text when using OPAQUE_TEXT)
//            str2_HR[i+1] = str_HR[i];
//            str2_SBP[i+1] = str_SBP[i];
//        }


        if ((SBP+offset >= 180) || (SBP+offset <= 100)){
            // alarm for 10 seconds
            ten_timer();
        }

        if(changed_alarm == true){
            setAlarm(hours[alarm_index], minutes[alarm_index]);
            changed_alarm = false;

        }

        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        Graphics_setFont(&g_sContext, &g_sFontCmss40);
        Graphics_drawStringCentered(&g_sContext, str_HR,
                                    AUTO_STRING_LENGTH,
                                    110,
                                    125,
                                    OPAQUE_TEXT);

        Graphics_drawStringCentered(&g_sContext, str_SBP,
                                    AUTO_STRING_LENGTH,
                                    210,
                                    125,
                                    OPAQUE_TEXT);


        Graphics_setFont(&g_sContext, &g_sFontCmss20);
        Graphics_drawStringCentered(&g_sContext, time,
                                    AUTO_STRING_LENGTH,
                                    160,
                                    80,
                                    OPAQUE_TEXT);

        // detect whether button is pressed
        touch_updateCurrentTouch(&g_sTouchContext);

        if(g_sTouchContext.touch)
        {
            if(Graphics_isButtonSelected(&calibrateButton, g_sTouchContext.x,
                                         g_sTouchContext.y))
            {
                Graphics_drawSelectedButton(&g_sContext, &calibrateButton);
                screen = 2;
                calibrate();
            }
            else if(Graphics_isButtonSelected(&intervalButton, g_sTouchContext.x,
                                                          g_sTouchContext.y))
            {
                Graphics_drawSelectedButton(&g_sContext, &intervalButton);
                screen = 3;
                runInterval();
            }

            if(g_ranDemo == true)
            {
                g_ranDemo = false;
                screen = 1;
                drawMainMenu();
            }

        }



    }
}

void updateLCDafterSleep(void){
    switch (screen){
    case 1:
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_clearDisplay(&g_sContext);
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);

        drawMainMenu();
        break;
    case 2:
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_clearDisplay(&g_sContext);
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);

        drawCalibrate();
        break;
    case 3:
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_clearDisplay(&g_sContext);
        Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);

        drawInterval();
        break;
    }
}

void initializeDemoButtons(void)
{

    // Initiliaze images Demo Button
    downButton.xPosition = 190;
    downButton.yPosition = 130;
    downButton.borderWidth = 0;
    downButton.selected = false;
    downButton.imageWidth = down_triangle34BPP_UNCOMP.xSize;
    downButton.imageHeight = down_triangle34BPP_UNCOMP.ySize;
    downButton.borderColor = GRAPHICS_COLOR_BLACK;
    downButton.selectedColor = GRAPHICS_COLOR_BLACK;
    downButton.image = &down_triangle34BPP_UNCOMP;

    upButton.xPosition = 190;
    upButton.yPosition = 55;
    upButton.borderWidth = 0;
    upButton.selected = false;
    upButton.imageWidth = up_triangle34BPP_UNCOMP.xSize;
    upButton.imageHeight = up_triangle34BPP_UNCOMP.ySize;
    upButton.borderColor = GRAPHICS_COLOR_BLACK;
    upButton.selectedColor = GRAPHICS_COLOR_BLACK;
    upButton.image = &up_triangle34BPP_UNCOMP;

    intervalButton.xMin = 130;
    intervalButton.xMax = 190;
    intervalButton.yMin = 40;
    intervalButton.yMax = 70;
    intervalButton.borderWidth = 1;
    intervalButton.selected = false;
    intervalButton.fillColor = GRAPHICS_COLOR_VIOLET;
    intervalButton.borderColor = GRAPHICS_COLOR_VIOLET;
    intervalButton.selectedColor = GRAPHICS_COLOR_DARK_VIOLET;
    intervalButton.textColor = GRAPHICS_COLOR_WHITE;
    intervalButton.selectedTextColor = GRAPHICS_COLOR_WHITE;
    intervalButton.textXPos = 145;
    intervalButton.textYPos = 45;
    intervalButton.text = "Alarm";
    intervalButton.font = &g_sFontCmss18;


    calibrateButton.xMin = 130;
    calibrateButton.xMax = 190;
    calibrateButton.yMin = 170;
    calibrateButton.yMax = 200;
    calibrateButton.borderWidth = 1;
    calibrateButton.selected = false;
    calibrateButton.fillColor = GRAPHICS_COLOR_VIOLET;
    calibrateButton.borderColor = GRAPHICS_COLOR_VIOLET;
    calibrateButton.selectedColor = GRAPHICS_COLOR_DARK_VIOLET;
    calibrateButton.textColor = GRAPHICS_COLOR_WHITE;
    calibrateButton.selectedTextColor = GRAPHICS_COLOR_WHITE;
    calibrateButton.textXPos = 135;
    calibrateButton.textYPos = 175;
    calibrateButton.text = "Calibrate";
    calibrateButton.font = &g_sFontCmss18;

    doneButton.xMin = 130;
    doneButton.xMax = 190;
    doneButton.yMin = 170;
    doneButton.yMax = 200;
    doneButton.borderWidth = 1;
    doneButton.selected = false;
    doneButton.fillColor = GRAPHICS_COLOR_VIOLET;
    doneButton.borderColor = GRAPHICS_COLOR_VIOLET;
    doneButton.selectedColor = GRAPHICS_COLOR_DARK_VIOLET;
    doneButton.textColor = GRAPHICS_COLOR_WHITE;
    doneButton.selectedTextColor = GRAPHICS_COLOR_WHITE;
    doneButton.textXPos = 145;
    doneButton.textYPos = 175;
    doneButton.text = "Done";
    doneButton.font = &g_sFontCmss18;

}

void drawMainMenu(void)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    //Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    //Graphics_clearDisplay(&g_sContext);

    Graphics_fillCircle(&g_sContext, 160, 120, 100);

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_VIOLET);
    Graphics_setFont(&g_sContext, &g_sFontCmss20b);

    Graphics_drawStringCentered(&g_sContext, "HR",
                                AUTO_STRING_LENGTH,
                                110,
                                100,
                                TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "SBP",
                                AUTO_STRING_LENGTH,
                                210,
                                100,
                                TRANSPARENT_TEXT);




    Graphics_drawButton(&g_sContext, &intervalButton);
    Graphics_drawButton(&g_sContext, &calibrateButton);

}

void drawCalibrate(void){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    //Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    //Graphics_clearDisplay(&g_sContext);
    Graphics_fillCircle(&g_sContext, 160, 120, 100);

    //Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_VIOLET);
    Graphics_drawButton(&g_sContext, &doneButton);

    updateSBP(); // draw SBP

    Graphics_setFont(&g_sContext, &g_sFontCmss20);
    Graphics_drawStringCentered(&g_sContext, "Adjust",
                                AUTO_STRING_LENGTH,
                                125,
                                80,
                                TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "SBP to",
                                AUTO_STRING_LENGTH,
                                125,
                                100,
                                TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "match",
                                AUTO_STRING_LENGTH,
                                125,
                                120,
                                TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "BP cuff",
                                AUTO_STRING_LENGTH,
                                125,
                                140,
                                TRANSPARENT_TEXT);
    Graphics_drawImageButton(&g_sContext, &upButton);
    Graphics_drawImageButton(&g_sContext, &downButton);
}

void calibrate(void){
    drawCalibrate();

    do
    {

        touch_updateCurrentTouch(&g_sTouchContext);
        if(Graphics_isImageButtonSelected(&upButton, g_sTouchContext.x,
                                     g_sTouchContext.y))
        {
//            Graphics_drawSelectedImageButton(&g_sContext, &upButton);
            offset++; // increase calibration offset
            updateSBP();
            __delay_cycles(1000000);
        }
        if(Graphics_isImageButtonSelected(&downButton, g_sTouchContext.x,
                                     g_sTouchContext.y))
        {
            offset--; // decrease calibration offset
            updateSBP();
            __delay_cycles(1000000);
        }

    }
    while(!Graphics_isButtonSelected(&doneButton, g_sTouchContext.x,
                                     g_sTouchContext.y));
    Graphics_drawSelectedButton(&g_sContext, &doneButton);

    g_ranDemo = true;
//    __delay_cycles(10000000);
}

void drawInterval(void){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    //Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    //Graphics_clearDisplay(&g_sContext);
    Graphics_fillCircle(&g_sContext, 160, 120, 100);

    //Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_VIOLET);
    Graphics_drawButton(&g_sContext, &doneButton);

    //draw interval screen
//    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
//    Graphics_setFont(&g_sContext, &g_sFontCmss20b);
//    Graphics_drawStringCentered(&g_sContext, "min",
//                                AUTO_STRING_LENGTH,
//                                205,
//                                119,
//                                TRANSPARENT_TEXT);
    updateInterval();

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setFont(&g_sContext, &g_sFontCmss20);
    Graphics_drawStringCentered(&g_sContext, "Set",
                                AUTO_STRING_LENGTH,
                                125,
                                80,
                                TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "time",
                                AUTO_STRING_LENGTH,
                                125,
                                100,
                                TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "for",
                                AUTO_STRING_LENGTH,
                                125,
                                120,
                                TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "alarm",
                                AUTO_STRING_LENGTH,
                                125,
                                140,
                                TRANSPARENT_TEXT);
    Graphics_drawImageButton(&g_sContext, &upButton);
    Graphics_drawImageButton(&g_sContext, &downButton);

}

void runInterval(void){
    drawInterval();
    do
    {

        touch_updateCurrentTouch(&g_sTouchContext);
        if(Graphics_isImageButtonSelected(&upButton, g_sTouchContext.x,
                                     g_sTouchContext.y))
        {
//            Graphics_drawSelectedImageButton(&g_sContext, &upButton);
            alarm_index++; // increase calibration offset
            if (alarm_index>=ALARM_TIMES){
                alarm_index = 0;
            }
            updateInterval();
            __delay_cycles(2000000);
        }
        if(Graphics_isImageButtonSelected(&downButton, g_sTouchContext.x,
                                     g_sTouchContext.y))
        {
            alarm_index--; // decrease calibration offset
            if (alarm_index<0){
                alarm_index = ALARM_TIMES-1;
            }
            updateInterval();
            __delay_cycles(2000000);
        }

    }
    while(!Graphics_isButtonSelected(&doneButton, g_sTouchContext.x,
                                     g_sTouchContext.y));
    Graphics_drawSelectedButton(&g_sContext, &doneButton);

    g_ranDemo = true;
    changed_alarm = true;
//    __delay_cycles(10000000);
}

void updateSBP(){ // draw SBP for the calibrate screen
    sprintf(str_SBP, " %d ", SBP+offset);

//    for (i = 0; i<5; i++){ // reset string to all spaces
//        str2_SBP[i] = ' ';
//    }
//    for (i = 0; i<3; i++){ // set middle of string to number (the extra spaces cover over the previous text when using OPAQUE_TEXT)
//        str2_SBP[i+1] = str_SBP[i];
//    }
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setFont(&g_sContext, &g_sFontCmss40);
    Graphics_drawStringCentered(&g_sContext, str_SBP,
                                AUTO_STRING_LENGTH,
                                210,
                                105,
                                OPAQUE_TEXT);
}

void updateInterval(){ // draw SBP for the calibrate screen


//    for (i = 0; i<3; i++){ // reset string to all spaces
//        str_interval[i] = ' ';
//    }

//    display_times[1] = interval[interval_index] + '0'; //convert to ASCII


    //char display_time[7];
    //sprintf(display_time, "%s", display_times[alarm_index], 7);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setFont(&g_sContext, &g_sFontCmss30);
    Graphics_drawStringCentered(&g_sContext, display_times[alarm_index],
                                7,
                                210,
                                105,
                                OPAQUE_TEXT);


}

void boardInit(void)
{
    // Setup XT1 and XT2
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P5,
        GPIO_PIN2 + GPIO_PIN3 +
        GPIO_PIN4 + GPIO_PIN5
        );
}

void clockInit(void)
{
    UCS_setExternalClockSource(
        32768,
        0);

    // Set Vcore to accomodate for max. allowed system speed
    PMM_setVCore(
        PMM_CORE_LEVEL_3
        );

    // Use 32.768kHz XTAL as reference
    UCS_turnOnLFXT1(
        UCS_XT1_DRIVE_3,
        UCS_XCAP_3
        );

    // Set system clock to max (25MHz)
    UCS_initFLLSettle(
        25000,
        762
        );

    SFR_enableInterrupt(
        SFR_OSCILLATOR_FAULT_INTERRUPT
        );
}

void timerInit()
{
    Timer_A_initUpModeParam timerAUpModeParams =
    {
        TIMER_A_CLOCKSOURCE_SMCLK,
        TIMER_A_CLOCKSOURCE_DIVIDER_64,
        UINT16_MAX,
        TIMER_A_TAIE_INTERRUPT_DISABLE,
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,
        TIMER_A_SKIP_CLEAR
    };
    //Configure timer A to count cycles/64
    Timer_A_initUpMode(
        TIMER_A1_BASE,&timerAUpModeParams);
}

void Delay(){
    __delay_cycles(SYSTEM_CLOCK_SPEED * 3);
}
