#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <cctype>
#include <boost/asio.hpp>
#include <thread>
#include <mutex>

std::mutex serialMutex; 

void ListSerialPorts()
{
    struct dirent *entry;
    DIR *dp = opendir("/dev");

    if (dp == NULL)
    {
        perror("opendir");
        std::cerr << "Không thể mở cổng serial." << std::endl;
        return;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        if (strncmp("ttyUSB", entry->d_name, 6) == 0 || strncmp("ttyACM", entry->d_name, 6) == 0)
        {
            std::cout << "/dev/" << entry->d_name << std::endl;
        }
    }

    closedir(dp);
}

void SendControlData(boost::asio::serial_port &serial)
{
    while (true)
    {
        std::string command;
        char ch;
        std::cout << "Nhập tín hiệu điều khiển hoặc nhấn Esc để kết thúc: ";
        while (true)
        {
            ch = std::cin.get();
            if (ch == 27)
            {
                return;
            }
            else if (!std::isspace(ch))
            {
                command += ch;
            }
        }
        
        std::lock_guard<std::mutex> lock(serialMutex);
        boost::asio::write(serial, boost::asio::buffer(command));
        std::cout << "Đã gửi tín hiệu điều khiển: " << command << std::endl;
    }

}

void ReceiveDistanceData(boost::asio::serial_port &serial, bool isDistance)
{
    while (true)
    {
        char data[256];
        {
            std::lock_guard<std::mutex> lock(serialMutex);
            size_t bytesRead = boost::asio::read(serial, boost::asio::buffer(data, sizeof(data)));
            data[bytesRead] = '\0';
        }
        if (isDistance) {
            std::cout << "Received distance: " << data << std::endl;
        } else {
            std::cout << "Received control signal: " << data << std::endl;
        }
    }
}

int main()
{
    std::cout << "Các cổng Serial có sẵn:" << std::endl;
    ListSerialPorts();

    std::string portName;
    std::cout << "Nhập tên cổng Serial (COM chính) : ";
    std::cin >> portName;

    int baudRate;
    std::cout << "Nhập tốc độ truyền : ";
    std::cin >> baudRate;

    boost::asio::io_service io_service;
    boost::asio::serial_port mainSerial(io_service, portName);
    mainSerial.set_option(boost::asio::serial_port_base::baud_rate(baudRate));

    boost::asio::io_service io_serviceVirtual;
    std::string portNameVirtual = "/dev/ttyS10"; 
    boost::asio::serial_port serialVirtual(io_serviceVirtual, portNameVirtual);
    serialVirtual.set_option(boost::asio::serial_port_base::baud_rate(baudRate));

    std::thread sender(SendControlData, std::ref(mainSerial));
    std::thread receiver(ReceiveDistanceData, std::ref(serialVirtual),  true);

    sender.join();
    receiver.join();

    return 0;
}
