/* Config */
#include "../cfg/config.h"

/* Arduino Libraries */
#include <Arduino.h>
#include <MySensors.h>
#include <SoftwareSerial.h>
#include <tic_reader.h>

/* C/C++ libraries */
#include <ctype.h>
#include <stdlib.h>

/* Working variables */
static SoftwareSerial m_tic_port(CONFIG_TIC_DATA_PIN, CONFIG_TIC_DUMMY_PIN);
static uint16_t m_tic_port_baudrate = 0;
static tic_reader m_tic_reader;
static enum {
    STATE_STARTING,
    STATE_VALID,
    STATE_INVALID,
} m_tic_state;

/* List of virtual sensors */
enum {
    SENSOR_0_SERIAL_NUMBER,                 // V_TEXT
    SENSOR_1_INDEX_BASE,                    // V_KWH
    SENSOR_2_POWER_APPARENT,                // V_WATT
    SENSOR_3_MULTIMETER_PHASE_1,            // V_VOLTAGE and V_CURRENT
    SENSOR_4_MULTIMETER_PHASE_2,            // V_VOLTAGE and V_CURRENT
    SENSOR_5_MULTIMETER_PHASE_3,            // V_VOLTAGE and V_CURRENT
    SENSOR_6_PRICING,                       // V_TEXT
    SENSOR_7_PRICING_TEMPO_COLOR_TOMORROW,  // V_TEXT
};

/**
 * Setup function.
 * Called before MySensors does anything.
 */
void preHwInit(void) {

    /* Setup leds
     * Ensures tic link led is off at startup */
    pinMode(CONFIG_LED_TIC_GREEN_PIN, OUTPUT);
    pinMode(CONFIG_LED_TIC_RED_PIN, OUTPUT);
    digitalWrite(CONFIG_LED_TIC_GREEN_PIN, LOW);
    digitalWrite(CONFIG_LED_TIC_RED_PIN, LOW);
}

/**
 * Setup function.
 * Called once MySensors has successfully initialized.
 */
void setup(void) {

    /* Setup serial port to computer */
    Serial.begin(115200);
    Serial.println(" [i] Hello world.");

    /* Setup tic reader */
    m_tic_reader.setup(m_tic_port);
}

/**
 * MySensors function called to describe this sensor and its capabilites.
 */
void presentation(void) {
    int res = 1;
    do {
        res &= sendSketchInfo(F("SLHA00011 Linky"), F("0.2.0"));
        res &= present(SENSOR_0_SERIAL_NUMBER, S_INFO, F("Numéro de Série"));                      // V_TEXT (ADCO, ADSC)
        res &= present(SENSOR_1_INDEX_BASE, S_POWER, F("Index Base"));                             // V_KWH (BASE)
        res &= present(SENSOR_2_POWER_APPARENT, S_POWER, F("Puissance Apparente"));                // V_WATT (PAPP)
        res &= present(SENSOR_3_MULTIMETER_PHASE_1, S_MULTIMETER, F("Phase 1"));                   // V_VOLTAGE (URMS1) and V_CURRENT (IINST, IINST1, IRMS1)
        res &= present(SENSOR_4_MULTIMETER_PHASE_2, S_MULTIMETER, F("Phase 2"));                   // V_VOLTAGE (URMS2) and V_CURRENT (IINST2, IRMS2)
        res &= present(SENSOR_5_MULTIMETER_PHASE_3, S_MULTIMETER, F("Phase 3"));                   // V_VOLTAGE (URMS3) and V_CURRENT (IINST3, IRMS3)
        res &= present(SENSOR_6_PRICING, S_INFO, F("Option tarifaire"));                           // V_TEXT
        res &= present(SENSOR_7_PRICING_TEMPO_COLOR_TOMORROW, S_INFO, F("Tempo couleur demain"));  // V_TEXT
    } while (res == 0);
}

/**
 * MySensors function called when a message is received.
 */
void receive(const MyMessage &message) {

    /* For now we ignore the received message */
    (void)message;
}

/**
 * Main loop.
 */
