#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include "STUSB4500.h"
#include <hardware/watchdog.h>

#define STUSB4500_I2C_INSTANCE i2c1
#define STUSB4500_I2C_ADDR 0x28
#define STUSB4500_SDA_PIN 14
#define STUSB4500_SCL_PIN 15
#define STATUS_LED_PIN 25

// Create an instance of the STUSB4500 driver
STUSB4500 usb;

// Fast Blink (100ms on/off) for Initialization Failure
void enterErrorState_InitFail() {
    printf("ERROR: Initialization Failed (usb.begin). Halting with FAST BLINK.\n");
    while (1) {
        gpio_put(STATUS_LED_PIN, 1);
        sleep_ms(100);
        gpio_put(STATUS_LED_PIN, 0);
        sleep_ms(100);
    }
  }
  
  // Slow Blink (1000ms on/off) for Verification Failure
  void enterErrorState_VerifyFail() {
    printf("ERROR: Configuration Verification Failed. Halting with SLOW BLINK.\n");
    while (1) {
        gpio_put(STATUS_LED_PIN, 1);
        sleep_ms(1000);
        gpio_put(STATUS_LED_PIN, 0);
        sleep_ms(1000);
    }
  }

// Helper for comparing float values (adjust tolerance if needed)
bool floatsAreClose(float val1, float val2, float tolerance = 0.01) {
    return abs(val1 - val2) < tolerance;
}

