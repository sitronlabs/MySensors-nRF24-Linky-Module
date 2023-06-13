#ifndef CONFIG_H
#define CONFIG_H

/* MySensors configuration */
#define MY_DEBUG
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN 9
#define MY_RF24_CS_PIN 10
#define MY_RF24_PA_LEVEL RF24_PA_HIGH
#define MY_DEFAULT_ERR_LED_PIN A0
#define MY_DEFAULT_TX_LED_PIN A1

/* Linky configuration */
#define CONFIG_LINKY_DATA_PIN 2
#define CONFIG_LINKY_DUMMY_PIN 5

/* Leds configuration */
#define CONFIG_LED_LINKY_GREEN_PIN 4
#define CONFIG_LED_LINKY_RED_PIN 3

#endif
