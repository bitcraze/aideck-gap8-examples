/*
 * config.h
 * Elia Cereda <elia.cereda@idsia.ch>
 *
 * Copyright (C) 2023-2025 IDSIA, USI-SUPSI
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * This software is based on the following publication:
 *    E. Cereda, A. Giusti, D. Palossi. "NanoCockpit: Performance-optimized 
 *    Application Framework for AI-based Autonomous Nanorobotics"
 * We kindly ask for a citation if you use in academic work.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define VERBOSE

// Enable CPX debug prints
#define CPX_VERBOSE

// Enable CPX SPI debug prints
#define CPX_SPI_VERBOSE

/**************************** CPX SETTINGS ****************************/

// Enable bidirectional CPX SPI communication (GAP8<=>ESP32).
// Keep disabled (i.e., GAP8->ESP32 only) unless needed, because it 
// allows much higher SPI bandwidth compared to bidirectional 
// communication. This is further made worse by an AI-deck PCB bug.
// (see also src/nina/main/cpx_spi.c)
#define CPX_SPI_BIDIRECTIONAL

/***********************************************************************
 *                                                                     *
 *          WARNING: DO NOT MODIFY THE FOLLOWING PARAMETERS            *
 *                                                                     *
 **********************************************************************/

/*************************** GPIO SETTINGS ****************************/
// Available GPIOs on AI-Deck:
//                       PMSIS function             Schematic name      Notes
#define GPIO_LED         PI_GPIO_A2_PAD_14_A2    // GAP8_LED            accessible with clip on LED, free
#define GPIO_GAP8_RTT    PI_GPIO_A3_PAD_15_B1    // GAP8_GPIO_NINA_IO   not accessible [ERRATA: 1V8 pin, use only as GAP8 out -> NINA in, DOUBLE ERRATA: according to padframe.xlsx, it's NINA_GPIO_GAP8_IO that is supposed to be 1V8 instead]
#define GPIO_NINA_RTT    PI_GPIO_A18_PAD_32_A13  // NINA_GPIO_GAP8_IO   not accessible
#define GPIO_I2C_SDA     PI_GPIO_A15_PAD_29_B34  // SPARE_I2C_SDA       accessible from header, pulled-up on cf side (also TIMER3_CH3)
#define GPIO_I2C_SCL     PI_GPIO_A16_PAD_30_D1   // SPARE_I2C_SCL       accessible from header, pulled-up on cf side
#define GPIO_TIMER0_CH0  PI_GPIO_A17_PAD_31_B11  // GAP8_TIMER0CH0_1V8  accessible with clip on U9 2, camera MCLK
#define GPIO_UART_RX     PI_GPIO_A24_PAD_38_B6   // GAP8_UART_RX_3V     accessible from header, driven by cf (usable only as input)
#define GPIO_UART_TX     PI_GPIO_A25_PAD_39_A7   // GAP8_UART_TX_1V8    accessible from header, free

#endif /* __CONFIG_H__ */
