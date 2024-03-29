//#############################################################################
//
// FILE:   epwm_ex2_updown_aq.c
//
// TITLE:  ePWM Action Qualifier Module - Using up/down count.
//
//! \addtogroup driver_example_list
//! <h1> ePWM Up Down Count Action Qualifier</h1>
//!
//! This example configures ePWM1, ePWM2, EPWM5 to produce a waveform with
//! independent modulation on ePWMxA and ePWMxB.
//!
//! The compare values CMPA and CMPB are modified within the ePWM's ISR.
//!
//! The TB counter is in up/down count mode for this example.
//!
//! View the ePWM1A/B(GPIO0 & GPIO1), ePWM2A/B(GPIO2 &GPIO3)
//! and EPWM5A/B(GPIO4 & GPIO5) waveforms on oscilloscope.
//
//#############################################################################
// $TI Release: F28004x Support Library v1.05.00.00 $
// $Release Date: Thu Oct 18 15:43:57 CDT 2018 $
// $Copyright:
// Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include <stdio.h>
#include "sci.h"

//
// Defines
//
#define EPWM1_TIMER_TBPRD  2000U
#define EPWM1_MAX_CMPA     1950U
#define EPWM1_MIN_CMPA       50U
//#define EPWM1_MAX_CMPA     1000U
//#define EPWM1_MIN_CMPA     1000U
#define EPWM1_MAX_CMPB     1950U
#define EPWM1_MIN_CMPB       50U
//#define EPWM1_MAX_CMPB     1000U
//#define EPWM1_MIN_CMPB     1000U

#define EPWM2_TIMER_TBPRD  2000U
#define EPWM2_MAX_CMPA     1950U
#define EPWM2_MIN_CMPA       50U
#define EPWM2_MAX_CMPB     1950U
#define EPWM2_MIN_CMPB       50U

#define EPWM5_TIMER_TBPRD  850 // about 56kHz
#define EPWM5_MAX_CMPA     425 // 50% duty cycle
#define EPWM5_MIN_CMPA     425
#define EPWM5_MAX_CMPB     425
#define EPWM5_MIN_CMPB     425

//#define EPWM5_TIMER_TBPRD  2000U
//#define EPWM5_MAX_CMPA     1950U
//#define EPWM5_MIN_CMPA     50U
//#define EPWM5_MAX_CMPB     1950U
//#define EPWM5_MIN_CMPB     50U

// 2000, 1000 is about 25% duty cycle

//50% duty cycle with orignal pwm3 example code
//#define EPWM5_TIMER_TBPRD  2000U
//#define EPWM5_MAX_CMPA     100U
//#define EPWM5_MIN_CMPA     100U
//#define EPWM5_MAX_CMPB     100U
//#define EPWM5_MIN_CMPB     100U

#define EPWM_CMP_UP           1U
#define EPWM_CMP_DOWN         0U

//
// Globals
//
typedef struct
{
    uint32_t epwmModule;
    uint16_t epwmCompADirection;
    uint16_t epwmCompBDirection;
    uint16_t epwmTimerIntCount;
    uint16_t epwmMaxCompA;
    uint16_t epwmMinCompA;
    uint16_t epwmMaxCompB;
    uint16_t epwmMinCompB;
}epwmInformation;

//
// Globals to hold the ePWM information used in this example
//
epwmInformation epwm1Info;
epwmInformation epwm2Info;
epwmInformation epwm5Info;

//
// Function Prototypes
//
void initEPWM1(void);
void initEPWM2(void);
void initEPWM5(void);
void itoa(long unsigned int value, char* result, int base);
__interrupt void epwm1ISR(void);
__interrupt void epwm2ISR(void);
__interrupt void epwm5ISR(void);
void updateCompare(epwmInformation *epwmInfo);

