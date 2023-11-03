#include "Si4731_driver.h"
#include "DebugLog.h"

radio_modules::Si4731::Si4731(uint8_t rst_pin,
        uint8_t sen_pin, 
        TwoWire& in_wire) : 
wire_ref(in_wire), 
sen{sen_pin}, 
rst{rst_pin}
{}
    
bool radio_modules::Si4731::init() 
{
    //start up sen 
    set_sen(); 

    //restart the chip 
    if (reset_chip()) 
    {
        //success fully init 
        LOG_DEBUG("successfully init the chip"); 
        LOG_DEBUG("running through power up sequence"); 
        if (power_up_cmd()) 
        {
            LOG_DEBUG("chip is powered up in FM mode and ready to go"); 
        } 
        else 
        {
            LOG_ERROR("power up came back as false"); 
            return false; 
        } 
    }
    else 
    {
        LOG_ERROR("restart failed when running through init"); 
        return false;
    }
    return true; 
}

radio_modules::Si4731::~Si4731()
{
    wire_ref.end(); 
}

bool radio_modules::Si4731::test_connection() 
{   
    //start i2c connection 
    wire_ref.begin(); 

    LOG_DEBUG("testing connection"); 
    //assumes sen is set to low 
    wire_ref.beginTransmission(SEN_LOW_ADDRESS); 
    auto error = wire_ref.endTransmission(); 
    if (error != 0)
    {
        LOG_ERROR("Received error with connection : ",error); 
        return false; 
    } 
    else 
    {
        LOG_DEBUG("successfully connected to the chip");
    }
    return true; 
}

bool radio_modules::Si4731::reset_chip() 
{
    //turn down then up again (docs says it only needs to be for 10 ns)
    LOG_DEBUG("attempting to reset"); 
    pinMode(rst,OUTPUT); 
    delay(10); 
    digitalWrite(rst,LOW); 
    delay(10); 
    digitalWrite(rst,HIGH); 
    delay(10); //should be reset 

    return test_connection(); //return if still connected
}

void radio_modules::Si4731::set_sen()
{
    LOG_DEBUG("setting SEN pin to low"); 
    pinMode(sen,OUTPUT); 
    delay(10); 
    digitalWrite(sen,LOW); 
    delay(10); 
}

bool radio_modules::Si4731::set_radio_station(float input_station)
{
    uint16_t freq = input_station * 10; 
    freq *= 10; 
    LOG_DEBUG("tuning to station : ",freq); 

    wait_for_cts(); 

    auto val_one = static_cast<uint8_t>(freq >>8); 
    auto val_two = static_cast<uint8_t>(freq & 0xff);

    LOG_DEBUG("val one is : ", val_one); 
    LOG_DEBUG("val two is : ", val_two);  

    wire_ref.begin(); 
    wire_ref.beginTransmission(SEN_LOW_ADDRESS);
    // wire_ref.write(0x21); 
    // wire_ref.write(0b00001100); 
    wire_ref.write(FM_TUNE_FREQ); 
    wire_ref.write(0b00000000); 
    wire_ref.write(static_cast<uint8_t>(freq >>8)); 
    wire_ref.write(static_cast<uint8_t>(freq & 0xff)); 
    wire_ref.write(0); 
    auto error = wire_ref.endTransmission(); 

    //check the return status 
    sleep_ms(10); 
    wire_ref.requestFrom(SEN_LOW_ADDRESS,1); 
    LOG_DEBUG("response after radio set is : ",wire_ref.read()); 

    return true; 
}

bool radio_modules::Si4731::tune_frequency(bool tune_up) 
{
    LOG_DEBUG("tuning frequency : ", tune_up ? "up" : "down"); 
    bool status = true; 
    status &= wait_for_cts(); 

    wire_ref.begin(); 
    wire_ref.beginTransmission(SEN_LOW_ADDRESS);
    wire_ref.write(FM_SEEK_START); 
    if (tune_up)
    {
        wire_ref.write(0b00001000); //seeks up 
    }
    else
    {
        wire_ref.write(0b00000000); //seeks down
    }
    wire_ref.endTransmission(); 

    //check for error 
    sleep_ms(10); 
    wire_ref.requestFrom(SEN_LOW_ADDRESS,1); 
    if (wire_ref.read() & 0b11000000) 
    {
        status = false; 
        LOG_ERROR("returned error when cts is not ready"); 
    }
    return status; 
}

