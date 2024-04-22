#include "WebServer.hpp"

WebServer::WebServer(uint16_t _port)
    : tcpSocket(ioContext), tcpAcceptor(ioContext), port(_port), 
      tcpEndpoint(boost::asio::ip::tcp::v4(), _port), thrPool(WebServer::threadsNumber)
{
    
}

void WebServer::launch()
{
    boost::system::error_code ec;

    tcpAcceptor.open(tcpEndpoint.protocol(), ec);
    if(ec)
    {
        std::cout << ec.message() << std::endl;
        return;
    }

    tcpAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

    tcpAcceptor.bind(tcpEndpoint, ec);
    if(ec)
    {
        std::cout << ec.message() << std::endl;
        return;
    }

    tcpAcceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    if(ec)
    {
        std::cout << ec.message() << std::endl;
        return;
    }

    listenForNewConnection();
    
    boost::asio::post(thrPool, [&](){
        ioContext.run();
    });
    
    thrPool.join();
}

WebServer::~WebServer()
{
    ioContext.stop();
}

void WebServer::listenForNewConnection()
{
    boost::shared_ptr<HTTPConnection> connection = HTTPConnection::createNewConnection(ioContext);

    tcpAcceptor.async_accept(connection->getSocket(), 
        boost::bind(&WebServer::onConnection, this, connection, boost::placeholders::_1));
}

void WebServer::onConnection(boost::shared_ptr<HTTPConnection> connection, boost::system::error_code ec)
{
    if(ec)
        std::cout << "[*] " << ec.message() << std::endl;
    else
        connection->start();

    listenForNewConnection();
}