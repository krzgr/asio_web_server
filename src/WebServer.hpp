#include "HTTPConnection.hpp"

class WebServer
{
public:
    static const int threadsNumber = 4;

private:
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::socket tcpSocket;
    boost::asio::ip::tcp::acceptor tcpAcceptor;
    boost::asio::ip::tcp::endpoint tcpEndpoint;

    boost::asio::thread_pool thrPool;

    uint16_t port;
    
public:
    WebServer(uint16_t _port);
    ~WebServer();

    void launch();
private:
    void listenForNewConnection();
    void onConnection(boost::shared_ptr<HTTPConnection> connection, const boost::system::error_code ec);
};