#include "mlx90640.h"

void MLX90640::run()
{
    serialport = new YSerialPort;
    state = STATE_NONE;
    while (1) {
        std::unique_lock<std::mutex> locker(mutex);
        if (state == STATE_TERMINATE) {
            /* close device */
            serialport->closeDevice();
            serialport->deleteLater();
            break;
        } else if (state == STATE_OPEN) {
            /* open device */
            if (portName.empty()) {
                continue;
            }
            /*  close if device is opened. */
            serialport->closeDevice();
            bool ret = serialport->openDevice(portName.c_str(), baudRate);
            if (ret == false) {
                continue;
            }
            state = STATE_RECV;
        } else if (state == STATE_SEND) {
            /* send message */
            if (sendBuffer.empty() == false) {
                serialport->send(sendBuffer.c_str(), sendBuffer.size());
            }
            state = STATE_RECV;
        } else if (state == STATE_RECV) {
            /* recv message */
            if (serialport->waitForReadyRead(50) == false) {
                continue;
            }
            if (serialport->bytesAvailable()) {
                QByteArray data = serialport->readAll();
                while (serialport->waitForReadyRead(10)) {
                    data += serialport->readAll();
                }
                unsigned char *packet = (unsigned char *)data.data();
                if (packet[0]==0x5a && packet[1]==0x5a &&
                        packet[2]==0x02 && packet[3]==0x06) {
                    /* packet to temperature */
                    packetToTemperatures(temperatures, ta, packet);
                    /* temperature to rgb image */
                    temperatureToRGB(h, w, img, temperatures, colorScalar, maxTemp, minTemp);
                    process(h, w, img);
                }
            }
        }
    }
    return;
}

void MLX90640::packetToTemperatures(std::vector<float> &temperatures, float &ta, const unsigned char *packet)
{
    /* TA */
    ta = (packet[PACKET_TA + 1]<<8) + packet[PACKET_TA];
    /* temperatures */
    const unsigned char *temps = packet + PACKET_TEMPERATUES;
    for (std::size_t i = 0; i < temperatures.size(); i++) {
        temperatures[i] = (temps[2*i + 1]<<8) + temps[2*i];
    }
    /* correction */
    float delta = 0;
    float envTemp = ta - 800;
    for (std::size_t i = 0; i < temperatures.size(); i++) {
        delta += (temperatures[i] - envTemp)*(temperatures[i] - envTemp);
    }
    delta /= temperatures.size();
    delta = std::sqrt(delta);

    for (std::size_t i = 0; i < temperatures.size(); i++) {
        temperatures[i] -= delta;
    }
    return;
}

void MLX90640::temperatureToRGB(int h, int w, unsigned char *img,
                                const std::vector<float> &temperatures,
                                const Scalar &s,
                                Temperature &maxTemp,
                                Temperature &minTemp)
{
    float maxValue = temperatures[0];
    float minValue = temperatures[0];
    for (std::size_t i = 0; i < temperatures.size(); i++) {
        if (maxValue < temperatures[i]) {
            maxValue = temperatures[i];
            maxTemp.value = maxValue;
            maxTemp.i = i/w;
            maxTemp.j = i%w;
        }
        if (minValue > temperatures[i]) {
            minValue = temperatures[i];
            minTemp.value = maxValue;
            minTemp.i = i/w;
            minTemp.j = i%w;
        }
    }
    int widthstep = w*3;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            float temp = temperatures[i*w + j];
            /* normalize */
            double pixel = (temp - minValue)/(maxValue - minValue)*255;
            /* image */
            img[i*widthstep + j*3]     = pixel*s.r;
            img[i*widthstep + j*3 + 1] = pixel*s.g;
            img[i*widthstep + j*3 + 2] = pixel*s.b;

        }
    }
    return;
}

MLX90640::MLX90640(int h_, int w_, MLX90640::FnProcess func)
        :state(STATE_NONE),process(func), h(h_), w(w_),img(nullptr)
{
    temperatures = std::vector<float>(h*w);
    img = new unsigned char[h*w*3];
    colorScalar = Scalar(0.8, 0.1, 0.1);
    sampleThread = std::thread(&MLX90640::run, this);
}

MLX90640::~MLX90640()
{
    close();
    sampleThread.join();
    /* clear */
    if (img != nullptr) {
        delete [] img;
    }
}

void MLX90640::setColorScalar(float r, float g, float b)
{
    colorScalar = Scalar(r, g, b);
    return;
}

void MLX90640::setFrequence(int freq)
{
    std::unique_lock<std::mutex> locker(mutex);
    if (state != STATE_RECV) {
        return;
    }
    if (freq == FREQ_8HZ) {
       unsigned char data[4] = {0xa5, 0x25, 0x01, 0xcb};
       sendBuffer = std::string((char*)data, 4);
    } else if (freq == FREQ_4HZ) {
       unsigned char data[4] = {0xa5, 0x25, 0x02, 0xcc};
       sendBuffer = std::string((char*)data, 4);
    } else if (freq == FREQ_2HZ){
       unsigned char data[4] = {0xa5, 0x25, 0x03, 0xcd};
       sendBuffer = std::string((char*)data, 4);
    }
    state = STATE_SEND;
    condit.notify_all();
    return;
}

void MLX90640::setMode(int mode)
{  
    std::unique_lock<std::mutex> locker(mutex);
    if (state != STATE_RECV) {
        return;
    }
    if (mode == MODE_PUSH) {
       unsigned char data[4] = {0xa5, 0x35, 0x01, 0xdb};
       sendBuffer = std::string((char*)data, 4);
    } else if(mode == MODE_REQ) {
       unsigned char data[4] = {0xa5, 0x35, 0x02, 0xdc};
       sendBuffer = std::string((char*)data, 4);
    }
    state = STATE_SEND;
    condit.notify_all();
    return;
}

void MLX90640::setEmissivity(float value)
{
    std::unique_lock<std::mutex> locker(mutex);
    if (state != STATE_RECV) {
        return;
    }
    unsigned char data[4] = {0xa5, 0x45, (unsigned char)(value*100), 0xdc};
    sendBuffer = std::string((char*)data, 4);
    state = STATE_SEND;
    condit.notify_all();
    return;
}

bool MLX90640::open(const std::string &portName_, unsigned long baudRate_)
{
    if (portName_.empty())  {
        return false;
    }
    std::unique_lock<std::mutex> locker(mutex);
    portName = portName_;
    baudRate = baudRate_;
    state = STATE_OPEN;
    condit.notify_all();
    return true;
}

void MLX90640::close()
{
    /* stop sample */
    std::unique_lock<std::mutex> locker(mutex);
    if (state == STATE_NONE) {
        return;
    }
    if (state == STATE_TERMINATE) {
        return;
    }
    state = STATE_TERMINATE;
    condit.notify_all();
    return;
}
