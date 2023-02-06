/*
 * UART.h
 *
 *  Created on: 2019. jan. 28.
 *      Author: Dell
 */

#ifndef MAIN_UART_H_
#define MAIN_UART_H_
#include <iostream>

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string>
using namespace std;


typedef struct {

    string sData;
    string sTopic;
	char *data;                         /*!< Data asociated with this event */
    char *topic;                        /*!< Topic asociated with this event */
} mqttTopic;

static const int RX_BUF_SIZE = 1024;
static const char *TX_TASK_TAG = "TX_TASK";
static const char *RX_TASK_TAG = "RX_TASK";
static const char *ESP_SOFT_RESET = "espreset";
char endData[] = {0xFF,0xFF,0xFF};

//This defines the UART(0,1,2) and the TX/RX pins on the ESP32
//Default showing is for UART2 Pins 16,17 for the ESP32-WROOM-DevKitC breakout board.
//
//This should really go into the menuconfig for esp-idf
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define nUART	(UART_NUM_2)

class UART {
};

#endif /* MAIN_UART_H_ */
