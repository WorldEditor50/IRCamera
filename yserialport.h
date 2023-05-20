#ifndef YSERIALPORT_H
#define YSERIALPORT_H
#include <QSerialPort>
#include <QSerialPortInfo>
#include <string>
#include <vector>

class YSerialPort : public QSerialPort
{
public:
    YSerialPort();
    bool openDevice(const std::string &name, int baudRate);
    void closeDevice();
    static std::vector<std::string> enumerate();
    void send(const char *data, int len);
};

#endif // YSERIALPORT_H
