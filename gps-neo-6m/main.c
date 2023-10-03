/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  -
 *
 *        Version:  1.0
 *        Created:  03/10/2023 08:06:40
 *       Revision:  none
 *       Compiler:  arm-linux-gnueabihf-gcc
 *
 *         Author:  Isaac Vinicius, isaacvinicius2121@alu.ufc.br
 *   Organization:  UFC-Quixadá
 *
 * =====================================================================================
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <string.h>
#include <stdlib.h>
#define UART_PRINT_ID uart0
#define UART_PRINT_TX 0
#define UART_PRINT_RX 1
#define UART_PRINT_BAUD_RATE 115200

#define UART_GPS_ID uart1
#define UART_GPS_TX 4
#define UART_GPS_RX 5
#define UART_GPS_BAUD_RATE 9600

#define BUFFER_SIZE 50 /* Definindo o tamanho do buffer para armazenar os caracteres recebidos */ 
char receive_buffer[BUFFER_SIZE]; /* Armazenando os caracteres recebidos em um buffer */
size_t receive_buffer_index = 0; /* Mantendo um indice atual no buffer */ 

volatile double lat, lon;
char latitude[20];
char longitude[20];

/*referencia: https://microcontrollerslab.com/neo-6m-gps-module-raspberry-pi-pico-micropython/ */
double convertToDegree(double coordinate) {
    // conversao de graus, minutos e segundos para graus decimais
    int degrees = (int)coordinate / 100;        // Obtem os graus inteiros
    double minutes = coordinate - degrees * 100; // Obtem os minutos
    double decimalDegrees = degrees + (minutes / 60.0); // Calcula os graus decimais
    return decimalDegrees;
}

void getLocalizacao(void) {
    /* Dividindo a entrada em linhas */
    char* linha = strtok(receive_buffer, "\n");

    while (linha != NULL) {
        /* Verificando NMEA Sentences: Geographic position, latitude / longitude  */
        if (strstr(linha, "$GPGLL") == linha) {
            char *token;
            int i = 0;
            token = strtok(linha, ",");
            
            while (token != NULL) {
                // no indice 1 contem a latitude
                if (i == 1) {
                    lat = strtod(token, NULL);
                    lat = convertToDegree(lat); // Converte a latitude para graus decimais
                }
                if (i == 2 && *token == 'S') {
                    lat = -lat;
                }  
                // no indice 3 contem a longitude
                if (i == 3) {
                    lon = strtod(token, NULL);
                    lon = convertToDegree(lon); // Converte a longitude para graus decimais
                }
                if (i == 4 && *token == 'W') {
                    lon = -lon;
                }
                token = strtok(NULL, ",");
                i++;
            }
        }

        /* Avançando para a proxima linha */
        linha = strtok(NULL, "\n");
    }

    /* sprintf para converter os valores float em strings */
    sprintf(latitude, "%.8f", lat); 
    sprintf(longitude, "%.8f", lon);

    /* Limpando o buffer para a proxima mensagem */
    receive_buffer_index = 0;
    memset(receive_buffer, 0, sizeof(receive_buffer));
}

void on_uart_gps_irq(void) {
    while (uart_is_readable(UART_GPS_ID)) {
        char received_char = uart_getc(UART_GPS_ID);
        
        /* Armazenando o caractere no buffer */
        if (receive_buffer_index < BUFFER_SIZE - 1) {
            receive_buffer[receive_buffer_index++] = received_char;
            receive_buffer[receive_buffer_index] = '\0'; // Terminando a string com um nulo
        }
        
        /* Verificando se o caractere recebido ha uma quebra de linha (indicando o fim de uma mensagem) */
        if (received_char == '\n') {
            /*Uma linha completa foi recebida, chamando getLocalizacao */
            getLocalizacao();
        }
    }
}


int main() {
    stdio_init_all();

    /* Configuracoes de UART para o modulo GPS e terminal serial */
    uart_init(UART_GPS_ID, UART_GPS_BAUD_RATE);
    gpio_set_function(UART_GPS_RX, GPIO_FUNC_UART);

    uart_init(UART_PRINT_ID, UART_PRINT_BAUD_RATE);
    gpio_set_function(UART_PRINT_TX, GPIO_FUNC_UART);

    /* Habilitando a interrupcao de Received para o UART_GPS_ID */
    uart_set_irq_enables(UART_GPS_ID, true, false);
    /* Definindo a ISR  */
    irq_set_exclusive_handler(UART1_IRQ, on_uart_gps_irq);
    /*Habilitando a interrupção UART */
    irq_set_enabled(UART1_IRQ, true); 

    while (1) {
        /* Imprimindo coordenadas */
        uart_puts(UART_PRINT_ID, latitude);
        uart_puts(UART_PRINT_ID, ",");
        uart_puts(UART_PRINT_ID, longitude);
        uart_puts(UART_PRINT_ID, "\n\r");
        sleep_ms(1000);
    }

    return 0;
}