//
// Main
//
void main(void)
{

    uint16_t receivedChar;
    unsigned char *msg;
    char *test;

    int dutyCycle = 425;
    double dutyCycleTrack = 0.5;
    int dutyCyclePrint = 0;
    unsigned int period = 850;
    int frequencyPrint = 0;
    int guiState = 0;

    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pull ups.
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Assign the interrupt service routines to ePWM interrupts
    //
    Interrupt_register(INT_EPWM1, &epwm1ISR);
    Interrupt_register(INT_EPWM2, &epwm2ISR);
    Interrupt_register(INT_EPWM5, &epwm5ISR);

    //
    // Configure GPIO0/1 , GPIO2/3 and GPIO4/5 as ePWM1A/1B, ePWM2A/2B and
    // EPWM5A/3B pins respectively
    //
    GPIO_setPadConfig(0, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_0_EPWM1A);
    GPIO_setPadConfig(1, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_1_EPWM1B);

    GPIO_setPadConfig(2, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_2_EPWM2A);
    GPIO_setPadConfig(3, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_3_EPWM2B);

//    GPIO_setPadConfig(4, GPIO_PIN_TYPE_STD);
//    GPIO_setPinConfig(GPIO_4_EPWM5A);
//    GPIO_setPadConfig(5, GPIO_PIN_TYPE_STD);
//    GPIO_setPinConfig(GPIO_5_EPWM5B);

    GPIO_setPadConfig(8, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_8_EPWM5A);
    GPIO_setPadConfig(9, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_9_EPWM5B);

    //
    // GPIO3 is the SCI Rx pin.
    //
    GPIO_setMasterCore(3, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_3_SCIRXDA);
    GPIO_setDirectionMode(3, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(3, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(3, GPIO_QUAL_ASYNC);

    //
    // GPIO2 is the SCI Tx pin.
    //
    GPIO_setMasterCore(2, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_2_SCITXDA);
    GPIO_setDirectionMode(2, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(2, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(2, GPIO_QUAL_ASYNC);

    // set GPIOs for LEDs
    GPIO_setPadConfig(DEVICE_GPIO_PIN_LED1, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);

    //
    // Initialize SCIA and its FIFO.
    //
    SCI_performSoftwareReset(SCIA_BASE);

    //
    // Configure SCIA for echoback.
    //
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 9600, (SCI_CONFIG_WLEN_8 |
                                                        SCI_CONFIG_STOP_ONE |
                                                        SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);

    #ifdef AUTOBAUD
        //
        // Perform an autobaud lock.
        // SCI expects an 'a' or 'A' to lock the baud rate.
        //
        SCI_lockAutobaud(SCIA_BASE);
    #endif

    // start with LED off
    GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1);

    //
    // Disable sync(Freeze clock to PWM as well)
    //
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    initEPWM1();
    initEPWM2();
    initEPWM5();

    //
    // Enable sync and clock to PWM
    //
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    //
    // Enable ePWM interrupts
    //
    Interrupt_enable(INT_EPWM1);
    Interrupt_enable(INT_EPWM2);
    Interrupt_enable(INT_EPWM5);

    //
    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    //
    EINT;
    ERTM;

    //
    // IDLE loop. Just sit and loop forever (optional):
    //
    for(;;)
    {
        // print a bunch of new lines to clear out window
        msg = "\r\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\0";
        SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 25);
        //itoa(50, test, 10);
        //SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 5);

        // print duty cycle and frequency
        //dutyCyclePrint = 100 - (100 * dutyCycleTrack);
        //TODO: convert int to string for printing. Unable to use sprintf or itoa.
        //TODO: fix freq calculation getting overflow or something (get -9000 instead of 56000)
        frequencyPrint = period * 66;  // 66 is the constant I calculated to find frequency from period. Since 850counts = ~56000Hz.

        switch(guiState){
        case 0:
            msg = "\r\n\nChoose an option: \n\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 23);
            msg = "\r\n 1. Change duty cycle \n\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 25);
            msg = "\r\n 2. Change frequency \n\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 24);
            msg = "\r\n 3. Power off \0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 15);
            msg = "\r\n\nEnter number: \0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 17);

            // Read a character from the FIFO.
            receivedChar = SCI_readCharBlockingFIFO(SCIA_BASE);

            switch(receivedChar) {
               case 49  :
                   guiState = 1;
                   break;
               case 50  :
                   guiState = 2;
                   break;
               case 51  :
                   // Enter halt mode.
                   SysCtl_enterHaltMode();
               default :
                   msg = "\r\nPlease choose one of the options\n\0";
                   SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 36);
                   break;
            }
            break;

        case 1:
            msg = "\r\n 1. Increase duty cycle \n\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 27);
            msg = "\r\n 2. Decrease duty cycle \n\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 27);
            msg = "\r\n 3. Go back \n\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 13);
            msg = "\r\n\nEnter number: \0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 17);

            // Read a character from the FIFO.
            receivedChar = SCI_readCharBlockingFIFO(SCIA_BASE);

            switch(receivedChar) {
               case 49  :
                   // Turn on LED
                   GPIO_writePin(DEVICE_GPIO_PIN_LED1, 0);
                   // decrease duty cycle
                   if(dutyCycleTrack < .90){
                       dutyCycleTrack = dutyCycleTrack + 0.005;
                   }
                   dutyCycle = period * dutyCycleTrack;
                   EPWM_setCounterCompareValue(EPWM5_BASE, EPWM_COUNTER_COMPARE_A, dutyCycle);
                   EPWM_setCounterCompareValue(EPWM5_BASE, EPWM_COUNTER_COMPARE_B, dutyCycle);
                   break;
               case 50  :
                   // Turn off LED
                   GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1);
                   // increase duty cycle
                   if(dutyCycleTrack > .001){
                       dutyCycleTrack = dutyCycleTrack - 0.005;
                   }
                   dutyCycle = period * dutyCycleTrack;
                   EPWM_setCounterCompareValue(EPWM5_BASE, EPWM_COUNTER_COMPARE_A, dutyCycle);
                   EPWM_setCounterCompareValue(EPWM5_BASE, EPWM_COUNTER_COMPARE_B, dutyCycle);
                   break;
               case 51  :
                   // return to home
                   guiState = 0;
                   break;
               default :
                   msg = "\r\nPlease choose one of the options\n\0";
                   SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 36);
            }
//            dutyCyclePrint = 100 - (int)(100 * dutyCycleTrack);
//            frequencyPrint = (int)(period * 66);  // 66 is the constant I calculated to find frequency from period. Since 850counts = ~56000Hz.
            break;

        case 2:
            msg = "\r\n 1. Decrease frequency \n\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 26);
            msg = "\r\n 2. Increase frequency \n\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 26);
            msg = "\r\n 3. Go back \n\0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 13);
            msg = "\r\n\nEnter number: \0";
            SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 17);

            // Read a character from the FIFO.
            receivedChar = SCI_readCharBlockingFIFO(SCIA_BASE);

            switch(receivedChar) {
               case 49  :
                   if(period < 1500){
                       // decrease frequency increase period
                       period = period + 50;
                   }
                   // update duty cycle to new period
                   dutyCycle = period * dutyCycleTrack;
                   EPWM_setCounterCompareValue(EPWM5_BASE, EPWM_COUNTER_COMPARE_A, dutyCycle);
                   EPWM_setCounterCompareValue(EPWM5_BASE, EPWM_COUNTER_COMPARE_B, dutyCycle);
                   // update period
                   EPWM_setTimeBasePeriod(EPWM5_BASE, period);
                   break;
               case 50  :
                   if(period > 500){
                       // increase frequency decrease period
                       period = period - 50;
                   }
                   // update duty cycle to new period
                   dutyCycle = period * dutyCycleTrack;
                   EPWM_setCounterCompareValue(EPWM5_BASE, EPWM_COUNTER_COMPARE_A, dutyCycle);
                   EPWM_setCounterCompareValue(EPWM5_BASE, EPWM_COUNTER_COMPARE_B, dutyCycle);
                   // update period
                   EPWM_setTimeBasePeriod(EPWM5_BASE, period);
                   break;
               case 51  :
                   guiState = 0;
                   break;
               default :
                   msg = "\r\nPlease choose one of the options\n\0";
                   SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 36);
            }