int main() 
{
    stdio_init_all();
    while (!stdio_usb_connected())
    {
        sleep_ms(100);
    }

    printf("--- STUSB4500 Config Verification (Auto Resetting + Slow I2C) ---\n");
    printf("--- Running Setup ---\n");
  
    // Set LED pin mode early
    gpio_init(STATUS_LED_PIN);
    gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
    gpio_put(STATUS_LED_PIN, 0);
  
    sleep_ms(1500); // Increased delay slightly before I2C init
  
    // Initialize I2C communication
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(STUSB4500_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(STUSB4500_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(STUSB4500_SDA_PIN);
    gpio_pull_up(STUSB4500_SCL_PIN);
    bi_decl(bi_2pins_with_func(STUSB4500_SDA_PIN, STUSB4500_SCL_PIN, GPIO_FUNC_I2C));

    // Initialize the STUSB4500 library
    printf("Attempting usb.begin()...\n");
    if (!usb.begin(STUSB4500_I2C_ADDR, STUSB4500_I2C_INSTANCE)) {
      enterErrorState_InitFail(); // *** Fast Blink ***
    }
    printf("usb.begin() successful.\n");
  
    // =======================================================================
    // == STUSB4500 PROGRAMMING/CONFIGURATION CODE (Using Library Functions) ==
    // =======================================================================
    printf("Starting STUSB4500 configuration using library functions...\n");
  
    // Store intended values for verification later
    uint8_t setPdoNum = 3;
    float setPdo1Current = 0.5; uint8_t setPdo1Ovlo = 20;
    float setPdo2Voltage = 20.0; float setPdo2Current = 4.5; uint8_t setPdo2Uvlo = 10; uint8_t setPdo2Ovlo = 20;
    float setPdo3Voltage = 20.0; float setPdo3Current = 5.0; uint8_t setPdo3Uvlo = 20; uint8_t setPdo3Ovlo = 20;
    bool setExtPower = false; bool setUsbComm = true; uint8_t setConfigOk = 2; bool setPwrAbove5 = true;
  
    usb.setPdoNumber(setPdoNum);
    usb.setCurrent(1, setPdo1Current); usb.setUpperVoltageLimit(1, setPdo1Ovlo); // PDO1
    usb.setVoltage(2, setPdo2Voltage); usb.setCurrent(2, setPdo2Current); usb.setLowerVoltageLimit(2, setPdo2Uvlo); usb.setUpperVoltageLimit(2, setPdo2Ovlo); // PDO2
    usb.setVoltage(3, setPdo3Voltage); usb.setCurrent(3, setPdo3Current); usb.setLowerVoltageLimit(3, setPdo3Uvlo); usb.setUpperVoltageLimit(3, setPdo3Ovlo); // PDO3
    usb.setExternalPower(setExtPower); usb.setUsbCommCapable(setUsbComm); usb.setConfigOkGpio(setConfigOk); usb.setPowerAbove5vOnly(setPwrAbove5);
  
    printf("Library configuration set.\n\n");
  
    // =======================================================================
    // == WRITE NVM AND RESET ==
    // =======================================================================
    printf("Attempting usb.write()...\n");
    usb.write(); // Write ALL settings to NVM
    printf("usb.write() called.\n");
    sleep_ms(50);
  
    printf("Attempting usb.softReset()...\n");
    usb.softReset(); // Apply settings from NVM
    printf("usb.softReset() called.\n");
    sleep_ms(750); // Post-reset delay
    printf("Post-reset delay complete.\n\n");
    // =======================================================================
  
  
    // =======================================================================
    // == VERIFY CONFIGURATION BY READING BACK ==
    // =======================================================================
    printf("--- Verifying Configuration via Read-back ---\n");
    bool verificationFailed = false;
  
    // --- Verify PDO Number ---
    // NOTE: Assumes getPdoNumber() function exists in the library!
    uint8_t readPdoNum = usb.getPdoNumber(); // Read back actual number of PDOs
    printf("Verifying PDO Number... Set: "); 
    printf("%f", setPdoNum);
    printf(", Read: "); 
    printf("%f\n", readPdoNum);
    if (readPdoNum != setPdoNum) {
        printf("  MISMATCH!\n");
        verificationFailed = true;
    }
  
    // --- Verify PDO 2 Settings ---
    // NOTE: Assumes getVoltage() and getCurrent() functions exist!
    float readPdo2Voltage = usb.getVoltage(2);
    float readPdo2Current = usb.getCurrent(2);
  
    printf("Verifying PDO2 Voltage... Set: "); 
    printf("%.1f", setPdo2Voltage);
    printf(", Read: "); 
    printf("%.1f\n", readPdo2Voltage);
     if (!floatsAreClose(readPdo2Voltage, setPdo2Voltage)) {
        printf("  MISMATCH!\n");
        verificationFailed = true;
    }
  
    printf("Verifying PDO2 Current... Set: "); 
    printf("%.1f", setPdo2Current);
    printf(", Read: "); 
    printf("%.1f\n", readPdo2Current);
    if (!floatsAreClose(readPdo2Current, setPdo2Current)) {
        printf("  MISMATCH!\n");
        verificationFailed = true;
    }
  
    // --- Verify PDO 3 Settings ---
    float readPdo3Voltage = usb.getVoltage(3);
    float readPdo3Current = usb.getCurrent(3);
  
    printf("Verifying PDO3 Voltage... Set: "); 
    printf("%.1f", setPdo3Voltage);
    printf(", Read: "); 
    printf("%.1f\n", readPdo3Voltage);
     if (!floatsAreClose(readPdo3Voltage, setPdo3Voltage)) {
        printf("  MISMATCH!\n");
        verificationFailed = true;
    }
  
    printf("Verifying PDO3 Current... Set: "); 
    printf("%.1f", setPdo3Current);
    printf(", Read: ");
    printf("%.1f\n", readPdo3Current);
     if (!floatsAreClose(readPdo3Current, setPdo3Current)) {
        printf("  MISMATCH!\n");
        verificationFailed = true;
    }
  
    // Add more checks if needed
  
    // --- Final Verification Check ---
    if (verificationFailed) {
        enterErrorState_VerifyFail(); // *** Slow Blink ***
    } else {
        printf("Configuration Verification Successful!\n");
    }
  
    // If code reaches here, setup sequence finished successfully for this cycle.
    printf("--- Setup Complete ---\n");
    printf("Turning Status LED ON SOLID for Success indication (until reset).\n");
    gpio_put(STATUS_LED_PIN, 1); // Turn LED ON SOLID for SUCCESS
  
    // The setup() function does all the work in this sketch.
    // If setup completes successfully, the status LED will be solid ON.
    // If an error occurred, the code execution is halted in an error state loop.
  
    printf("\nEntering loop() - delaying before reset...\n");
  
    sleep_ms(10000); // Wait for 10 seconds
  
    printf("Resetting...\n");
    sleep_ms(100); // Short delay to allow Serial message to send
  
    // --- Software Reset ---
    watchdog_reboot(0, 0, 0);
    while (true);
}