/* Self header */
#include "linky_tic.h"

/**
 *
 */
int linky_tic::setup(const int pin_data_in, const int pin_data_out) {
    if (m_port == NULL) {
        m_port = new SoftwareSerial(pin_data_in, pin_data_out);
        if (m_port == NULL) {
            return -ENOMEM;
        }
        m_pin = pin_data_in;
    }
    return 0;
}

/**
 *
 */
int linky_tic::begin(const int baudrate) {

    /* Ensure setup has been performed */
    if (m_port == NULL) {
        return -EINVAL;
    }

    /* If baudrate is known, setup the port directly */
    if (baudrate == 1200 || baudrate == 9600) {
        Serial.print(" [i] Using baudrate of ");
        Serial.println(baudrate);
        m_port->begin(baudrate);
    }

    /* Otherwise, try to determine the baudrate by sampling the pin for the fastest transition
     * @see https://forum.arduino.cc/t/auto-serial-baudrate-detector-selector/38256
     * 9600bauds = approx 104µs per bit
     * 1200bauds = approx 833µs per bit */
    else {
        pinMode(m_pin, INPUT);
        uint32_t rate_min = UINT32_MAX;
        for (uint8_t i = 0; i < 10; i++) {
            while (digitalRead(m_pin) == 1) {
            }
            uint32_t rate = pulseIn(m_pin, LOW);
            Serial.println(rate);
            if (rate < rate_min) rate_min = rate;
        }
        if (rate_min >= 666 && rate_min <= 1000) {
            Serial.println(" [i] Detected baudrate of 1200");
            m_port->begin(1200);
        } else if (rate_min >= 83 && rate_min <= 125) {
            Serial.println(" [i] Detected baudrate of 9600");
            m_port->begin(9600);
        } else {
            Serial.println(" [e] Failed to detect baudrate!");
            return -EIO;
        }
    }

    /* Return success */
    return 0;
}

/**
 *
 * @return 1 if a valid dataset is available, 0 if no dataset is available, or a negative error code otherwise
 */