//            dutyCyclePrint = 100 - (int)(100 * dutyCycleTrack);
//            frequencyPrint = (int)(period * 66);  // 66 is the constant I calculated to find frequency from period. Since 850counts = ~56000Hz.
            break;

        default:
            guiState = 0;
            break;
        }

        NOP;
    }
}

//
// epwm1ISR - ePWM 1 ISR
//
__interrupt void epwm1ISR(void)
{
    //
    // Update the CMPA and CMPB values
    //
    updateCompare(&epwm1Info);

    //
    // Clear INT flag for this timer
    //
    EPWM_clearEventTriggerInterruptFlag(EPWM1_BASE);

    //
    // Acknowledge interrupt group
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP3);
}

//
// epwm2ISR - ePWM 2 ISR
//
__interrupt void epwm2ISR(void)
{
    //
    // Update the CMPA and CMPB values
    //
    updateCompare(&epwm2Info);

    //
    // Clear INT flag for this timer
    //
    EPWM_clearEventTriggerInterruptFlag(EPWM2_BASE);

    //
    // Acknowledge interrupt group
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP3);
}

//
// EPWM5ISR - ePWM 3 ISR
//
__interrupt void epwm5ISR(void)
{
    //
    // Update the CMPA and CMPB values
    //
    updateCompare(&epwm5Info);

    //
    // Clear INT flag for this timer
    //
    EPWM_clearEventTriggerInterruptFlag(EPWM5_BASE);

    //
    // Acknowledge interrupt group
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP3);
}