radio_modules::TuneStatus radio_modules::Si4731::get_radio_frequency(bool abort_seek)
{
    //checks the status command and returns the frequency 
    //also logs the statuses 
    //if error returns 0; 
    LOG_DEBUG("running through get frequency function"); 
    radio_modules::TuneStatus tune_stat; 
    tune_stat.status = wait_for_cts();

    wire_ref.begin(); 
    wire_ref.beginTransmission(SEN_LOW_ADDRESS); 
    wire_ref.write(FM_TUNE_STATUS); //fm tune status
    if (abort_seek)
    {
        LOG_INFO("canceling current abort"); 
        wire_ref.write(0b00000010); 
    }
    else 
    {
        wire_ref.write(0); 
    }
    auto ret_status  =  wire_ref.endTransmission(); 
    LOG_DEBUG("return status = ", ret_status); 
    
    if (ret_status != 0) 
    {
        tune_stat.status = false; 
    }   

    sleep_ms(1); 

    //read back the results 
    uint8_t results[8];
    wire_ref.requestFrom(SEN_LOW_ADDRESS, 8);
    if (wire_ref.readBytes(results,8) != 8) 
    {
        LOG_ERROR("didnt read back the correct number of bytes"); 
        tune_stat.status = false; 
    } 

    //reading through bytes 
    if (!(0b01000000 & results[0]))
    {
        LOG_ERROR("received back error status from module"); 
        tune_stat.status = false; 
    }

    uint8_t response_1 = results[1]; 
    tune_stat.freq_lim_hit = 0b10000000 & response_1; 
    tune_stat.valid_channel = 0b00000001 & response_1;
    unsigned int high_byte = results[2];
    high_byte = high_byte << 8;  
    unsigned int low_byte = results[3];
    LOG_DEBUG("high freq byte : ", high_byte);  
    LOG_DEBUG("low freq byte : ", low_byte);   
    tune_stat.freq = high_byte + low_byte; 
    tune_stat.rssi = results[4]; 
    tune_stat.snr = results[5]; 
    tune_stat.mult = results[6]; 
    tune_stat.junk = results[7];  

    LOG_DEBUG("valid channel : ",tune_stat.valid_channel); 
    LOG_DEBUG("freq_limit_hit : ", tune_stat.freq_lim_hit); 
    LOG_DEBUG("current freq : ", tune_stat.freq); 

    return tune_stat; 
}


bool radio_modules::Si4731::set_gpio(Si4731_GPIO in_gpio, 
    bool status)
{
    return true;
}

bool radio_modules::Si4731::wait_for_cts()
{
    //add a break out counter in case
    int counter = 0; 
    do 
    {
        sleep_ms(10); 
        wire_ref.requestFrom(SEN_LOW_ADDRESS, 1); 
        counter++; 
    } while (!(wire_ref.read() & CTS_BIT) || counter > 100); 
    if (counter == 100) 
    {
        return false; 
    }
    return true; 
}

bool radio_modules::Si4731::power_up_cmd() 
{
    //check that I send something first 
    if (!wait_for_cts()) 
    {
        LOG_ERROR("wait for cts timed out"); 
        return false; 
    }

    wire_ref.begin(); 
    wire_ref.beginTransmission(SEN_LOW_ADDRESS); 
    wire_ref.write(0x01); 
    wire_ref.write(0b00010000); 
    wire_ref.write(0b00000101); 
    auto error = wire_ref.endTransmission(); 
    if (error != 0) 
    {
        LOG_ERROR("error occured when sending message : ",error); 
        return false; 
    }

    if (!wait_for_cts())
    {
        LOG_ERROR("wait for cts timed out (after power up cmd sent)"); 
        return false; 
    }

    LOG_INFO("power up cmds entered right"); 
    return true; 
}

bool radio_modules::Si4731::set_audio_enabled(bool audio_enabled)
{
    return true; 
}