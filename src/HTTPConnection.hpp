#include <iostream>
#include <stdio.h>
#include <vector>
#include <ctype.h>
#include <chrono>
#include <fstream>
#include <string>
#include <regex>
#include <filesystem>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

class HTTPConnection : public boost::enable_shared_from_this<HTTPConnection>
{
public:
    static const unsigned int bufferMaxSize = 64 * 1024;
    static const std::string rootPath;

private:
    boost::asio::streambuf buffer;
    boost::asio::ip::tcp::socket tcpSocket;

    std::vector<std::string> requestHeader;

public:
    ~HTTPConnection();

    bool isOpen();

    void start();
    void close();

    boost::asio::ip::tcp::socket& getSocket();

    static boost::shared_ptr<HTTPConnection> createNewConnection(boost::asio::io_context& ioContext);

    static std::string decodeURL(std::string url);

private:
    HTTPConnection(boost::asio::io_context& ioContext);
    void readRequestHeader();
    void respond();

    void sendChunck(std::string path, int offset);
    void onSendChunk(std::string path, int offset, const boost::system::error_code ec, std::size_t bytes_transferred);

    void receiveChunck(std::string path);
    void onReceiveChunk(std::string path, const boost::system::error_code ec, std::size_t bytes_transferred);

    void readRequestLine();
    void onReadRequestLine(boost::system::error_code ec, std::size_t bytes_transferred);

    bool isPathValid(std::string path) const;

    // HTTP Methods

    void GET_(std::string path, std::string protocol);
    void HEAD_(std::string path, std::string protocol);
    void POST_(std::string path, std::string protocol);
    void PUT_(std::string path, std::string protocol);
    void DELETE_(std::string path, std::string protocol);
    void CONNECT_(std::string path, std::string protocol);
    void OPTIONS_(std::string path, std::string protocol);
    void TRACE_(std::string path, std::string protocol);
    void PATCH_(std::string path, std::string protocol);
};