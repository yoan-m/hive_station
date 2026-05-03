#ifndef LORAMANAGER_H
#define LORAMANAGER_H

#include <stdbool.h>
#include <stdio.h>

#include "espnow.h"

#define UART_NUM UART_NUM_1
#define UART_BUF_SIZE 1024

#define WAIT_MS 500
void initLora();

void sendLoraATCommand(const char* cmd, uint32_t wait_ms);
void sendDataViaLora(const char* data);
void preparePayloadLora(const char* input, char* output, size_t size);
void sendData(const client_msg_t* data);
#endif