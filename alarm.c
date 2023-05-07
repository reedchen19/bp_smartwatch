#include "driverlib.h"
#include <stdio.h>
#include "custom_defs.h"

// we need to receive these two numbers from UI
// write decimal in hex but no conversion
// ox14 means 14:00 = 2PM

//volatile int min = 0x01;
//volatile int hr = 0x14;

#define TIMER_PERIOD 20000
volatile Calendar newTime;
//volatile char str_min[16];
//volatile char str_hr[16];
volatile char time[7];
volatile bool sleep;

char * getTime(void){
    return time;
}


// Call this main() from your UI code when the user gives the hour/min for alarm.
void initAlarm (void)
{
    puts("initAlarm called");
    Calendar currentTime;

    WDT_A_hold(WDT_A_BASE);

    sprintf(time, " %02x:%02x ", START_HR, START_MIN);

    // buzzer
    GPIO_setAsOutputPin(
        GPIO_PORT_P3,
        GPIO_PIN5
        );
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P3,
        GPIO_PIN5
            );
    // led on board
    GPIO_setAsOutputPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );


    // Select XT1
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P5,
            GPIO_PIN4 + GPIO_PIN5
            );
    //Initialize LFXT1
    UCS_turnOnLFXT1(UCS_XT1_DRIVE_3,
        UCS_XCAP_3
        );

    //Setup Current Time for Calendar
    currentTime.Seconds    = 0x00;
    currentTime.Minutes    = START_MIN;
    currentTime.Hours      = START_HR;
    currentTime.DayOfWeek  = 0x01;
    currentTime.DayOfMonth = 0x03;
    currentTime.Month      = 0x04;
    currentTime.Year       = 0x2023;

    RTC_A_initCalendar(RTC_A_BASE,
        &currentTime,
        RTC_A_FORMAT_BCD);


    //setAlarm(0x14, 0x01);

    //Specify an interrupt to assert every minute
    RTC_A_setCalendarEvent(RTC_A_BASE,
        RTC_A_CALENDAREVENT_MINUTECHANGE);


    //Enable interrupt for RTC Ready Status, which asserts when the RTC
    //Calendar registers are ready to read.
    //Also, enable interrupts for the Calendar alarm and Calendar event.
    RTC_A_clearInterrupt(RTC_A_BASE,
        RTCRDYIFG + RTCTEVIFG + RTCAIFG);
    RTC_A_enableInterrupt(RTC_A_BASE,
        RTCRDYIE + RTCTEVIE + RTCAIE);


    //Start RTC Clock
    RTC_A_startClock(RTC_A_BASE);

//    //Enter LPM3 mode with interrupts enabled
//    __bis_SR_register(LPM3_bits + GIE);
//    __no_operation();

}

void setAlarm(volatile int hr, volatile int min){
    puts("setAlarm called");
    //Setup Calendar Alarm
    RTC_A_configureCalendarAlarmParam param = {0};
    param.minutesAlarm = min;
    param.hoursAlarm = hr;
    param.dayOfWeekAlarm = RTC_A_ALARMCONDITION_OFF;
    param.dayOfMonthAlarm = RTC_A_ALARMCONDITION_OFF;
    RTC_A_configureCalendarAlarm(RTC_A_BASE, &param);

}

void ten_timer (void)
{
    puts("ten timer called");

    //alarm
    GPIO_setOutputHighOnPin(
        GPIO_PORT_P3,
        GPIO_PIN5
            );
    // led on board
    GPIO_setOutputHighOnPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );

    //Start timer
    Timer_B_clearTimerInterrupt(TIMER_B0_BASE);

    Timer_B_initUpModeParam param = {0};
    param.clockSource = TIMER_B_CLOCKSOURCE_ACLK;
    param.clockSourceDivider = TIMER_B_CLOCKSOURCE_DIVIDER_10;
    param.timerPeriod = TIMER_PERIOD;
    param.timerInterruptEnable_TBIE = TIMER_B_TBIE_INTERRUPT_DISABLE;
    param.captureCompareInterruptEnable_CCR0_CCIE =
        TIMER_B_CAPTURECOMPARE_INTERRUPT_ENABLE;
    param.timerClear = TIMER_B_DO_CLEAR;
    param.startTimer = true;
    Timer_B_initUpMode(TIMER_B0_BASE, &param);

    //Enter LPM0, enable interrupts
    //__bis_SR_register(LPM0_bits + GIE);

    //For debugger
    __no_operation();
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=RTC_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(RTC_VECTOR)))
#endif
void RTC_A_ISR (void)
{

    switch (__even_in_range(RTCIV,16)){
        case 0: break;
        case 2:         //RTCRDYIFG
            break;
        case 4:         //RTCEVIFG
            //Interrupts every minute
            __no_operation();

            //Read out New Time a Minute Later BREAKPOINT HERE
            newTime = RTC_A_getCalendarTime(RTC_A_BASE);

            sprintf(time, " %02x:%02x ", newTime.Hours, newTime.Minutes);
//            puts(time);

//            uint8_t hr0 = newTime.Hours & 0x0f;
//            uint8_t hr1 = (newTime.Hours >> 4) & 0x0f;
//
//            sprintf(str_hr, "%d", hr0);
//            sprintf(str_min, "%d", hr1);
//
//            puts(str_hr);
//            puts(str_min);


            break;
        case 6:         //RTCAIFG
            //Interrupts when the real time reaches the set time
            __bic_SR_register_on_exit(LPM3_bits);
            sleep = false;
            updateLCDafterSleep();
            //resetLCD();
            ten_timer();
            __no_operation();
            break;
        case 8: break;  //RT0PSIFG
        case 10: break; //RT1PSIFG
        case 12: break; //Reserved
        case 14: break; //Reserved
        case 16: break; //Reserved
        default: break;
    }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMERB0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMERB0_VECTOR)))
#endif
void TIMERB0_ISR (void)
{

    //alarm
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P3,
        GPIO_PIN5
            );
    //LED
    GPIO_setOutputLowOnPin(
        GPIO_PORT_P1,
        GPIO_PIN0
        );
    // Stops the timer after one interrupt
    Timer_B_stop(TIMER_B0_BASE);
}



