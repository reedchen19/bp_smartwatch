#include "driverlib.h"
#include <stdio.h>
#include "custom_defs.h"


extern volatile bool sleep;

void main (void)
{
    sleep = false;
    WDT_A_hold(WDT_A_BASE);

    //Enable P2.1 internal resistance as pull-Up resistance
    GPIO_setAsInputPinWithPullUpResistor(
        GPIO_PORT_P2,
        GPIO_PIN1
        );

    //P1.4 interrupt enabled
    GPIO_enableInterrupt(
        GPIO_PORT_P2,
        GPIO_PIN1
        );

    //P1.4 Hi/Lo edge
    GPIO_selectInterruptEdge(
        GPIO_PORT_P2,
        GPIO_PIN1,
        GPIO_HIGH_TO_LOW_TRANSITION
        );


    //P1.4 IFG cleared
    GPIO_clearInterrupt(
        GPIO_PORT_P2,
        GPIO_PIN1
        );

    __enable_interrupt();
    initAlarm();
    initLCD();

    //Enter LPM3 mode with interrupts enabled
    __bis_SR_register(LPM3_bits + GIE);
    __no_operation();
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(PORT2_VECTOR)))
#endif
void Port_2 (void)
{
    if (sleep==true){
        puts("wake up");
        sleep = false;
        updateLCDafterSleep();
        //resetLCD();
        __bic_SR_register_on_exit(LPM3_bits);
        __delay_cycles(2000000); // avoid button bounce

    }
    else{
        puts("sleep");
        sleep = true;
        clearLCD();
        __bis_SR_register_on_exit(LPM3_bits + GIE);
        __delay_cycles(2000000); // avoid button bounce
        //P1.4 IFG cleared
    }

    GPIO_clearInterrupt(
        GPIO_PORT_P2,
        GPIO_PIN1
        );

}




