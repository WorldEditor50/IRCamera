#include "mlx90640.h"
#include <iostream>

void MLX90640::run()
{
    serialport = new YSerialPort;
    while (1) {
        std::unique_lock<std::mutex> locker(mutex);
        {
            condit.wait_for(locker, std::chrono::microseconds(1000), [=]()->bool {
               return state == STATE_TERMINATE || state == STATE_PREPEND;
            });
        }
        if (state == STATE_TERMINATE) {
            /* close device */
            serialport->closeDevice();
            serialport->deleteLater();
            state = STATE_NONE;
            break;
        } else if (state == STATE_PREPEND) {
            /* open device */
            if (devPath.empty()) {
                continue;
            }
            /*  close if device is opened. */
            serialport->closeDevice();
            bool ret = serialport->openDevice(devPath.c_str(), baudRate);
            if (ret == false) {
                continue;
            }
            state = STATE_READY;
        } else if (state == STATE_SEND) {
            /* send message */
            if (sendBuffer.empty() == false) {
                serialport->send(sendBuffer.c_str(), sendBuffer.size());
            }
            state = STATE_READY;
        } else if (state == STATE_READY) {
            /* recv message */
            if (serialport->waitForReadyRead(50) == false) {
                continue;
            }
            if (serialport->bytesAvailable()) {
                QByteArray data = serialport->readAll();
                while (serialport->waitForReadyRead(10)) {
                    data += serialport->readAll();
                }
                unsigned char *d = (unsigned char *)data.data();
                if (d[0]==0x5a && d[1]==0x5a &&
                        d[2]==0x02 && d[3]==0x06) {
                    parse(d, data.size());
                    process(hi, wi, img);
                }
            }
        }
    }
    return;
}

void MLX90640::parse(unsigned char *data, int datasize)
{
    if (datasize < packet_len) {
        return;
    }
    Packet *packet = (Packet*)data;
    ta = packet->ta;
    //float envTemp = ta - 800;
    unsigned short* temperatures = packet->temperatures;
    /* find max-min temperature */
    float maxValue = temperatures[0];
    float minValue = temperatures[0];
    for (int i = 0; i < hi; i++) {
        for (int j = 0; j < wi; j++) {
            int k = i*wi + j;
            if (maxValue < temperatures[k]) {
                maxValue = temperatures[k];
                maxTemp.value = maxValue;
                maxTemp.i = i;
                maxTemp.j = j;
            }
            if (minValue > temperatures[k]) {
                minValue = temperatures[k];
                minTemp.value = maxValue;
                minTemp.i = i;
                minTemp.j = j;
            }
        }
    }
    /* convert to rgb image */
    int widthstep = wi*3;
    for (int i = 0; i < hi; i++) {
        for (int j = 0; j < wi; j++) {
            float temp = temperatures[i*wi + j];
            /* normalize */
            double pixel = (temp - minValue)/(maxValue - minValue)*255;
            /* image */
            img[i*widthstep + j*3]     = pixel;
            img[i*widthstep + j*3 + 1] = 0;
            img[i*widthstep + j*3 + 2] = 0;
        }
    }
    return;
}

MLX90640::MLX90640(MLX90640::FnProcess func)
        :state(STATE_NONE), process(func),img(nullptr)
{
    img = new unsigned char[hi*wi*3];
}

MLX90640::~MLX90640()
{
    /* clear */
    if (img != nullptr) {
        delete [] img;
    }
}

void MLX90640::setFrequence(int freq)
{
    if (state != STATE_READY) {
        return;
    }
    std::unique_lock<std::mutex> locker(mutex);
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
    if (state != STATE_READY) {
        return;
    }
    std::unique_lock<std::mutex> locker(mutex);
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
    if (state != STATE_READY) {
        return;
    }
    std::unique_lock<std::mutex> locker(mutex);
    unsigned char data[4] = {0xa5, 0x45, (unsigned char)(value*100), 0xdc};
    sendBuffer = std::string((char*)data, 4);
    state = STATE_SEND;
    condit.notify_all();
    return;
}

bool MLX90640::openDevice(const std::string &path, unsigned long baudRate_)
{
    if (path.empty())  {
        return false;
    }
    if (state != STATE_NONE) {
        return true;
    }
    devPath = path;
    baudRate = baudRate_;
    state = STATE_PREPEND;
    sampleThread = std::thread(&MLX90640::run, this);
    std::unique_lock<std::mutex> locker(mutex);
    condit.wait_for(locker, std::chrono::microseconds(1000), [=]()->bool {
       return state == STATE_READY;
    });
    return state == STATE_READY;
}

void MLX90640::closeDevice()
{
    /* stop sample */
    if (state == STATE_NONE) {
        return;
    }
    if (state != STATE_TERMINATE) {
        std::unique_lock<std::mutex> locker(mutex);
        state = STATE_TERMINATE;
        condit.notify_all();
    }
    sampleThread.join();
    return;
}
