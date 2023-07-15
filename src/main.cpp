/* Config */
#include "../cfg/config.h"

/* Project code */
#include "tic.h"

/* Arduino Libraries */
#include <Arduino.h>
#include <MySensors.h>

/* C/C++ libraries */
#include <ctype.h>
#include <stdlib.h>

/* Working variables */
static tic m_tic;

/* List of virtual sensors */
enum {
    SENSOR_0_SERIAL_NUMBER,       // V_TEXT
    SENSOR_1_INDEX_BASE,          // V_KWH
    SENSOR_2_POWER_APPARENT,      // V_WATT
    SENSOR_3_MULTIMETER_PHASE_1,  // V_VOLTAGE and V_CURRENT
    SENSOR_4_MULTIMETER_PHASE_2,  // V_VOLTAGE and V_CURRENT
    SENSOR_5_MULTIMETER_PHASE_3,  // V_VOLTAGE and V_CURRENT
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
 * Called when setup() encounters an error.
 */
void setup_failed(const char *const reason) {
    Serial.println(reason);
    Serial.flush();
    while (1) {
        // sleep(0, false);
    }
}

/**
 * Setup function.
 * Called once MySensors has successfully initialized.
 */
void setup(void) {
    int res;

    /* Setup serial */
    Serial.begin(115200);
    Serial.println(" [i] Hello world.");

    /* Setup tic */
    res = 0;
    res |= m_tic.setup(CONFIG_TIC_DATA_PIN, CONFIG_TIC_DUMMY_PIN);
    res |= m_tic.begin();
    if (res < 0) {
        setup_failed(" [e] Failed to communicate with linky!");
    }
}

/**
 * MySensors function called to describe this sensor and its capabilites.
 */
void presentation(void) {
    int res = 1;
    do {
        res &= sendSketchInfo(F("SLHA00011 Linky"), F("0.1.1"));
        res &= present(SENSOR_0_SERIAL_NUMBER, S_INFO, F("Numéro de Série"));        // V_TEXT (ADCO, ADSC)
        res &= present(SENSOR_1_INDEX_BASE, S_POWER, F("Index Base"));               // V_KWH (BASE)
        res &= present(SENSOR_2_POWER_APPARENT, S_POWER, F("Puissance Apparente"));  // V_WATT (PAPP)
        res &= present(SENSOR_3_MULTIMETER_PHASE_1, S_MULTIMETER, F("Phase 1"));     // V_VOLTAGE (URMS1) and V_CURRENT (IINST, IINST1, IRMS1)
        res &= present(SENSOR_4_MULTIMETER_PHASE_2, S_MULTIMETER, F("Phase 2"));     // V_VOLTAGE (URMS2) and V_CURRENT (IINST2, IRMS2)
        res &= present(SENSOR_5_MULTIMETER_PHASE_3, S_MULTIMETER, F("Phase 3"));     // V_VOLTAGE (URMS3) and V_CURRENT (IINST3, IRMS3)
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
    static bool tic_valid = false;
    static uint32_t m_led_timestamp = 0;
    static enum {
        STATE_0,
        STATE_1,
        STATE_2,
        STATE_3,
        STATE_4,
        STATE_5,
        STATE_6,
    } m_led_sm;
    switch (m_led_sm) {
        case STATE_0: {
            if (tic_valid) {
                m_led_sm = STATE_1;
            } else {
                m_led_sm = STATE_4;
            }
            break;
        }
        case STATE_1: {
            digitalWrite(CONFIG_LED_TIC_RED_PIN, LOW);
            digitalWrite(CONFIG_LED_TIC_GREEN_PIN, HIGH);
            m_led_timestamp = millis();
            m_led_sm = STATE_2;
            break;
        }
        case STATE_2: {
            if (millis() - m_led_timestamp >= 100) {
                digitalWrite(CONFIG_LED_TIC_GREEN_PIN, LOW);
                m_led_sm = STATE_3;
            }
            break;
        }
        case STATE_3: {
            if (millis() - m_led_timestamp >= 3000) {
                m_led_sm = STATE_0;
            }
            break;
        }
        case STATE_4: {
            digitalWrite(CONFIG_LED_TIC_GREEN_PIN, LOW);
            digitalWrite(CONFIG_LED_TIC_RED_PIN, HIGH);
            m_led_timestamp = millis();
            m_led_sm = STATE_5;
            break;
        }
        case STATE_5: {
            if (millis() - m_led_timestamp >= 100) {
                digitalWrite(CONFIG_LED_TIC_RED_PIN, LOW);
                m_led_sm = STATE_6;
            }
            break;
        }
        case STATE_6: {
            if (millis() - m_led_timestamp >= 1000) {
                m_led_sm = STATE_0;
            }
            break;
        }
    }

    /* Tic task */
    struct tic::dataset dataset = {0};
    res = m_tic.dataset_get(dataset);
    if (res < 0) {
        Serial.println(" [e] Tic error!");
        tic_valid = false;
    } else if (res == 1) {
        Serial.printf(" [d] Received dataset %s = %s\r\n", dataset.name, dataset.data);
        tic_valid = true;

        /* Serial number */
        if (strcmp_P(dataset.name, PSTR("ADCO")) == 0 || strcmp_P(dataset.name, PSTR("ADSC")) == 0) {
            static bool initial_sent = false;
            if (initial_sent == false) {
                MyMessage message(SENSOR_0_SERIAL_NUMBER, V_TEXT);
                if (send(message.set(dataset.data)) == true) {
                    initial_sent = true;
                }
            }
        }

        /* Index for base */
        else if (strcmp_P(dataset.name, PSTR("BASE")) == 0) {
            static bool initial_sent = false;
            static uint32_t base_wh_last = 0;
            uint32_t base_wh = 0;
            for (size_t i = 0; i < 9; i++) {
                if (dataset.data[i] >= '0' && dataset.data[i] <= '9') {
                    base_wh *= 10;
                    base_wh += dataset.data[i] - '0';
                } else {
                    break;
                }
            }
            if (base_wh > base_wh_last || initial_sent == false) {
                MyMessage message(SENSOR_1_INDEX_BASE, V_KWH);
                if (send(message.set(base_wh / 1000.0, 3)) == true) {
                    initial_sent = true;
                    base_wh_last = base_wh;
                }
            }
        }

        /* Puissance apparente */
        else if (strcmp_P(dataset.name, PSTR("PAPP")) == 0) {
            static bool initial_sent = false;
            static uint32_t power_va_last = 0;
            uint32_t power_va = 0;
            for (size_t i = 0; i < 5; i++) {
                if (dataset.data[i] >= '0' && dataset.data[i] <= '9') {
                    power_va *= 10;
                    power_va += dataset.data[i] - '0';
                } else {
                    break;
                }
            }
            if (power_va != power_va_last || initial_sent == false) {
                MyMessage message(SENSOR_2_POWER_APPARENT, V_WATT);
                if (send(message.set(power_va)) == true) {
                    initial_sent = true;
                    power_va_last = power_va;
                }
            }
        }

        /* Intensité Phase 1 */
        else if (strcmp_P(dataset.name, PSTR("IINST")) == 0 || strcmp_P(dataset.name, PSTR("IINST1")) == 0 || strcmp_P(dataset.name, PSTR("IRMS1")) == 0) {
            static bool initial_sent = false;
            static uint32_t current_a_last = 0;
            uint32_t current_a = 0;
            for (size_t i = 0; i < 3; i++) {
                if (dataset.data[i] >= '0' && dataset.data[i] <= '9') {
                    current_a *= 10;
                    current_a += dataset.data[i] - '0';
                } else {
                    break;
                }
            }
            if (current_a != current_a_last || initial_sent == false) {
                MyMessage message(SENSOR_3_MULTIMETER_PHASE_1, V_CURRENT);
                if (send(message.set(current_a)) == true) {
                    initial_sent = true;
                    current_a_last = current_a;
                }
            }
        }

        /* Tension Phase 1 */
        else if (strcmp_P(dataset.name, PSTR("URMS1")) == 0) {
            static bool initial_sent = false;
            static uint16_t voltage_v_last = 0;
            uint16_t voltage_a = 0;
            for (size_t i = 0; i < 3; i++) {
                if (dataset.data[i] >= '0' && dataset.data[i] <= '9') {
                    voltage_a *= 10;
                    voltage_a += dataset.data[i] - '0';
                } else {
                    break;
                }
            }
            if (voltage_a != voltage_v_last || initial_sent == false) {
                MyMessage message(SENSOR_3_MULTIMETER_PHASE_1, V_VOLTAGE);
                if (send(message.set(voltage_a)) == true) {
                    initial_sent = true;
                    voltage_v_last = voltage_a;
                }
            }
        }

        /* Intensité Phase 2 */
        else if (strcmp_P(dataset.name, PSTR("IINST2")) == 0 || strcmp_P(dataset.name, PSTR("IRMS2")) == 0) {
            static bool initial_sent = false;
            static uint32_t current_a_last = 0;
            uint32_t current_a = 0;
            for (size_t i = 0; i < 3; i++) {
                if (dataset.data[i] >= '0' && dataset.data[i] <= '9') {
                    current_a *= 10;
                    current_a += dataset.data[i] - '0';
                } else {
                    break;
                }
            }
            if (current_a != current_a_last || initial_sent == false) {
                MyMessage message(SENSOR_4_MULTIMETER_PHASE_2, V_CURRENT);
                if (send(message.set(current_a)) == true) {
                    initial_sent = true;
                    current_a_last = current_a;
                }
            }
        }

        /* Tension Phase 2 */
        else if (strcmp_P(dataset.name, PSTR("URMS2")) == 0) {
            static bool initial_sent = false;
            static uint16_t voltage_v_last = 0;
            uint16_t voltage_a = 0;
            for (size_t i = 0; i < 3; i++) {
                if (dataset.data[i] >= '0' && dataset.data[i] <= '9') {
                    voltage_a *= 10;
                    voltage_a += dataset.data[i] - '0';
                } else {
                    break;
                }
            }
            if (voltage_a != voltage_v_last || initial_sent == false) {
                MyMessage message(SENSOR_4_MULTIMETER_PHASE_2, V_VOLTAGE);
                if (send(message.set(voltage_a)) == true) {
                    initial_sent = true;
                    voltage_v_last = voltage_a;
                }
            }
        }

        /* Intensité Phase 3 */
        else if (strcmp_P(dataset.name, PSTR("IINST3")) == 0 || strcmp_P(dataset.name, PSTR("IRMS3")) == 0) {
            static bool initial_sent = false;
            static uint32_t current_a_last = 0;
            uint32_t current_a = 0;
            for (size_t i = 0; i < 3; i++) {
                if (dataset.data[i] >= '0' && dataset.data[i] <= '9') {
                    current_a *= 10;
                    current_a += dataset.data[i] - '0';
                } else {
                    break;
                }
            }
            if (current_a != current_a_last || initial_sent == false) {
                MyMessage message(SENSOR_5_MULTIMETER_PHASE_3, V_CURRENT);
                if (send(message.set(current_a)) == true) {
                    initial_sent = true;
                    current_a_last = current_a;
                }
            }
        }

        /* Tension Phase 3 */
        else if (strcmp_P(dataset.name, PSTR("URMS3")) == 0) {
            static bool initial_sent = false;
            static uint16_t voltage_v_last = 0;
            uint16_t voltage_a = 0;
            for (size_t i = 0; i < 3; i++) {
                if (dataset.data[i] >= '0' && dataset.data[i] <= '9') {
                    voltage_a *= 10;
                    voltage_a += dataset.data[i] - '0';
                } else {
                    break;
                }
            }
            if (voltage_a != voltage_v_last || initial_sent == false) {
                MyMessage message(SENSOR_5_MULTIMETER_PHASE_3, V_VOLTAGE);
                if (send(message.set(voltage_a)) == true) {
                    initial_sent = true;
                    voltage_v_last = voltage_a;
                }
            }
        }
    }
}
