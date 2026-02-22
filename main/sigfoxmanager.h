#ifndef SIGFOX_MANAGER_H
#define SIGFOX_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIGFOX_BAUD 9600
#define SIGFOX_UART_NUM UART_NUM_1

void initSigfox();
void preparePayload(const char* input, char* output, size_t max_len);
void sendSigfoxATCommand(const char* command);
bool sendDataViaSigfox(const char* data);

#endif