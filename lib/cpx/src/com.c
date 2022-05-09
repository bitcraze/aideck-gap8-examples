
/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * AI-deck GAP8
 *
 * Copyright (C) 2022 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * com.c - SPI interface for ESP32 communication
 */

#include "pmsis.h"
#include "com.h"

#define max(a, b)               \
  (                             \
      {                         \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
      })

#if 0
#define DEBUG_PRINTF printf
#else
#define DEBUG_PRINTF(...) ((void) 0)
#endif /* DEBUG */

// Nina RTT (goes high when Nina is ready to talk)
#define CONFIG_NINA_GPIO_NINA_ACK 18
#define CONFIG_NINA_GPIO_NINA_ACK_PAD PI_PAD_32_A13_TIMER0_CH1
#define CONFIG_NINA_GPIO_NINA_ACK_PAD_FUNC PI_PAD_32_A13_GPIO_A18_FUNC1

// Structs for setting up interrut for the Nina RTT pin
static struct pi_gpio_conf cts_gpio_conf;
static pi_gpio_callback_t cb_gpio;

// GAP8 RTT (pull high when we want to talk)
#define CONFIG_NINA_GPIO_NINA_NOTIF 3
#define CONFIG_NINA_GPIO_NINA_NOTIF_PAD PI_PAD_15_B1_RF_PACTRL3
#define CONFIG_NINA_GPIO_NINA_NOTIF_PAD_FUNC PI_PAD_15_B1_GPIO_A3_FUNC1

#define GPIO_HIGH ((uint32_t)1)
#define GPIO_LOW ((uint32_t)0)

static pi_device_t spi_dev, nina_rtt_dev, gap8_rtt_dev;

// Queues for interacting with COM layer
static QueueHandle_t txq = NULL;
static QueueHandle_t rxq = NULL;

// To optimize sending the queue should fit at least one image
#ifndef TXQ_SIZE
#define TXQ_SIZE (80)
#endif

#ifndef RXQ_SIZE
#define RXQ_SIZE (5)
#endif

static EventGroupHandle_t evGroup;
#define NINA_RTT_BIT (1 << 0)
#define TX_QUEUE_BIT (1 << 1)

#define INITIAL_TRANSFER_SIZE (4)

