#include "platform.h"
#include "xgpiops.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "sleep.h"
// Timer Registers and Constants
#define TIMER_BASE_ADDR     XPAR_PS7_SCUTIMER_0_BASEADDR
#define TIMER_LOAD_REG      (TIMER_BASE_ADDR + 0x00)
#define TIMER_COUNT_REG     (TIMER_BASE_ADDR + 0x04)
#define TIMER_CTRL_REG      (TIMER_BASE_ADDR + 0x08)
#define TIMER_LOAD_VAL      0x4A817C80  // 5 seconds (250 MHz clock)

// GPIO Instance
XGpioPs Gpio;

// Function Prototypes
void initialize_gpio(XGpioPs *Gpio);
void initialize_timer();
void delay(int count);

int main() {
    init_platform();

    xil_printf("Initializing GPIO and Timer\n\r");

    // Initialize GPIO and Timer
    initialize_gpio(&Gpio);
    initialize_timer();

    xil_printf("Starting Timer-Controlled LED Blink\n\r");

    // Toggle LEDs based on timer
    while (1) {
        uint32_t timer_val = Xil_In32(TIMER_COUNT_REG);

        // Check if the timer has expired
        if (timer_val <= 0x100) { // Near zero (indicating timer expiration)
            static int led_state = 0;

            // Toggle LED state
            led_state = !led_state;

            // Set or clear MIO0 and MIO9
            XGpioPs_WritePin(&Gpio, 0, led_state); // LED on MIO0
            XGpioPs_WritePin(&Gpio, 9, led_state); // LED on MIO9

            xil_printf("LED State: %d, Reloading Timer\n\r", led_state);

            // Reload the timer for the next 5 seconds
            Xil_Out32(TIMER_LOAD_REG, TIMER_LOAD_VAL);
        }

        // Add a small delay to prevent overwhelming the CPU
        usleep(1000);
        //delay(100000);
    }

    cleanup_platform();
    return 0;
}

// Function to initialize GPIO
void initialize_gpio(XGpioPs *Gpio) {
    XGpioPs_Config *ConfigPtr;
    int Status;

    // Initialize GPIO driver
    ConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);
    Status = XGpioPs_CfgInitialize(Gpio, ConfigPtr, ConfigPtr->BaseAddr);

    if (Status != XST_SUCCESS) {
        xil_printf("GPIO Initialization Failed\n\r");
        while (1); // Halt if initialization fails
    }

    // Configure MIO0 and MIO9 as outputs
    XGpioPs_SetDirectionPin(Gpio, 0, 1); // MIO0
    XGpioPs_SetOutputEnablePin(Gpio, 0, 1);
    XGpioPs_SetDirectionPin(Gpio, 9, 1); // MIO9
    XGpioPs_SetOutputEnablePin(Gpio, 9, 1);

    // Initialize both LEDs to OFF
    XGpioPs_WritePin(Gpio, 0, 0);
    XGpioPs_WritePin(Gpio, 9, 0);
}

// Function to initialize the timer
void initialize_timer() {
    // Load the timer with the desired value for 5 seconds at 200 MHz clock
    Xil_Out32(TIMER_LOAD_REG, TIMER_LOAD_VAL);

    // Enable auto-reload and start the timer
    Xil_Out32(TIMER_CTRL_REG, 0x00000001);  // Enable the timer and auto-reload
}

