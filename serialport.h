#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <string>

class Serialport
{
public:
    Serialport();
    ~Serialport();
    int open(const std::string &portName, int baudRate);
    int close();
    int write(const std::string &data);
    int writeRaw(const char* data, int size);
    std::string read();
};

#endif // SERIALPORT_H
