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
    SENSOR_0_SERIAL_NUMBER,                   // S_INFO (V_TEXT)
    SENSOR_1_MULTIMETER_PHASE_1,              // S_MULTIMETER (V_VOLTAGE and V_CURRENT)
    SENSOR_2_MULTIMETER_PHASE_2,              // S_MULTIMETER (V_VOLTAGE and V_CURRENT)
    SENSOR_3_MULTIMETER_PHASE_3,              // S_MULTIMETER (V_VOLTAGE and V_CURRENT)
    SENSOR_4_POWER_APPARENT,                  // S_POWER (V_WATT)
    SENSOR_5_CONTRACT_NAME,                   // S_INFO (V_TEXT)
    SENSOR_6_CONTRACT_CURRENT,                // S_MULTIMETER (V_CURRENT)
    SENSOR_7_CONTRACT_PERIOD,                 // S_INFO (V_TEXT)
    SENSOR_8_CONTRACT_BASE_INDEX,             // S_POWER (V_KWH)
    SENSOR_9_CONTRACT_HC_INDEX_HC,            // S_POWER (V_KWH)
    SENSOR_10_CONTRACT_HC_INDEX_HP,           // S_POWER (V_KWH)
    SENSOR_11_CONTRACT_EJP_INDEX_HN,          // S_POWER (V_KWH)
    SENSOR_12_CONTRACT_EJP_INDEX_HPM,         // S_POWER (V_KWH)
    SENSOR_13_CONTRACT_EJP_NOTICE,            // S_INFO (V_TEXT)
    SENSOR_14_CONTRACT_TEMPO_INDEX_BLUE_PK,   // S_POWER (V_KWH)
    SENSOR_15_CONTRACT_TEMPO_INDEX_BLUE_OK,   // S_POWER (V_KWH)
    SENSOR_16_CONTRACT_TEMPO_INDEX_WHITE_PK,  // S_POWER (V_KWH)
    SENSOR_17_CONTRACT_TEMPO_INDEX_WHITE_OK,  // S_POWER (V_KWH)
    SENSOR_18_CONTRACT_TEMPO_INDEX_RED_PK,    // S_POWER (V_KWH)
    SENSOR_19_CONTRACT_TEMPO_INDEX_RED_OK,    // S_POWER (V_KWH)
    SENSOR_20_CONTRACT_TEMPO_TOMORROW,        // S_INFO (V_TEXT)
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
    /* Because messages might be lost,
     * we're not doing the presentation in one block, but rather step by step,
     * making sure each step is sucessful before advancing to the next */
    for (int8_t step = -1;;) {

        /* Send out presentation information corresponding to the current step,
         * and advance one step if successful */
        switch (step) {
            case -1: {
                if (sendSketchInfo(F("SLHA00011 Linky"), F("1.0.0")) == true) {
                    step++;
                }
                break;
            }
            case SENSOR_0_SERIAL_NUMBER: {
                if (present(SENSOR_0_SERIAL_NUMBER, S_INFO, F("Numéro de Série")) == true) {  // V_TEXT (ADCO, ADSC)
                    step++;
                }
                break;
            }
            case SENSOR_1_MULTIMETER_PHASE_1: {
                if (present(SENSOR_1_MULTIMETER_PHASE_1, S_MULTIMETER, F("Phase 1")) == true) {  // V_VOLTAGE (URMS1) and V_CURRENT (IINST, IINST1, IRMS1)
                    step++;
                }
                break;
            }
            case SENSOR_2_MULTIMETER_PHASE_2: {
                if (present(SENSOR_2_MULTIMETER_PHASE_2, S_MULTIMETER, F("Phase 2")) == true) {  // V_VOLTAGE (URMS2) and V_CURRENT (IINST2, IRMS2)
                    step++;
                }
                break;
            }
            case SENSOR_3_MULTIMETER_PHASE_3: {
                if (present(SENSOR_3_MULTIMETER_PHASE_3, S_MULTIMETER, F("Phase 3")) == true) {  // V_VOLTAGE (URMS3) and V_CURRENT (IINST3, IRMS3)
                    step++;
                }
                break;
            }
            case SENSOR_4_POWER_APPARENT: {
                if (present(SENSOR_4_POWER_APPARENT, S_POWER, F("Puissance Apparente")) == true) {  // V_WATT (PAPP)
                    step++;
                }
                break;
            }
            case SENSOR_5_CONTRACT_NAME: {
                if (present(SENSOR_5_CONTRACT_NAME, S_INFO, F("Option Tarifaire")) == true) {  // V_TEXT (OPTARIF)
                    step++;
                }
                break;
            }
            case SENSOR_6_CONTRACT_CURRENT: {
                if (present(SENSOR_6_CONTRACT_CURRENT, S_MULTIMETER, F("Intensité Souscrite")) == true) {  // V_CURRENT (ISOUSC)
                    step++;
                }
                break;
            }
            case SENSOR_7_CONTRACT_PERIOD: {
                if (present(SENSOR_7_CONTRACT_PERIOD, S_INFO, F("Période Tarifaire")) == true) {  // V_TEXT (PTEC)
                    step++;
                }
                break;
            }
            case SENSOR_8_CONTRACT_BASE_INDEX: {
                if (present(SENSOR_8_CONTRACT_BASE_INDEX, S_POWER, F("Opt Base Index")) == true) {  // V_KWH (BASE)
                    step++;
                }
                break;
            }
            case SENSOR_9_CONTRACT_HC_INDEX_HC: {
                if (present(SENSOR_9_CONTRACT_HC_INDEX_HC, S_POWER, F("Opt HC Index HC")) == true) {  // V_KWH (HCHC)
                    step++;
                }
                break;
            }
            case SENSOR_10_CONTRACT_HC_INDEX_HP: {
                if (present(SENSOR_10_CONTRACT_HC_INDEX_HP, S_POWER, F("Opt HC Index HP")) == true) {  // V_KWH (HCHP)
                    step++;
                }
                break;
            }
            case SENSOR_11_CONTRACT_EJP_INDEX_HN: {
                if (present(SENSOR_11_CONTRACT_EJP_INDEX_HN, S_POWER, F("Opt EJP Index HN")) == true) {  // V_KWH (EJPHN)
                    step++;
                }
                break;
            }
            case SENSOR_12_CONTRACT_EJP_INDEX_HPM: {
                if (present(SENSOR_12_CONTRACT_EJP_INDEX_HPM, S_POWER, F("Opt EJP Index HPM")) == true) {  // V_KWH (EJPHPM)
                    step++;
                }
                break;
            }
            case SENSOR_13_CONTRACT_EJP_NOTICE: {
                if (present(SENSOR_13_CONTRACT_EJP_NOTICE, S_INFO, F("Opt EJP Préavis")) == true) {  // V_TEXT (PEJP)
                    step++;
                }
                break;
            }
            case SENSOR_15_CONTRACT_TEMPO_INDEX_BLUE_OK: {
                if (present(SENSOR_15_CONTRACT_TEMPO_INDEX_BLUE_OK, S_POWER, F("Opt Tempo Index Bleu HC")) == true) {  // V_KWH (BBRHCJB)
                    step++;
                }
                break;
            }
            case SENSOR_14_CONTRACT_TEMPO_INDEX_BLUE_PK: {
                if (present(SENSOR_14_CONTRACT_TEMPO_INDEX_BLUE_PK, S_POWER, F("Opt Tempo Index Bleu HP")) == true) {  // V_KWH (BBRHPJB)
                    step++;
                }
                break;
            }
            case SENSOR_17_CONTRACT_TEMPO_INDEX_WHITE_OK: {
                if (present(SENSOR_17_CONTRACT_TEMPO_INDEX_WHITE_OK, S_POWER, F("Opt Tempo Index Blanc HC")) == true) {  // V_KWH (BBRHCJW)
                    step++;
                }
                break;
            }
            case SENSOR_16_CONTRACT_TEMPO_INDEX_WHITE_PK: {
                if (present(SENSOR_16_CONTRACT_TEMPO_INDEX_WHITE_PK, S_POWER, F("Opt Tempo Index Blanc HP")) == true) {  // V_KWH (BBRHPJW)
                    step++;
                }
                break;
            }
            case SENSOR_19_CONTRACT_TEMPO_INDEX_RED_OK: {
                if (present(SENSOR_19_CONTRACT_TEMPO_INDEX_RED_OK, S_POWER, F("Opt Tempo Index Rouge HC")) == true) {  // V_KWH (BBRHCJR)
                    step++;
                }
                break;
            }
            case SENSOR_18_CONTRACT_TEMPO_INDEX_RED_PK: {
                if (present(SENSOR_18_CONTRACT_TEMPO_INDEX_RED_PK, S_POWER, F("Opt Tempo Index Rouge HP")) == true) {  // V_KWH (BBRHPJR)
                    step++;
                }
                break;
            }
            case SENSOR_20_CONTRACT_TEMPO_TOMORROW: {
                if (present(SENSOR_20_CONTRACT_TEMPO_TOMORROW, S_INFO, F("Opt Tempo Couleur Demain")) == true) {  // V_TEXT (DEMAIN)
                    step++;
                }
                break;
            }
            default: {
                return;
            }
        }

        /* Sleep a little bit after each presentation, otherwise the next fails
         * @see https://forum.mysensors.org/topic/4450/sensor-presentation-failure */
        sleep(50);
    }
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
                m_led_sm = STATE_8;
                break;
            }
            case STATE_8: {
                if (millis() - m_led_timestamp >= 100) {
                    digitalWrite(CONFIG_LED_TIC_RED_PIN, LOW);
                    digitalWrite(CONFIG_LED_TIC_GREEN_PIN, LOW);
                    m_led_sm = STATE_9;
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
                m_tic_port.end();
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

                /* Numéro de Série */
                if (strcmp_P(dataset.name, PSTR("ADCO")) == 0 ||  //
                    strcmp_P(dataset.name, PSTR("ADSC")) == 0) {
                    static char value_last[12 + 1];
                    if (strcmp(dataset.data, value_last) != 0) {
                        MyMessage message(SENSOR_0_SERIAL_NUMBER, V_TEXT);
                        if (send(message.set(dataset.data)) == true) {
                            strncpy(value_last, dataset.data, 12);
                        }
                    }
                }

                /* Intensité Phase 1 */
                else if (strcmp_P(dataset.name, PSTR("IINST")) == 0 ||   //
                         strcmp_P(dataset.name, PSTR("IINST1")) == 0 ||  //
                         strcmp_P(dataset.name, PSTR("IRMS1")) == 0) {
                    static uint8_t value_last = 0;
                    uint8_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_1_MULTIMETER_PHASE_1, V_CURRENT);
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
                        MyMessage message(SENSOR_1_MULTIMETER_PHASE_1, V_VOLTAGE);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Intensité Phase 2 */
                else if (strcmp_P(dataset.name, PSTR("IINST2")) == 0 ||  //
                         strcmp_P(dataset.name, PSTR("IRMS2")) == 0) {
                    static uint8_t value_last = 0;
                    uint8_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_2_MULTIMETER_PHASE_2, V_CURRENT);
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
                        MyMessage message(SENSOR_2_MULTIMETER_PHASE_2, V_VOLTAGE);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Intensité Phase 3 */
                else if (strcmp_P(dataset.name, PSTR("IINST3")) == 0 ||  //
                         strcmp_P(dataset.name, PSTR("IRMS3")) == 0) {
                    static uint8_t value_last = 0;
                    uint8_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_3_MULTIMETER_PHASE_3, V_CURRENT);
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
                        MyMessage message(SENSOR_3_MULTIMETER_PHASE_3, V_VOLTAGE);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Puissance apparente */
                else if (strcmp_P(dataset.name, PSTR("PAPP")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_4_POWER_APPARENT, V_WATT);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option tarifaire choisie */
                else if (strcmp_P(dataset.name, PSTR("OPTARIF")) == 0) {
                    static char value_last[4 + 1];
                    if (strcmp(dataset.data, value_last) != 0) {
                        MyMessage message(SENSOR_5_CONTRACT_NAME, V_TEXT);
                        if (send(message.set(dataset.data)) == true) {
                            strncpy(value_last, dataset.data, 4);
                        }
                    }
                }

                /* Intensité Souscrite */
                else if (strcmp_P(dataset.name, PSTR("ISOUSC")) == 0) {
                    static uint8_t value_last = 0;
                    uint8_t value = strtol(dataset.data, NULL, 10);
                    if (value != value_last) {
                        MyMessage message(SENSOR_6_CONTRACT_CURRENT, V_CURRENT);
                        if (send(message.set(value)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Période tarifaire en cours */
                else if (strcmp_P(dataset.name, PSTR("PTEC")) == 0) {
                    static char value_last[4 + 1];
                    if (strcmp(dataset.data, value_last) != 0) {
                        MyMessage message(SENSOR_7_CONTRACT_PERIOD, V_TEXT);
                        if (send(message.set(dataset.data)) == true) {
                            strncpy(value_last, dataset.data, 4);
                        }
                    }
                }

                /* Option Base, index */
                else if (strcmp_P(dataset.name, PSTR("BASE")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_8_CONTRACT_BASE_INDEX, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option HC, index HC */
                else if (strcmp_P(dataset.name, PSTR("HCHC")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_9_CONTRACT_HC_INDEX_HC, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option HC, index HP */
                else if (strcmp_P(dataset.name, PSTR("HCHP")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_10_CONTRACT_HC_INDEX_HP, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option EJP, index heures normales */
                else if (strcmp_P(dataset.name, PSTR("EJPHN")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_11_CONTRACT_EJP_INDEX_HN, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option EJP, index heures de pointe mobile */
                else if (strcmp_P(dataset.name, PSTR("EJPHPM")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_12_CONTRACT_EJP_INDEX_HPM, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option EJP, préavis de début */
                else if (strcmp_P(dataset.name, PSTR("PEJP")) == 0) {
                    static char value_last[2 + 1];
                    if (strcmp(dataset.data, value_last) != 0) {
                        MyMessage message(SENSOR_13_CONTRACT_EJP_NOTICE, V_TEXT);
                        if (send(message.set(dataset.data)) == true) {
                            strncpy(value_last, dataset.data, 2);
                        }
                    }
                }

                /* Option Tempo, index bleu HC */
                else if (strcmp_P(dataset.name, PSTR("BBRHCJB")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_15_CONTRACT_TEMPO_INDEX_BLUE_OK, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option Tempo, index bleu HP */
                else if (strcmp_P(dataset.name, PSTR("BBRHPJB")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_14_CONTRACT_TEMPO_INDEX_BLUE_PK, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option Tempo, index blanc HC */
                else if (strcmp_P(dataset.name, PSTR("BBRHCJW")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_17_CONTRACT_TEMPO_INDEX_WHITE_OK, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option Tempo, index blanc HP */
                else if (strcmp_P(dataset.name, PSTR("BBRHPJW")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_16_CONTRACT_TEMPO_INDEX_WHITE_PK, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option Tempo, index rouge HC */
                else if (strcmp_P(dataset.name, PSTR("BBRHCJR")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_19_CONTRACT_TEMPO_INDEX_RED_OK, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option Tempo, index rouge HP */
                else if (strcmp_P(dataset.name, PSTR("BBRHPJR")) == 0) {
                    static uint32_t value_last = 0;
                    uint32_t value = strtol(dataset.data, NULL, 10);
                    if (value > value_last) {
                        MyMessage message(SENSOR_18_CONTRACT_TEMPO_INDEX_RED_PK, V_KWH);
                        if (send(message.set(value / 1000.0, 3)) == true) {
                            value_last = value;
                        }
                    }
                }

                /* Option Tempo, couleur du lendemain */
                else if (strcmp_P(dataset.name, PSTR("DEMAIN")) == 0) {
                    static char value_last[4 + 1];
                    if (strcmp(dataset.data, value_last) != 0) {
                        MyMessage message(SENSOR_20_CONTRACT_TEMPO_TOMORROW, V_TEXT);
                        if (send(message.set(dataset.data)) == true) {
                            strncpy(value_last, dataset.data, 4);
                        }
                    }
                }

                break;
            }
        }
    }
}
