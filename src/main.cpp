/* Config */
#include "../cfg/config.h"

/* Project code */
#include "linky_tic.h"

/* Arduino Libraries */
#include <Arduino.h>
#include <MySensors.h>

/* C/C++ libraries */
#include <ctype.h>
#include <stdlib.h>

/* Working variables */
static linky_tic m_linky;
static char m_linky_serial[12 + 1] = "";

/* Wireless messages */
static MyMessage m_message_info_serial(0, V_TEXT);
static MyMessage m_message_power_watt(1, V_WATT);
static MyMessage m_message_power_kwh(1, V_KWH);
static MyMessage m_message_multimeter_voltage(2, V_VOLTAGE);
static MyMessage m_message_multimeter_current(2, V_CURRENT);

/**
 *
 */
void setup_failed(const char *const reason) {
    Serial.println(reason);
    Serial.flush();
    while (1) {
        // sleep(0, false);
    }
}

/**
 *
 */
void setup() {
    int res;

    /* Setup serial */
    Serial.begin(115200);
    Serial.println(" [i] Hello world.");

    /* Setup linky */
    res = 0;
    res |= m_linky.setup(2, 3);
    res |= m_linky.begin();
    if (res < 0) {
        setup_failed(" [e] Failed to communicate with linky!");
    }
}

/**
 * MySensors function called to describe this sensor and its capabilites.
 */
void presentation() {
    sendSketchInfo("SLHA00011 Linky", "0.1.0");
    present(0, S_INFO);        // V_TEXT
    present(1, S_POWER);       // Power measuring device, like power meters: V_WATT, V_KWH
    present(2, S_MULTIMETER);  // Multimeter device:                         V_VOLTAGE, V_CURRENT, V_IMPEDANCE
}

/**
 * MySensors function called when a message is received.
 */
void receive(const MyMessage &message) {
}

/**
 * Main loop.
 */
void loop() {
    int res;

    /* Linky task */
    struct linky_tic::dataset dataset = {0};
    res = m_linky.dataset_get(dataset);
    if (res < 0) {
        Serial.println(" [e] Linky error!");
        return;
    } else if (res == 1) {
        Serial.printf(" [d] Received dataset %s = %s\r\n", dataset.name, dataset.data);

        /* Serial number */
        if (strcmp(dataset.name, "ADCO") == 0) {
            if (strlen(m_linky_serial) == 0) {
                strncpy(m_linky_serial, dataset.data, 12);
                send(m_message_info_serial.set(m_linky_serial));
            }
        }

        /* Index for base */
        else if (strcmp(dataset.name, "BASE") == 0) {
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
            if (base_wh > base_wh_last) {
                send(m_message_power_kwh.set(base_wh / 1000.0, 3));
                base_wh_last = base_wh;
            }
        }

        /* Intensité Instantanée */
        else if (strcmp(dataset.name, "IINST") == 0) {
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
            if (current_a != current_a_last) {
                send(m_message_multimeter_current.set(current_a));
                current_a_last = current_a;
            }
        }

        /* Puissance apparente */
        else if (strcmp(dataset.name, "PAPP") == 0) {
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
            if (power_va != power_va_last) {
                send(m_message_power_watt.set(power_va));
                power_va_last = power_va;
            }
        }
    }
}
