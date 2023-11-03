#pragma once 
#include <Arduino.h>
#include <Wire.h>

namespace radio_modules
{ 

    enum class Si4731_GPIO
    {
        GPIO1, 
        GPIO2
    }; 

    struct TuneStatus
    {
        unsigned int freq; //freq in 10 kHz
        bool freq_lim_hit; 
        bool afc_rail; 
        bool valid_channel; 
        uint8_t rssi; 
        uint8_t snr; 
        uint8_t mult; 
        uint8_t junk;  
        bool status; 
    }; 

    class Si4731
    {
        public: 
        Si4731(uint8_t rst_pin,
        uint8_t sen_pin, 
        TwoWire& in_wire); 

        ~Si4731();

        bool init(); //also restarts the chip 

        //functions that will be needed 
        bool test_connection(); 
        bool set_radio_station(float input_station); 
        bool set_gpio(Si4731_GPIO in_gpio, bool status);
        bool set_audio_enabled(bool audio_enabled); 
        bool tune_frequency(bool tune_up); //if false tunes down
        TuneStatus get_radio_frequency( bool abort_seek = false ); 

        private: 

        bool reset_chip(); 
        void set_sen(); 
        bool wait_for_cts(); 

        bool power_up_cmd();  
        TwoWire& wire_ref; 

        uint8_t sen; 
        uint8_t rst;

        //assumes sen is low
        static constexpr uint8_t SEN_LOW_ADDRESS {0x11};
        static constexpr uint8_t SEN_HIGH_ADDRESS {0x63};

        //status response bits 
        static constexpr uint8_t CTS_BIT    {0b10000000}; //if true good to send next command 
        static constexpr uint8_t ERR_BIT    {0b01000000}; 
        static constexpr uint8_t STCINT_BIT {0b00000001}; //tune is done

        //set freq 
        static constexpr uint8_t FM_TUNE_FREQ {0x20}; 
        static constexpr uint8_t FM_SEEK_START {0x21}; 
        static constexpr uint8_t FM_TUNE_STATUS {0x22}; 

    }; 
}