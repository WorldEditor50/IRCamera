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
        STATE_OPEN,
        STATE_RECV,
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
    enum PacketOffset {
        PACKET_HEADER = 0,
        PACKET_SIZE = 2,
        PACKET_TEMPERATUES = 4,
        PACKET_TA = 1540,
        PACKET_CHECKSUM = 1542
    };

    class Scalar
    {
    public:
        float r;
        float g;
        float b;
    public:
        Scalar(){}
        explicit Scalar(float r_, float g_, float b_)
            :r(r_),g(g_),b(b_){}
    };

    struct Temperature {
        int i;
        int j;
        float value;
    };

    using FnProcess = std::function<void(int, int, unsigned char*)>;
    constexpr static size_t packet_len = 1544;
public:
    Temperature maxTemp;
    Temperature minTemp;
protected:
    /* parameter */
    int state;
    unsigned long baudRate;
    std::string portName;
    std::string sendBuffer;
    float minTemperature;
    float maxTemperature;
    float ta;
    std::vector<float> temperatures;
    /* device */
    std::mutex mutex;
    std::thread sampleThread;
    std::condition_variable condit;
    YSerialPort *serialport;
    /* image */
    FnProcess process;
    Scalar colorScalar;
    int h;
    int w;
    unsigned char *img;
protected:
    void run();
    static void packetToTemperatures(std::vector<float> &temperatures, float &ta, const unsigned char *packet);
    static void temperatureToRGB(int h, int w, unsigned char *img,
                                 const std::vector<float> &temperatures,
                                 const Scalar &s,
                                 Temperature &maxTemp,
                                 Temperature &minTemp);
public:
    explicit MLX90640(int h_, int w_, FnProcess func);
    ~MLX90640();
    void setColorScalar(float r, float g, float b);
    void setFrequence(int freq);
    void setMode(int mode);
    void setEmissivity(float value);
    bool open(const std::string &portName, unsigned long baudRate_);
    void close();
};

#endif // MLX90640_H