//// Implementation of itoa()
//void itoa(long unsigned int value, char* result, int base)
//{
//    // check that the base if valid
//    if (base < 2 || base > 36) { *result = '\0';}
//
//    char* ptr = result, *ptr1 = result, tmp_char;
//    int tmp_value;
//
//    do {
//    tmp_value = value;
//    value /= base;
//    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
//    } while ( value );
//
//    // Apply negative sign
//    if (tmp_value < 0) *ptr++ = '-';
//    *ptr-- = '\0';
//    while(ptr1 < ptr) {
//    tmp_char = *ptr;
//    *ptr--= *ptr1;
//    *ptr1++ = tmp_char;
//    }
//
//}

//
// initEPWM1 - Configure ePWM1
//
void initEPWM1()
{
    //
    // Set-up TBCLK
    //
    EPWM_setTimeBasePeriod(EPWM1_BASE, EPWM1_TIMER_TBPRD);
    EPWM_setPhaseShift(EPWM1_BASE, 0U);
    EPWM_setTimeBaseCounter(EPWM1_BASE, 0U);

    //
    // Set Compare values
    //
    EPWM_setCounterCompareValue(EPWM1_BASE,
                                EPWM_COUNTER_COMPARE_A,
                                EPWM1_MIN_CMPA);
    EPWM_setCounterCompareValue(EPWM1_BASE,
                                EPWM_COUNTER_COMPARE_B,
                                EPWM1_MAX_CMPB);

    //
    // Set up counter mode
    //
    EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_disablePhaseShiftLoad(EPWM1_BASE);
    EPWM_setClockPrescaler(EPWM1_BASE,
                           EPWM_CLOCK_DIVIDER_1,
                           EPWM_HSCLOCK_DIVIDER_1);

    //
    // Set up shadowing
    //
    EPWM_setCounterCompareShadowLoadMode(EPWM1_BASE,
                                         EPWM_COUNTER_COMPARE_A,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);
    EPWM_setCounterCompareShadowLoadMode(EPWM1_BASE,
                                         EPWM_COUNTER_COMPARE_B,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);

    //
    // Set actions
    //
    EPWM_setActionQualifierAction(EPWM1_BASE,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(EPWM1_BASE,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(EPWM1_BASE,
                                  EPWM_AQ_OUTPUT_B,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(EPWM1_BASE,
                                  EPWM_AQ_OUTPUT_B,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);

    //
    // Interrupt where we will change the Compare Values
    // Select INT on Time base counter zero event,
    // Enable INT, generate INT on 3rd event
    //
    EPWM_setInterruptSource(EPWM1_BASE, EPWM_INT_TBCTR_ZERO);
    EPWM_enableInterrupt(EPWM1_BASE);
    EPWM_setInterruptEventCount(EPWM1_BASE, 3U);

    //
    // Information this example uses to keep track of the direction the
    // CMPA/CMPB values are moving, the min and max allowed values and
    // a pointer to the correct ePWM registers
    //
    epwm1Info.epwmCompADirection = EPWM_CMP_UP;
    epwm1Info.epwmCompBDirection = EPWM_CMP_DOWN;
    epwm1Info.epwmTimerIntCount = 0U;
    epwm1Info.epwmModule = EPWM1_BASE;
    epwm1Info.epwmMaxCompA = EPWM1_MAX_CMPA;
    epwm1Info.epwmMinCompA = EPWM1_MIN_CMPA;
    epwm1Info.epwmMaxCompB = EPWM1_MAX_CMPB;
    epwm1Info.epwmMinCompB = EPWM1_MIN_CMPB;
}

//
// initEPWM2 - Configure ePWM2
//
void initEPWM2()
{
    //
    // Set-up TBCLK
    //
    EPWM_setTimeBasePeriod(EPWM2_BASE, EPWM2_TIMER_TBPRD);
    EPWM_setPhaseShift(EPWM2_BASE, 0U);
    EPWM_setTimeBaseCounter(EPWM2_BASE, 0U);

    //
    // Set Compare values
    //
    EPWM_setCounterCompareValue(EPWM2_BASE,
                                EPWM_COUNTER_COMPARE_A,
                                EPWM2_MIN_CMPA);
    EPWM_setCounterCompareValue(EPWM2_BASE,
                                EPWM_COUNTER_COMPARE_B,
                                EPWM2_MIN_CMPB);

    //
    // Set-up counter mode
    //
    EPWM_setTimeBaseCounterMode(EPWM2_BASE, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_disablePhaseShiftLoad(EPWM2_BASE);
    EPWM_setClockPrescaler(EPWM2_BASE,
                           EPWM_CLOCK_DIVIDER_1,
                           EPWM_HSCLOCK_DIVIDER_1);

    //
    // Set-up shadowing
    //
    EPWM_setCounterCompareShadowLoadMode(EPWM2_BASE,
                                         EPWM_COUNTER_COMPARE_A,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);
    EPWM_setCounterCompareShadowLoadMode(EPWM2_BASE,
                                         EPWM_COUNTER_COMPARE_B,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);

    //
    // Set Action qualifier
    //
    EPWM_setActionQualifierAction(EPWM2_BASE,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(EPWM2_BASE,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
    EPWM_setActionQualifierAction(EPWM2_BASE,
                                  EPWM_AQ_OUTPUT_B,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(EPWM2_BASE,
                                  EPWM_AQ_OUTPUT_B,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);

    //
    // Interrupt where we will change the Compare Values
    // Select INT on Time base counter zero event,
    // Enable INT, generate INT on 3rd event
    //
    EPWM_setInterruptSource(EPWM2_BASE, EPWM_INT_TBCTR_ZERO);
    EPWM_enableInterrupt(EPWM2_BASE);
    EPWM_setInterruptEventCount(EPWM2_BASE, 3U);

    //
    // Information this example uses to keep track of the direction the
    // CMPA/CMPB values are moving, the min and max allowed values and
    // a pointer to the correct ePWM registers
    //
    epwm2Info.epwmCompADirection = EPWM_CMP_UP;
    epwm2Info.epwmCompBDirection = EPWM_CMP_UP;
    epwm2Info.epwmTimerIntCount = 0U;
    epwm2Info.epwmModule = EPWM2_BASE;
    epwm2Info.epwmMaxCompA = EPWM2_MAX_CMPA;
    epwm2Info.epwmMinCompA = EPWM2_MIN_CMPA;
    epwm2Info.epwmMaxCompB = EPWM2_MAX_CMPB;
    epwm2Info.epwmMinCompB = EPWM2_MIN_CMPB;
}

//
// initEPWM5 - Configure EPWM5
//
void initEPWM5(void)
{
    //
    // Set-up TBCLK
    //
    EPWM_setTimeBasePeriod(EPWM5_BASE, EPWM5_TIMER_TBPRD);
    EPWM_setPhaseShift(EPWM5_BASE, 0U);
    EPWM_setTimeBaseCounter(EPWM5_BASE, 0U);

    //
    // Set Compare values
    //
    EPWM_setCounterCompareValue(EPWM5_BASE,
                                EPWM_COUNTER_COMPARE_A,
                                EPWM5_MIN_CMPA);
    EPWM_setCounterCompareValue(EPWM5_BASE,
                                EPWM_COUNTER_COMPARE_B,
                                EPWM5_MAX_CMPB);

    //
    // Set up counter mode
    //
    EPWM_setTimeBaseCounterMode(EPWM5_BASE, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_disablePhaseShiftLoad(EPWM5_BASE);
    EPWM_setClockPrescaler(EPWM5_BASE,
                           EPWM_CLOCK_DIVIDER_1,
                           EPWM_HSCLOCK_DIVIDER_1);

    //
    // Set up shadowing
    //
    EPWM_setCounterCompareShadowLoadMode(EPWM5_BASE,
                                         EPWM_COUNTER_COMPARE_A,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);
    EPWM_setCounterCompareShadowLoadMode(EPWM5_BASE,
                                         EPWM_COUNTER_COMPARE_B,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);

    //
    // Set actions
    //
    EPWM_setActionQualifierAction(EPWM5_BASE,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(EPWM5_BASE,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(EPWM5_BASE,
                                  EPWM_AQ_OUTPUT_B,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(EPWM5_BASE,
                                  EPWM_AQ_OUTPUT_B,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);

    //
    // Interrupt where we will change the Compare Values
    // Select INT on Time base counter zero event,
    // Enable INT, generate INT on 3rd event
    //
    EPWM_setInterruptSource(EPWM5_BASE, EPWM_INT_TBCTR_ZERO);
    EPWM_enableInterrupt(EPWM5_BASE);
    EPWM_setInterruptEventCount(EPWM5_BASE, 3U);

    //
    // Information this example uses to keep track of the direction the
    // CMPA/CMPB values are moving, the min and max allowed values and
    // a pointer to the correct ePWM registers
    //
    epwm5Info.epwmCompADirection = EPWM_CMP_UP;
    epwm5Info.epwmCompBDirection = EPWM_CMP_DOWN;
    epwm5Info.epwmTimerIntCount = 0U;
    epwm5Info.epwmModule = EPWM5_BASE;
    epwm5Info.epwmMaxCompA = EPWM5_MAX_CMPA;
    epwm5Info.epwmMinCompA = EPWM5_MIN_CMPA;
    epwm5Info.epwmMaxCompB = EPWM5_MAX_CMPB;
    epwm5Info.epwmMinCompB = EPWM5_MIN_CMPB;
}

//
// updateCompare - Function to update the frequency
//
void updateCompare(epwmInformation *epwmInfo)
{
    uint16_t compAValue;
    uint16_t compBValue;

    compAValue = EPWM_getCounterCompareValue(epwmInfo->epwmModule,
                                             EPWM_COUNTER_COMPARE_A);

    compBValue = EPWM_getCounterCompareValue(epwmInfo->epwmModule,
                                             EPWM_COUNTER_COMPARE_B);

    //
    //  Change the CMPA/CMPB values every 10th interrupt.
    //
    if(epwmInfo->epwmTimerIntCount == 10U)
    {
        epwmInfo->epwmTimerIntCount = 0U;

        //
        // If we were increasing CMPA, check to see if we reached the max
        // value.  If not, increase CMPA else, change directions and decrease
        // CMPA
        //
        if(epwmInfo->epwmCompADirection == EPWM_CMP_UP)
        {
//            if(compAValue < (epwmInfo->epwmMaxCompA))
//            {
//                EPWM_setCounterCompareValue(epwmInfo->epwmModule,
//                                            EPWM_COUNTER_COMPARE_A,
//                                            ++compAValue);
//            }
//            else
//            {
//                epwmInfo->epwmCompADirection = EPWM_CMP_DOWN;
//                EPWM_setCounterCompareValue(epwmInfo->epwmModule,
//                                            EPWM_COUNTER_COMPARE_A,
//                                            --compAValue);
//            }
        }
        //
        // If we were decreasing CMPA, check to see if we reached the min
        // value.  If not, decrease CMPA else, change directions and increase
        // CMPA
        //
        else
        {
//            if( compAValue == (epwmInfo->epwmMinCompA))
//            {
//                epwmInfo->epwmCompADirection = EPWM_CMP_UP;
//                EPWM_setCounterCompareValue(epwmInfo->epwmModule,
//                                            EPWM_COUNTER_COMPARE_A,
//                                            ++compAValue);
//            }
//            else
//            {
//                EPWM_setCounterCompareValue(epwmInfo->epwmModule,
//                                            EPWM_COUNTER_COMPARE_A,
//                                            --compAValue);
//            }
        }

        //
        // If we were increasing CMPB, check to see if we reached the max
        // value.  If not, increase CMPB else, change directions and decrease
        // CMPB
        //
        if(epwmInfo->epwmCompBDirection == EPWM_CMP_UP)
        {
//            if(compBValue < (epwmInfo->epwmMaxCompB))
//            {
//                EPWM_setCounterCompareValue(epwmInfo->epwmModule,
//                                            EPWM_COUNTER_COMPARE_B,
//                                            ++compBValue);
//            }
//            else
//            {
//                epwmInfo->epwmCompBDirection = EPWM_CMP_DOWN;
//                EPWM_setCounterCompareValue(epwmInfo->epwmModule,
//                                            EPWM_COUNTER_COMPARE_B,
//                                            --compBValue);
//            }
        }
        //
        // If we were decreasing CMPB, check to see if we reached the min
        // value.  If not, decrease CMPB else, change directions and increase
        // CMPB
        //
        else
        {
//            if(compBValue == (epwmInfo->epwmMinCompB))
//            {
//                epwmInfo->epwmCompBDirection = EPWM_CMP_UP;
//                EPWM_setCounterCompareValue(epwmInfo->epwmModule,
//                                            EPWM_COUNTER_COMPARE_B,
//                                            ++compBValue);
//            }
//            else
//            {
//                EPWM_setCounterCompareValue(epwmInfo->epwmModule,
//                                            EPWM_COUNTER_COMPARE_B,
//                                            --compBValue);
//            }
        }
    }
    else
    {
        epwmInfo->epwmTimerIntCount++;
    }
}

