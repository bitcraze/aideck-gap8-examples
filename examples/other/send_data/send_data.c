#include "pmsis.h"
#include "stdio.h"
#include "bsp/bsp.h"

#include "cpx.h"
#include "msg.h"

// using a scenario of an april tag detection

static void send_task(void *parameters) {

  uint32_t time_start = pi_time_get_us();

  while (1)
  {    
    static CPXPacket_t txp;
    
    // send every second
    if (pi_time_get_us() - time_start > 1000000)
    {
      // cpx function
      txp.route.destination = CPX_T_STM32;
      // txp.route.source = CPX_T_GAP8;
      txp.route.function = CPX_F_APP;

      TagPacket tag_packet = {};
      // tag_packet.id = pi_time_get_us() % 0xFF;
      tag_packet.id = 1;

      for (int i = 0; i < 4; ++i)
      {
        // create a pixel value
        // tag_packet.corners[i].x = (2 * pi_time_get_us()) % 0xFFFF;
        // tag_packet.corners[i].y = pi_time_get_us() % 0xFFFF;
        tag_packet.corners[i].x = i*2;
        tag_packet.corners[i].y = i*2 + 1;
      }

      // create a homography element
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
        {
          // tag_packet.homography[i][j] = (float)(pi_time_get_us() / 1000000);
          tag_packet.homography[i][j] = (float)(3*i + j);
        }
      
      // debug message variables
      cpxPrintToConsole(LOG_TO_CRTP, 
        "(transmit) id(uint8_t) %d c[0](uint16_t) %d, %d h[0][0](float) %f  h[2][2](float) %f\n", 
        tag_packet.id, tag_packet.corners[0].x, tag_packet.corners[0].y,
        tag_packet.homography[0][0], tag_packet.homography[2][2]);
      
      serialization(tag_packet, &txp);

      // debug message serialized
      // cpxPrintToConsole(LOG_TO_CRTP, 
      //   "id(bytes) %x c[0](bytes) %x %x, %x %x h[0][0](bytes) %x %x %x %x\n", 
      //   txp.data[0], txp.data[1], txp.data[2], txp.data[3],
      //   txp.data[4], txp.data[17], txp.data[18], txp.data[19],
      //   txp.data[20]);
      // cpxPrintToConsole(LOG_TO_CRTP, 
      //   "(transmit) h[0][0](bytes) %x %x %x %x h[0][1](bytes) %x %x %x %x h[0][2](bytes) %x %x %x %x\n", 
      //   txp.data[17], txp.data[18], txp.data[19], txp.data[20],
      //   txp.data[21], txp.data[22], txp.data[23], txp.data[24],
      //   txp.data[25], txp.data[26], txp.data[27], txp.data[28]);

      vTaskDelay(10);

      cpxSendPacketBlocking(&txp);

      time_start = pi_time_get_us();
    }
  }
}

#define LED_PIN 2
static pi_device_t led_gpio_dev;

void hb_task(void *parameters)
{
  (void)parameters;
  char *taskname = pcTaskGetName(NULL);

  // Initialize the LED pin
  pi_gpio_pin_configure(&led_gpio_dev, LED_PIN, PI_GPIO_OUTPUT);

  const TickType_t xDelay = 100 / portTICK_PERIOD_MS;

  while (1)
  {
    pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 1);
    vTaskDelay(xDelay);
    pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 0);
    vTaskDelay(xDelay);
  }
}

void start_example(void)
{
  struct pi_uart_conf conf;
  struct pi_device device;
  pi_uart_conf_init(&conf);
  conf.baudrate_bps = 115200;

  pi_open_from_conf(&device, &conf);
  if (pi_uart_open(&device))
  {
    printf("[UART] open failed !\n");
    pmsis_exit(-1);
  }

  printf("\n-- GAP8 send data application --\n");

  BaseType_t xTask;

  xTask = xTaskCreate(hb_task, "hb_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);
  if (xTask != pdPASS)
  {
    printf("HB task did not start !\n");
    pmsis_exit(-1);
  }

  cpxInit();

  xTask = xTaskCreate(send_task, "test_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);
  if (xTask != pdPASS)
  {
    printf("send task did not start!\n");
    pmsis_exit(-1);
  }

  while (1)
  {
    pi_yield();
  }
}

int main(void)
{
  pi_bsp_init();

  // Increase the FC freq to 250 MHz
  pi_freq_set(PI_FREQ_DOMAIN_FC, 250000000);
  pi_pmu_voltage_set(PI_PMU_DOMAIN_FC, 1200);

  return pmsis_kickoff((void *)start_example);
}
