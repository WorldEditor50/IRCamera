#ifndef MLX90640_H
#define MLX90640_H
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <cmath>

#include "yserialport.h"


/*

    PACKET:
    +-------+-------+--------------+-------------+-------------+
    |HEADER | SIZE  | TEMPERATUES  |    TA       |  CHECKSUM   |
    +---+---+---+---+---+---+------+------+------+------+------+
    | 0 | 1 | 2 | 3 | 4 |...| 1539 | 1540 | 1541 | 1542 | 1543 |
    +---+---+---+---+---+---+------+------+------+------+------+
    HEADER:         0x5a,0x5a

    SIZE:           size = byte3*256 + byte2

    TEMPERATUES:    T1 = (byte5*256 + byte4)/100,
                    T768 = (byte1539*256 + byte1538)/100

    TA:             TA = (byte1541*256+ byte1540)/100

*/

class MLX90640
{
public:
    enum State {
        STATE_NONE = 0,
        STATE_PREPEND,
        STATE_SEND,
        STATE_WAIT,
        STATE_TIMEOUT,
        STATE_READY,
        STATE_TERMINATE
    };

    enum BaudRate {
        BAUDRATE_115200 = 115200,
        BAUDRATE_460800 = 460800
    };

    enum Frequence{
        FREQ_8HZ = 0,
        FREQ_4HZ,
        FREQ_2HZ
    };

    enum Mode {
        MODE_PUSH = 0,
        MODE_REQ
    };
    struct Packet {
        unsigned short header;
        unsigned short size;
        unsigned short temperatures[768];
        unsigned short ta;
        unsigned short checksum;
    };

    struct Temperature {
        int i;
        int j;
        float value;
    };

    using FnProcess = std::function<void(int, int, unsigned char*)>;
    constexpr static int packet_len = sizeof (Packet);
    constexpr static int hi = 24;
    constexpr static int wi = 32;
public:
    Temperature maxTemp;
    Temperature minTemp;
protected:
    /* parameter */
    int state;
    unsigned long baudRate;
    std::string devPath;
    std::string sendBuffer;
    float minTemperature;
    float maxTemperature;
    float ta;
    /* device */
    std::mutex mutex;
    std::thread sampleThread;
    std::condition_variable condit;
    YSerialPort *serialport;
    /* image */
    FnProcess process;
    unsigned char *img;
protected:
    void run();
    void parse(unsigned char* data, int datasize);
public:
    explicit MLX90640(FnProcess func);
    ~MLX90640();
    void setFrequence(int freq);
    void setMode(int mode);
    void setEmissivity(float value);
    bool openDevice(const std::string &path, unsigned long baudRate_);
    void closeDevice();
};

#endif // MLX90640_H