void loop(void) {
    int res;

    /* Led task */
    {
        static uint32_t m_led_timestamp = 0;
        static enum {
            STATE_0,
            STATE_1,
            STATE_2,
            STATE_3,
            STATE_4,
            STATE_5,
            STATE_6,
            STATE_7,
            STATE_8,
            STATE_9,
        } m_led_sm;
        switch (m_led_sm) {
            case STATE_0: {
                if (m_tic_state == STATE_STARTING) {
                    m_led_sm = STATE_1;
                } else if (m_tic_state == STATE_VALID) {
                    m_led_sm = STATE_4;
                } else {
                    m_led_sm = STATE_7;
                }
                break;
            }
            case STATE_1: {
                digitalWrite(CONFIG_LED_TIC_RED_PIN, HIGH);
                digitalWrite(CONFIG_LED_TIC_GREEN_PIN, HIGH);
                m_led_timestamp = millis();
                m_led_sm = STATE_2;
                break;
            }
            case STATE_2: {
                if (millis() - m_led_timestamp >= 100) {
                    digitalWrite(CONFIG_LED_TIC_RED_PIN, LOW);
                    digitalWrite(CONFIG_LED_TIC_GREEN_PIN, LOW);
                    m_led_sm = STATE_3;
                }
                break;
            }
            case STATE_3: {
                if (millis() - m_led_timestamp >= 1000) {
                    m_led_sm = STATE_0;
                }
                break;
            }
            case STATE_4: {
                digitalWrite(CONFIG_LED_TIC_GREEN_PIN, HIGH);
                digitalWrite(CONFIG_LED_TIC_RED_PIN, LOW);
                m_led_timestamp = millis();
                m_led_sm = STATE_5;
                break;
            }
            case STATE_5: {
                if (millis() - m_led_timestamp >= 100) {
                    digitalWrite(CONFIG_LED_TIC_RED_PIN, LOW);
                    digitalWrite(CONFIG_LED_TIC_GREEN_PIN, LOW);
                    m_led_sm = STATE_6;
                }
                break;
            }
            case STATE_6: {
                if (millis() - m_led_timestamp >= 10000) {
                    m_led_sm = STATE_0;
                }
                break;
            }
            case STATE_7: {
                digitalWrite(CONFIG_LED_TIC_GREEN_PIN, LOW);
                digitalWrite(CONFIG_LED_TIC_RED_PIN, HIGH);
                m_led_timestamp = millis();
                m_led_sm = STATE_5;
                break;
            }
            case STATE_8: {
                if (millis() - m_led_timestamp >= 100) {
                    digitalWrite(CONFIG_LED_TIC_RED_PIN, LOW);
                    digitalWrite(CONFIG_LED_TIC_GREEN_PIN, LOW);
                    m_led_sm = STATE_6;
                }
                break;
            }
            case STATE_9: {
                if (millis() - m_led_timestamp >= 1000) {
                    m_led_sm = STATE_0;
                }
                break;
            }
        }
    }

    /* Tic reading task */
    {
        static enum {
            STATE_0,
            STATE_1,
        } m_tic_sm;
        switch (m_tic_sm) {

            case STATE_0: {

                /* Automatically detect baud rate at which linky meter sends the data, as it can use:
                 * - either 1200 for historic (most common),
                 * - or 9600 for standard (required when producing elecriticity) */
                pinMode(CONFIG_TIC_DATA_PIN, INPUT);
                uint32_t period_us_min = UINT32_MAX;
                for (uint8_t i = 0; i < 10; i++) {

                    /* Wait for pin to be high (uart idle state) */
                    while (digitalRead(CONFIG_TIC_DATA_PIN) == 1) {
                    }

                    /* Once it is high, measure the amount of time it goes low */
                    uint32_t period_us = pulseIn(CONFIG_TIC_DATA_PIN, LOW);
                    if (period_us < period_us_min) {
                        period_us_min = period_us;
                    }
                }

                /* Convert minimal period to frequency */
                if (period_us_min >= 666 && period_us_min <= 1000) {
                    Serial.println(" [i] Detected baudrate of 1200");
                    m_tic_port_baudrate = 1200;
                    m_tic_port.begin(m_tic_port_baudrate);
                    m_tic_sm = STATE_1;
                } else if (period_us_min >= 83 && period_us_min <= 125) {
                    Serial.println(" [i] Detected baudrate of 9600");
                    m_tic_port_baudrate = 9600;
                    m_tic_port.begin(m_tic_port_baudrate);
                    m_tic_sm = STATE_1;
                } else {
                    Serial.println(" [e] Failed to detect baudrate!");
                    m_tic_state = STATE_INVALID;
                }
                break;
            }

            case STATE_1: {

                /* Read incoming datasets */
                struct tic_dataset dataset = {0};
                res = m_tic_reader.read(dataset);
                if (res < 0) {
                    Serial.println(" [e] Tic error!");
                    m_tic_state = STATE_INVALID;
                    m_tic_sm = STATE_0;
                    break;
                } else if (res == 0) {
                    break;
                }

                Serial.printf(" [d] Received dataset %s = %s\r\n", dataset.name, dataset.data);
                m_tic_state = STATE_VALID;

                /* Serial number */
                if (strcmp_P(dataset.name, PSTR("ADCO")) == 0 || strcmp_P(dataset.name, PSTR("ADSC")) == 0) {
                    static char value_last[12 + 1];
                    if (strcmp(dataset.data, value_last) != 0) {
                        MyMessage message(SENSOR_0_SERIAL_NUMBER, V_TEXT);
                        if (send(message.set(dataset.data)) == true) {
                            strncpy(value_last, dataset.data, 12);
                        }
                    }
                }

                /* Option tarifaire choisie */
                else if (strcmp_P(dataset.name, PSTR("OPTARIF")) == 0) {
                    static char value_last[4 + 1];
                    if (strcmp(dataset.data, value_last) != 0) {
                        MyMessage message(SENSOR_6_PRICING, V_TEXT);
                        if (send(message.set(dataset.data)) == true) {
                            strncpy(value_last, dataset.data, 4);
                        }
                    }
                }

                /* Couleur du lendemain pour les options tarifaires tempo */
                else if (strcmp_P(dataset.name, PSTR("DEMAIN")) == 0) {
                    static char value_last[4 + 1];
                    if (strcmp(dataset.data, value_last) != 0) {
                        MyMessage message(SENSOR_7_PRICING_TEMPO_COLOR_TOMORROW, V_TEXT);
                        if (send(message.set(dataset.data)) == true) {
                            strncpy(value_last, dataset.data, 4);
                        }
                    }
                }

                /* Index for base */
                else if (strcmp_P(dataset.name, PSTR("BASE")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_1_INDEX_BASE, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Puissance apparente */
                else if (strcmp_P(dataset.name, PSTR("PAPP")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_2_POWER_APPARENT, V_WATT);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Intensité Phase 1 */
                else if (strcmp_P(dataset.name, PSTR("IINST")) == 0 || strcmp_P(dataset.name, PSTR("IINST1")) == 0 || strcmp_P(dataset.name, PSTR("IRMS1")) == 0) {
                    static uint8_t value_last = 0;
                    uint8_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_3_MULTIMETER_PHASE_1, V_CURRENT);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Tension Phase 1 */
                else if (strcmp_P(dataset.name, PSTR("URMS1")) == 0) {
                    static uint16_t value_last = 0;
                    uint16_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_3_MULTIMETER_PHASE_1, V_VOLTAGE);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Intensité Phase 2 */
                else if (strcmp_P(dataset.name, PSTR("IINST2")) == 0 || strcmp_P(dataset.name, PSTR("IRMS2")) == 0) {
                    static uint8_t value_last = 0;
                    uint8_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_4_MULTIMETER_PHASE_2, V_CURRENT);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Tension Phase 2 */
                else if (strcmp_P(dataset.name, PSTR("URMS2")) == 0) {
                    static uint16_t value_last = 0;
                    uint16_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_4_MULTIMETER_PHASE_2, V_VOLTAGE);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Intensité Phase 3 */
                else if (strcmp_P(dataset.name, PSTR("IINST3")) == 0 || strcmp_P(dataset.name, PSTR("IRMS3")) == 0) {
                    static uint8_t value_last = 0;
                    uint8_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_5_MULTIMETER_PHASE_3, V_CURRENT);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Tension Phase 3 */
                else if (strcmp_P(dataset.name, PSTR("URMS3")) == 0) {
                    static uint16_t value_last = 0;
                    uint16_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_5_MULTIMETER_PHASE_3, V_VOLTAGE);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                break;
            }
        }
    }
}