int linky_tic::dataset_get(struct dataset &dataset) {

    /* Ensure setup has been performed */
    if (m_port == NULL) {
        return -EINVAL;
    }

    /* Process incoming bytes */
    while (m_port->available() > 0) {

        /* Read byte, ensure parity, and remove parity bit */
        uint8_t rx = m_port->read();

        /* Perform parity check */
        if (__builtin_parity(rx) != 0) {
            Serial.println(" [e] Parse error: invalid parity bit!");
            m_sm = STATE_0;
            return -EIO;
        }

        /* Remove parity bit */
        rx = rx & 0x7F;

        /* Parsing state machine */
        switch (m_sm) {

            case STATE_0: {  // Wait for frame start
                if (rx == 0x02) {
                    // Serial.println(" [d] Frame start");
                    m_sm = STATE_1;
                    break;
                }
                break;
            }

            case STATE_1: {  // Wait for either dataset start or frame stop
                if (rx == 0x0A) {
                    // Serial.println(" [d] Dataset start");
                    m_buffer_index = 0;
                    m_sm = STATE_2;
                } else if (rx == 0x03) {
                    // Serial.println(" [d] Frame stop");
                    m_sm = STATE_0;
                } else {
                    Serial.println(" [e] Parse error: expected dataset start or frame stop!");
                    m_sm = STATE_0;
                    return -EIO;
                }
                break;
            }

            case STATE_2: {  // Parse dataset tag
                if (rx == 0x09 || rx == 0x20) {
                    m_buffer[m_buffer_index] = rx;
                    m_buffer_index++;
                    m_splitter_char = rx;
                    m_sm = STATE_3;
                } else {
                    if (m_buffer_index >= 8) {
                        Serial.println(" [e] Parse error: tag too long!");
                        m_sm = STATE_0;
                        return -EIO;
                    } else {
                        m_buffer[m_buffer_index] = rx;
                        m_buffer_index++;
                    }
                }
                break;
            }

            case STATE_3: {  // Wait for dataset stop

                /* If we receive the end of dataset character */
                if (rx == 0x0D) {
                    Serial.print(" [d] Dataseet:");
                    for (size_t i = 0; i < m_buffer_index; i++) {
                        if ((m_buffer[i] >= 0x30 && m_buffer[i] <= 0x5A))
                            Serial.printf(" %c", m_buffer[i]);
                        else
                            Serial.printf(" 0x%02X", m_buffer[i]);
                    }
                    Serial.println();

                    /* Verify checksum:
                     *  - for historic datasets, where the splitter char is 0x20, the checksum doesn't include the last splitter char,
                     *  - for standard datasets, where the splitter char is 0x09, the checkum includes the last splitter char.
                     * Go figure */
                    if (m_splitter_char == 0x20) {
                        uint8_t checksum_received = m_buffer[m_buffer_index - 1];
                        uint8_t checksum_computed = 0x00;
                        for (size_t i = 0; i < m_buffer_index - 2; i++) checksum_computed += m_buffer[i];
                        checksum_computed = (checksum_computed & 0x3F) + 0x20;
                        if (checksum_computed != checksum_received) {
                            Serial.println(" [e] Parse error: invalid checksum!");
                            m_sm = STATE_0;
                            return -EIO;
                        }
                    } else if (m_splitter_char == 0x09) {
                        uint8_t checksum_received = m_buffer[m_buffer_index - 1];
                        uint8_t checksum_computed = 0x00;
                        for (size_t i = 0; i < m_buffer_index - 1; i++) checksum_computed += m_buffer[i];
                        checksum_computed = (checksum_computed & 0x3F) + 0x20;
                        if (checksum_computed != checksum_received) {
                            Serial.println(" [e] Parse error: invalid checksum!");
                            m_sm = STATE_0;
                            return -EIO;
                        }
                    } else {
                        Serial.println(" [e] Parse error: invalid splitter char!");
                        m_sm = STATE_0;
                        return -EIO;
                    }

                    /* Split dataset */
                    uint8_t splitter_posistions[3] = {0};
                    uint8_t splitter_count = 0;
                    for (uint8_t i = 0; i < m_buffer_index - 1; i++) {
                        if (m_buffer[i] == m_splitter_char) {
                            if (splitter_count < 3) {
                                splitter_posistions[splitter_count] = i;
                            }
                            splitter_count++;
                        }
                    }

                    /* Fill dataset struct and move on */
                    if (splitter_count == 2) {
                        strncpy(dataset.name, (char *)(&m_buffer[0]), splitter_posistions[0]);
                        strncpy(dataset.data, (char *)(&m_buffer[splitter_posistions[0] + 1]), splitter_posistions[1] - (splitter_posistions[0] + 1));
                        m_sm = STATE_1;
                        return 1;
                    } else if (splitter_count == 3) {
                        strncpy(dataset.name, (char *)(&m_buffer[0]), splitter_posistions[0]);
                        strncpy(dataset.data, (char *)(&m_buffer[splitter_posistions[1] + 1]), splitter_posistions[2] - (splitter_posistions[1] + 1));
                        m_sm = STATE_1;
                        // TODO Time parsing
                        return 1;
                    } else {
                        Serial.println(" [e] Parse error: invalid token count!");
                        m_sm = STATE_0;
                        return -EIO;
                    }
                    break;
                }

                /* Otherwise, keep appending data to current dataset */
                else {
                    if (m_buffer_index >= 37) {
                        Serial.println(" [e] Parse error: dataset too long!");
                        m_sm = STATE_0;
                    } else {
                        m_buffer[m_buffer_index] = rx;
                        m_buffer_index++;
                    }
                }
                break;
            }

            default: {
                Serial.println(" [e] Hurray, we found a cosmic ray!");
                break;
            }
        }
    }

    /* Return no dataset */
    return 0;
}