void vDataReadyISR(void *args)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xEventGroupSetBitsFromISR(evGroup, NINA_RTT_BIT, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void setup_nina_rtt_pin(pi_device_t *device)
{
  // Configure Nina RTT
  pi_gpio_conf_init(&cts_gpio_conf);
  pi_open_from_conf(device, &cts_gpio_conf);
  pi_gpio_open(device);
  pi_gpio_pin_configure(device, CONFIG_NINA_GPIO_NINA_ACK, PI_GPIO_INPUT);
  pi_gpio_pin_notif_configure(device, CONFIG_NINA_GPIO_NINA_ACK, PI_GPIO_NOTIF_RISE);
  pi_pad_set_function(CONFIG_NINA_GPIO_NINA_ACK_PAD, CONFIG_NINA_GPIO_NINA_ACK_PAD_FUNC);

  // Set up interrupt
  uint32_t gpio_mask = (1 << (CONFIG_NINA_GPIO_NINA_ACK & PI_GPIO_NUM_MASK));                   // create pin mask
  pi_gpio_callback_init(&cb_gpio, gpio_mask, vDataReadyISR, (void *)CONFIG_NINA_GPIO_NINA_ACK); // setup callback and handler
  pi_gpio_callback_add(device, &cb_gpio);                                                   // Attach callback to gpio pin
}

static void setup_gap8_rtt_pin(pi_device_t *device)
{
  // Set up GAP8 RTT
  struct pi_gpio_conf gpio_conf;
  pi_gpio_conf_init(&gpio_conf);
  pi_open_from_conf(device, &gpio_conf);
  pi_gpio_open(device);
  pi_gpio_pin_configure(device, CONFIG_NINA_GPIO_NINA_NOTIF, PI_GPIO_OUTPUT);
  pi_pad_set_function(CONFIG_NINA_GPIO_NINA_NOTIF_PAD, CONFIG_NINA_GPIO_NINA_NOTIF_PAD_FUNC);
}

void set_gap8_rtt_pin(pi_device_t *device, uint32_t val)
{
  // Set the GAP8 RTT pin
  if (pi_gpio_pin_write(device, CONFIG_NINA_GPIO_NINA_NOTIF, val))
  {
    DEBUG_PRINTF("Could not set notification\n");
    pmsis_exit(-1);
  }
}

static void init_spi(pi_device_t *device)
{
  struct pi_spi_conf spi_conf = {0};

  pi_spi_conf_init(&spi_conf);
  spi_conf.wordsize = PI_SPI_WORDSIZE_8;
  spi_conf.big_endian = 1;
  spi_conf.max_baudrate = 10000000;
  spi_conf.polarity = 0;
  spi_conf.phase = 0;
  spi_conf.itf = 1; // SPI1
  spi_conf.cs = 0;  // CS0

  pi_open_from_conf(device, &spi_conf);

  if (pi_spi_open(device))
  {
    DEBUG_PRINTF("SPI open failed\n");
    pmsis_exit(-1);
  }
}

static uint32_t start;
static uint32_t end;

static packet_t rx_buff;
static packet_t tx_buff;

void com_task(void *parameters)
{
  EventBits_t evBits;
  uint32_t startupESPRTTValue;

  DEBUG_PRINTF("Starting com task\n");

  pi_gpio_pin_read(&nina_rtt_dev, CONFIG_NINA_GPIO_NINA_ACK, &startupESPRTTValue);

  if (startupESPRTTValue > 0) {
    xEventGroupSetBits(evGroup, NINA_RTT_BIT);
  }

  while (1)
  {

    // Check if we have more to send, if not then wait until we have or Nina wants to send
    if (uxQueueMessagesWaiting(txq) == 0) {
      DEBUG_PRINTF("Waiting for action!\n");
      // Wait for either TXQ or RTT from Nina
      evBits = xEventGroupWaitBits(evGroup,
                                  NINA_RTT_BIT | TX_QUEUE_BIT,
                                  pdTRUE, // Clear bits before returning
                                  pdFALSE, // Wait for any bit
                                  portMAX_DELAY);
      DEBUG_PRINTF("Unlocked\n");
      start = xTaskGetTickCount();
    } else {
      // If we didn't unlock on the bits, then reset them
      start = xTaskGetTickCount();
      evBits = 0;
      DEBUG_PRINTF("Still has stuff to send\n");
    }

    if ((evBits & NINA_RTT_BIT) == NINA_RTT_BIT)
    {
      DEBUG_PRINTF("We were awakened by Nina RTT\n");
    }

    if (uxQueueMessagesWaiting(txq) > 0)
    {
      xQueueReceive(txq, &tx_buff, 0);
      DEBUG_PRINTF("Should send packet of size %i\n", tx_buff.len);
    }
    else
    {
      memset(&tx_buff, 0, sizeof(packet_t));
    }

    // Check if we have a package to send (all 0 otherwise)
    if (tx_buff.len > 0)
    {
      set_gap8_rtt_pin(&gap8_rtt_dev, GPIO_HIGH);
      // Check if Nina RTT was set at the same time, if not wait
      if ((evBits & NINA_RTT_BIT) == 0)
      {
        DEBUG_PRINTF("Waiting for Nina RTT\n");
        xEventGroupWaitBits(evGroup, NINA_RTT_BIT, pdTRUE, pdFALSE, (TickType_t)portMAX_DELAY);
      } else {
        DEBUG_PRINTF("Nina RTT already high\n");
      }
    }
    //memset(rx_buff, 0x01, sizeof(packet_t));
    // There's a risk that we've been emptying the queue while another package has been
    // pushed and set the event bit again, which will trigger this loop again.
    // To avoid one extra read (that's not needed) double check here.
    if ((evBits & NINA_RTT_BIT) == NINA_RTT_BIT || tx_buff.len > 0) {
      DEBUG_PRINTF("Initiating SPI tansfer\n");

      pi_spi_transfer(&spi_dev,
                      &tx_buff,
                      &rx_buff,
                      INITIAL_TRANSFER_SIZE * 8,
                      PI_SPI_LINES_SINGLE | PI_SPI_CS_KEEP);

      int tx_len = tx_buff.len;
      int rx_len = rx_buff.len;

      DEBUG_PRINTF("Should read %i bytes\n", rx_len);

      int sizeLeft = max(tx_len - INITIAL_TRANSFER_SIZE + 2, rx_len - INITIAL_TRANSFER_SIZE + 2);

      DEBUG_PRINTF("Transfer size left is %i\n", sizeLeft);

      // Set minumum size left, this works with 0 bytes as well
      sizeLeft = max(0, sizeLeft);

      // We only support transfers which are multiples of 4
      if ((sizeLeft % 4) > 0) {
        sizeLeft += (4-sizeLeft%4); // Pad upwards
      }

      // Protect against the case where the ESP might signal
      // on the RTT line that it wants to send, but actually has
      // no length. Calling the SPI transfer function with size = 0
      // will corrupt the following transaction. Sending random data
      // is ok, since the length is set to 0 and the ESP will ignore it.
      if (sizeLeft == 0) {
        sizeLeft = 4;
      }

      DEBUG_PRINTF("Sending %i bytes\n", sizeLeft);

      // Set GAP8 RTT low before we end the transfer
      set_gap8_rtt_pin(&gap8_rtt_dev, GPIO_LOW);

      // Transfer the remaining bytes
      pi_spi_transfer(&spi_dev,
                      ((uint8_t*)&tx_buff) + INITIAL_TRANSFER_SIZE,
                      ((uint8_t*)&rx_buff) + INITIAL_TRANSFER_SIZE,
                      sizeLeft * 8,
                      PI_SPI_LINES_SINGLE | PI_SPI_CS_AUTO);

      DEBUG_PRINTF("Read %i bytes\n", rx_buff.len);

      if (rx_buff.len > 0)
      {
        if (xQueueSend(rxq, &rx_buff, (TickType_t)portMAX_DELAY) != pdPASS)
        {
          DEBUG_PRINTF("RX Queue full!\n");
        } else {
          DEBUG_PRINTF("Queued packet\n");
        }
      }

      // Do not wait for Nina RTT to go low, we trigger on rising edge anyway
    } else {
      // For debug
      DEBUG_PRINTF("Spurious read\n");
    }
  }
}

void com_init()
{
  DEBUG_PRINTF("Initialize communication\n");

  setup_gap8_rtt_pin(&gap8_rtt_dev);
  init_spi(&spi_dev);

  txq = xQueueCreate(TXQ_SIZE, sizeof(packet_t));
  rxq = xQueueCreate(RXQ_SIZE, sizeof(packet_t));

  if (txq == NULL || rxq == NULL)
  {
    printf("Could not allocate txq and/or rxq in com\n");
    pmsis_exit(1);
  }

  evGroup = xEventGroupCreate();

  BaseType_t xTask;
  xTask = xTaskCreate(com_task, "com_task", configMINIMAL_STACK_SIZE * 6,
                      NULL, tskIDLE_PRIORITY + 1, NULL);
  if (xTask != pdPASS)
  {
    DEBUG_PRINTF("COM task did not start !\n");
    pmsis_exit(-1);
  }

  setup_nina_rtt_pin(&nina_rtt_dev);
}

void com_read(packet_t *p)
{
  xQueueReceive(rxq, p, (TickType_t)portMAX_DELAY);
}

void com_write(packet_t *p)
{
  start = xTaskGetTickCount();
  //printf("Will queue up packet\n");
  xQueueSend(txq, p, (TickType_t)portMAX_DELAY);
  //printf("Have queued up packet!\n");
  xEventGroupSetBits(evGroup, TX_QUEUE_BIT);
}
