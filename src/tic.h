#ifndef TIC_H
#define TIC_H

/* Arduino libraries */
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>

/* C/C++ libraries */
#include <errno.h>

class tic {
   public:
    struct dataset {
        char name[8 + 1];
        char data[12 + 1];
        uint32_t time;  // Optional
    };
    int setup(const int pin_data_in, const int pin_data_out);
    int begin(const int baudrate = 0);
    int dataset_get(struct dataset &dataset);

   protected:
    SoftwareSerial *m_port = NULL;
    int m_pin;
    enum {
        STATE_0,
        STATE_1,
        STATE_2,
        STATE_3,
        STATE_ERROR,
    } m_sm;
    size_t m_buffer_index;
    uint8_t m_buffer[8 + 1 + 13 + 1 + 12 + 1 + 1];
    uint8_t m_splitter_char;
};

#endif
