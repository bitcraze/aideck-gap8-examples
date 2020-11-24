/* 
Decktest
*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/uart.h"
#include "esp_log.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

// For UART test
#define ECHO_TEST_TXD  (GPIO_NUM_1) //nina 22
#define ECHO_TEST_RXD  (GPIO_NUM_3)  //nina 23
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)
#define GPIO_BUTTON_IO    0
#define GPIO_BUTTON_PIN_SEL  (1ULL<<GPIO_BUTTON_IO) 

// pinselect for Gap8 gpio pinouts
#define GAP8_SPI_MISO 23 // nina 1
#define GAP8_GPIO_NINA_IO 32 // nina 5
#define GAP8_SPI_MOSI 19 // nina 21
#define NINA_GPIO_GAP8_IO 2 // nina 25
#define GAP8_SPI_CS 5 // nina 28
#define GAP8_SPI_SCK 18 // nina 29

// pinselect for reset from esp to gap8
#define NINA_GAP8_NRST 27 // nina 18

static void command_uart_task()
{
    // Button GPIO
    gpio_pad_select_gpio(GPIO_BUTTON_IO);
    gpio_set_direction(GPIO_BUTTON_IO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_BUTTON_IO, GPIO_PULLUP_ONLY);

    // GAP8 GPIO setup
    gpio_pad_select_gpio(GAP8_SPI_MISO);
    gpio_set_direction(GAP8_SPI_MISO, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(GAP8_GPIO_NINA_IO);
    gpio_set_direction(GAP8_GPIO_NINA_IO, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(GAP8_SPI_MOSI);
    gpio_set_direction(GAP8_SPI_MOSI, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(NINA_GPIO_GAP8_IO);
    gpio_set_direction(NINA_GPIO_GAP8_IO, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(GAP8_SPI_CS);
    gpio_set_direction(GAP8_SPI_CS, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(GAP8_SPI_SCK);
    gpio_set_direction(GAP8_SPI_SCK, GPIO_MODE_INPUT);

    // GAP8 reset setup
    gpio_pad_select_gpio(NINA_GAP8_NRST);
    gpio_set_level(NINA_GAP8_NRST,1);
    gpio_set_direction(NINA_GAP8_NRST, GPIO_MODE_OUTPUT_OD);


    //Config for the uart driver
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    // Send a start string to CF2
    uint8_t start_char=0xbc;
    uart_write_bytes(UART_NUM_1, (const char *)&start_char, 1);


    uint8_t test_command;
    volatile uint8_t len;
    volatile uint8_t button_press=0;
    uint8_t button_been_high=0;
    uint8_t button_been_low=0;
    uint8_t gap8_has_been_received=0;


    button_press = gpio_get_level(GPIO_BUTTON_IO);
    if(button_press==1)
    {
       button_been_high = 1;
    }


    uint8_t gpio_mask;

    while (1) {
        
        button_press = gpio_get_level(GPIO_BUTTON_IO);
        if(button_press==0)
        {
            button_been_low = 1;
        }

        // fill in gpiomask
        gpio_mask=0x00;
        gpio_mask|=gpio_get_level(GAP8_SPI_MISO)<<0;
        gpio_mask|=gpio_get_level(GAP8_GPIO_NINA_IO)<<1;
        gpio_mask|=gpio_get_level(GAP8_SPI_MOSI)<<2;
        gpio_mask|=gpio_get_level(NINA_GPIO_GAP8_IO)<<3;
        gpio_mask|=gpio_get_level(GAP8_SPI_CS)<<4;
        gpio_mask|=gpio_get_level(GAP8_SPI_SCK)<<5;

        // Read data from the UART
        len = uart_read_bytes(UART_NUM_1, &test_command, 1, 20 / portTICK_RATE_MS);

        uint8_t dummy_true = 1;
        if(len>0){
            switch (test_command){
            case 0x01:
                uart_write_bytes(UART_NUM_1, (const char *) &button_been_high, 1);
                break;
            case 0x02:
                uart_write_bytes(UART_NUM_1, (const char *) &button_been_low, 1);
                break;
            case 0x03: 
                button_been_low = 0;
                uart_write_bytes(UART_NUM_1, (const char *) &dummy_true, 1);
                break;
            case 0x04:
                uart_write_bytes(UART_NUM_1, (const char *) &gpio_mask, 1);
                break;
            case 0x05:
                //set reset pin to 1, wait and then  pull it down again.
                gpio_set_level(NINA_GAP8_NRST,0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                gpio_set_level(NINA_GAP8_NRST,1);
                uart_write_bytes(UART_NUM_1, (const char *) &dummy_true, 1);
                break;            
                }
        }
    }
}


void app_main()
{
    esp_log_level_set("*", ESP_LOG_NONE);
    xTaskCreate(command_uart_task, "command_uart_task", 1024, NULL, 1, NULL);

    //Set the LED GPIO for blinking
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(1) {
        /* Blink off (output low) */
	printf("Turning off the LED\n");
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
	printf("Turning on the LED\n");
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

}
