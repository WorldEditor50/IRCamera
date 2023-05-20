#include "yserialport.h"

YSerialPort::YSerialPort()
{

}

bool YSerialPort::openDevice(const std::string &name, int baudRate)
{
    setPortName(name.c_str());
    bool ret = this->open(QIODevice::ReadWrite);
    if (ret == false) {
        return false;
    }
    this->setBaudRate(baudRate);
    this->setDataBits(QSerialPort::Data8);
    this->setParity(QSerialPort::NoParity);
    this->setStopBits(QSerialPort::OneStop);
    this->setFlowControl(QSerialPort::NoFlowControl);
    this->setReadBufferSize(2048);
    return true;
}

void YSerialPort::closeDevice()
{
    if (this->isOpen()) {
        this->clear();
        this->close();
    }
    return;
}

std::vector<std::string> YSerialPort::enumerate()
{
    const QSerialPortInfo info;
    QList<QSerialPortInfo> la = info.availablePorts();

    std::vector<std::string> devices;

    for (int i =0; i < la.size(); i++) {
        QSerialPort serialport;
        serialport.setPort(la[i]);
        if (serialport.open(QIODevice::ReadWrite)) {
            devices.push_back(serialport.portName().toStdString());
            serialport.close();
        }
    }
    return devices;
}

void YSerialPort::send(const char * data, int len)
{
    this->write(data, len);
    return;
}

