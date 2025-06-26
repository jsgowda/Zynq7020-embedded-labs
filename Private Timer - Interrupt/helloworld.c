#include "xgpiops.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "platform.h"

#define GPIO_DEVICE_ID      XPAR_PS7_GPIO_0_DEVICE_ID
#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID
#define PRIVATE_TIMER_IRPT  29   // IRQ ID for Private Timer

#define TIMER_BASE_ADDR     0xF8F00600
#define LOAD_REGISTER       (*(volatile uint32_t *)(TIMER_BASE_ADDR + 0x00))
#define CONTROL_REGISTER    (*(volatile uint32_t *)(TIMER_BASE_ADDR + 0x08))
#define ISR_REGISTER        (*(volatile uint32_t *)(TIMER_BASE_ADDR + 0x0C))

#define TIMER_LOAD_VALUE    0x4A817C80  // 5 seconds @ 250 MHz

XGpioPs Gpio;
XScuGic Intc;

volatile int led_state = 0;

void TimerIntrHandler(void *CallbackRef) {
    // Clear interrupt
    ISR_REGISTER = 0x1;

    // Toggle LED
    led_state ^= 1;
    XGpioPs_WritePin(&Gpio, 0, led_state);
    XGpioPs_WritePin(&Gpio, 9, led_state);

    xil_printf("Private Timer Interrupt: LED %d\n\r", led_state);
}

void InitGpio() {
    XGpioPs_Config *GpioCfg;
    GpioCfg = XGpioPs_LookupConfig(GPIO_DEVICE_ID);
    XGpioPs_CfgInitialize(&Gpio, GpioCfg, GpioCfg->BaseAddr);

    XGpioPs_SetDirectionPin(&Gpio, 0, 1);
    XGpioPs_SetOutputEnablePin(&Gpio, 0, 1);
    XGpioPs_SetDirectionPin(&Gpio, 9, 1);
    XGpioPs_SetOutputEnablePin(&Gpio, 9, 1);

    XGpioPs_WritePin(&Gpio, 0, 0);
    XGpioPs_WritePin(&Gpio, 9, 0);
}

void InitPrivateTimerInterrupt() {
    XScuGic_Config *GicCfg;
    GicCfg = XScuGic_LookupConfig(INTC_DEVICE_ID);
    XScuGic_CfgInitialize(&Intc, GicCfg, GicCfg->CpuBaseAddress);

    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 &Intc);

    XScuGic_Connect(&Intc, PRIVATE_TIMER_IRPT,
                    (Xil_ExceptionHandler)TimerIntrHandler,
                    NULL);
    XScuGic_Enable(&Intc, PRIVATE_TIMER_IRPT);
    Xil_ExceptionEnable();

    // Configure the private timer
    CONTROL_REGISTER = 0x0; // disable timer first
    LOAD_REGISTER = TIMER_LOAD_VALUE;
    CONTROL_REGISTER = 0x07; // enable timer, auto-reload, enable interrupt
}

int main() {
    init_platform();
    xil_printf("Private Timer Interrupt LED Blink Example\n\r");

    InitGpio();
    InitPrivateTimerInterrupt();

    while (1) {
        // Nothing to do; LED toggled via interrupt
    }

    cleanup_platform();
    return 0;
}
