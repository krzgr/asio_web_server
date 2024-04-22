#include "HTTPConnection.hpp"

const std::string HTTPConnection::rootPath = "C:/Users/Krzysztof Grund/Desktop/dell/Asio_projects/asio_web_server/public/"; //"C:/"; 

HTTPConnection::HTTPConnection(boost::asio::io_context& ioContext)
    : tcpSocket(ioContext), buffer(bufferMaxSize)
{

}

bool HTTPConnection::isOpen()
{
    return tcpSocket.is_open();
}

void HTTPConnection::close()
{
    tcpSocket.close();
}

void HTTPConnection::start()
{
    readRequestHeader();
}

void HTTPConnection::readRequestHeader()
{
    readRequestLine();
}

boost::asio::ip::tcp::socket& HTTPConnection::getSocket()
{
    return tcpSocket;
}

HTTPConnection::~HTTPConnection()
{
    close();
}

boost::shared_ptr<HTTPConnection> HTTPConnection::createNewConnection(boost::asio::io_context& ioContext)
{
    return boost::shared_ptr<HTTPConnection>(new HTTPConnection(ioContext));
}

std::string aaa(std::string url)
{
    std::string result = "";

    for(int i = 0; i < url.size(); i++)
    {
        if(url[i] != '%')
            result += url[i];
        else if(i + 2 < url.size())
        {
            int charCode;// = (url[i + 1] - '0') * 16 + url[i + 2] - '0';
            char tmp = url[i + 3];
            url[i + 3] = '\0';
            sscanf(&url[i + 1], "%x", &charCode);
            url[i + 3] = tmp;
            result += char(charCode);
            i += 2;
        }
    }

    return result;
}

std::string HTTPConnection::decodeURL(std::string str){
    std::string ret;
    char ch;
    int i, ii, len = str.length();

    for (i=0; i < len; i++){
        if(str[i] != '%'){
            //if(str[i] == '+')
            //    ret += ' ';
            //else
                ret += str[i];
        }else{
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return ret;
}

bool HTTPConnection::isPathValid(std::string path) const
{
    try
    {
        std::string pathCanonical = std::filesystem::canonical(std::filesystem::u8path(rootPath + path)).generic_u8string();
        std::string rootPathCanonical = std::filesystem::canonical(std::filesystem::u8path(rootPath)).generic_u8string();

        std::cout << "Path:      " << pathCanonical << std::endl;
        std::cout << "Root path: " << rootPathCanonical << std::endl;

        if(pathCanonical.find(rootPathCanonical) == std::string::npos)
            return false;
    }
    catch(...)
    {
        return false;
    }

    return true;
}


void HTTPConnection::readRequestLine()
{
    boost::asio::async_read_until(tcpSocket, buffer, "\r\n",
        boost::bind(&HTTPConnection::onReadRequestLine, shared_from_this(), boost::placeholders::_1, boost::placeholders::_2));
}

void HTTPConnection::onReadRequestLine(boost::system::error_code ec, std::size_t bytes_transferred)
{
    if(ec)
    {
        std::cout << "[*] " << ec.message() << std::endl;
        return;
    }

    if(bytes_transferred <= 2)
    {
        //std::cout << "[*] " << bytes_transferred << std::endl;
        buffer.consume(2);
        respond();
        return;
    }
    
    std::istream is(&buffer);
    std::string line; 
    std::getline(is, line);
    line.pop_back(); // removing \r character

    //std::cout << "[*] " << bytes_transferred << " " << line << std::endl;

    requestHeader.emplace_back(std::move(line));
    readRequestLine();
}

void HTTPConnection::respond()
{
    std::ostream os(&buffer);

    std::regex rgx("^([A-Z]+) /([^\\s]*) (HTTP/\\d\\.\\d)$");
    std::smatch rgxResult;

    if(!std::regex_search(requestHeader[0], rgxResult, rgx))
    {
        os << "HTTP/1.1 400 Bad Request\r\n\r\n";
        tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });
        return;
    }

    std::string method = rgxResult[1];
    std::string path = rgxResult[2];
    std::string protocol = rgxResult[3];

    size_t lastquestionMark = path.find_last_of('?');
    if(lastquestionMark != std::string::npos)
        path = path.substr(0, lastquestionMark);
    
    path = decodeURL(path);
    
    std::cout << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << method << '|' << path << '|' << protocol << std::endl;

    //std::cout << "Path:      " << std::filesystem::canonical(std::filesystem::u8path(rootPath + path)).generic_u8string() << std::endl;
    //std::cout << "Root path: " << std::filesystem::canonical(std::filesystem::u8path(rootPath)).generic_u8string() << std::endl;

    if(!isPathValid(path))
    {
        os << "HTTP/1.1 404 NOT FOUND\r\n\r\n";
        tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });
        return;
    }

    // method selection
    if(method == "GET") 
        GET_(std::move(path), std::move(protocol));
    else if(method == "HEAD") 
        HEAD_(std::move(path), std::move(protocol));
    else if(method == "POST") 
        POST_(std::move(path), std::move(protocol));
    else if(method == "PUT") 
        PUT_(std::move(path), std::move(protocol));
    else if(method == "DELETE") 
        DELETE_(std::move(path), std::move(protocol));
    else if(method == "CONNECT") 
        CONNECT_(std::move(path), std::move(protocol));
    else if(method == "OPTIONS") 
        OPTIONS_(std::move(path), std::move(protocol));
    else if(method == "TRACE")
        TRACE_(std::move(path), std::move(protocol));
    else if(method == "PATCH") 
        PATCH_(std::move(path), std::move(protocol));
    else
    {
        os << "HTTP/1.1 400 Bad Request\r\n\r\n";
        tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });
    }


    /*
    std::cout << std::endl;
    for(const auto& x : requestHeader)
        std::cout << x  << std::endl;
    std::cout << std::endl;
    */
}

void HTTPConnection::sendChunck(std::string path, int offset)
{
    std::fstream file(std::filesystem::u8path(path), std::ios::in | std::ios::binary);

    if(file.good())
    {
        if(offset > 0)
            file.seekg(offset);

        std::ostream os(&buffer);
        os << file.rdbuf();

        file.close();

        if(buffer.size() == 0)
            return;

        tcpSocket.async_send(buffer.data(), 
            boost::bind(&HTTPConnection::onSendChunk, shared_from_this(), path, offset, boost::placeholders::_1, boost::placeholders::_2));
    }
}

void HTTPConnection::onSendChunk(std::string path, int offset, const boost::system::error_code ec, std::size_t bytes_transferred)
{
    if(ec)
    {
        std::cout << "[*] " << ec.message() << std::endl;
        return;
    }

    std::cout << "[*] " << bytes_transferred << " bytes transfered!" << std::endl;
    buffer.consume(buffer.size());
    sendChunck(path, offset + bytes_transferred); 
}

void HTTPConnection::receiveChunck(std::string path)
{
    //tcpSocket.async_receive(boost::asio::buffer(buffer), 
    //        boost::bind(&HTTPConnection::onReceiveChunk, shared_from_this(), path, boost::placeholders::_1, boost::placeholders::_2));
}

void HTTPConnection::onReceiveChunk(std::string path, const boost::system::error_code ec, std::size_t bytes_transferred)
{
    if(ec)
    {
        std::cout << "[*] " << ec.message() << std::endl;
        return;
    }
    else
    {
        std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);
        std::ostream os(&buffer);

        if(file.is_open())
        {
            file << os.rdbuf();
            //buffer.consume(buffer.size());
            receiveChunck(path);
        }
    }
}



void HTTPConnection::GET_(std::string path, std::string protocol)
{
    std::ostream os(&buffer);

    std::string fullPath = rootPath + path;

    std::cout << '{' << fullPath << '}' << std::endl;

    if(std::filesystem::is_directory(std::filesystem::u8path(fullPath)))
    {   
        if(fullPath.back() != '/')
        {
            os << "HTTP/1.1 404 NOT FOUND\r\n\r\n";
                
            tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });

            return;
        }

        if(std::filesystem::exists(std::filesystem::u8path(fullPath + "index.html"))) // File index.html exists
        {
            os << "HTTP/1.1 200 OK\r\n\r\n";

            sendChunck(fullPath + "index.html", -buffer.size());
        }
        else // File index.html doesn't exists
        {
            os << "HTTP/1.1 200 OK\r\n\r\n";
            os << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"></head><body>";
            
            if(fullPath != rootPath)
                os << "<a href=\"../\">[..]</a></br>";

            for(const std::filesystem::directory_entry& x : std::filesystem::directory_iterator(std::filesystem::u8path(fullPath)))
            {
                std::string a = x.path().filename().generic_u8string();

                if(x.is_directory())
                    os << "<a href=\"" << a << "/\">" << a << "</a>" << "</br>";
                else if(x.is_regular_file())
                    os << "<a href=\"" << a << "\">" << a << "</a>" << "</br>";
            }

            os << "</body></html>";
                
            tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });
        }
    }
    else if(std::filesystem::is_regular_file(std::filesystem::u8path(fullPath)))
    {
        std::ifstream file(std::filesystem::u8path(fullPath), std::ios::in | std::ios::binary);

        if(file.is_open())
        {
            os << "HTTP/1.1 200 OK\r\n\r\n";
            file.close();

            sendChunck(fullPath, -buffer.size());
        }
        else // Something went wrong
        {
            os << "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){
                
                });
            std::cout << '^' << "~~~~ ERROR 500 " <<std::string(50, '~') << std::endl;
        }
    }
    else
    {
        std::cout << "File: "  << path << " not found" << std::endl;
        os << "HTTP/1.1 404 NOT FOUND\r\n\r\n";
        tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });
        std::cout << '^' << "~~~~ ERROR 404 " <<std::string(50, '~') << std::endl;
    }
}

void HTTPConnection::HEAD_(std::string path, std::string protocol)
{
    std::ostream os(&buffer);

    if(std::filesystem::is_regular_file(path))
    {
        os << "HTTP/1.1 200 OK\r\n\r\n";
        tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });
    }
    else
    {
        os << "HTTP/1.1 404 NOT FOUND\r\n\r\n";
        tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });
    }
}

void HTTPConnection::POST_(std::string path, std::string protocol)
{
    std::ostream os(&buffer);
    
    if(std::filesystem::is_regular_file(path))
    {
        os << "HTTP/1.1 200 OK\r\n\r\n";
        tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });
    }
    else
    {
        os << "HTTP/1.1 404 NOT FOUND\r\n\r\n";
        tcpSocket.async_send(buffer.data(), 
                [me = shared_from_this()](const boost::system::error_code ec, std::size_t bytes_transferred){

            });
    }
}

void HTTPConnection::PUT_(std::string path, std::string protocol)
{
    std::ostream os(&buffer);
    
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if(std::filesystem::is_regular_file(path))
    {
        os << "HTTP/1.1 200 OK\r\n\r\n";
        tcpSocket.send(buffer.data());
    }
    else
    {
        os << "HTTP/1.1 201 CREATED\r\n\r\n";
        tcpSocket.send(buffer.data());
        //buffer.consume(buffer.size());
        //buffer.prepare(buffer.max_size());
        //receiveChunck(path);
    }
}

void HTTPConnection::DELETE_(std::string path, std::string protocol)
{
    std::ostream os(&buffer);
    
    os << "HTTP/1.1 501 Not Implemented\r\n\r\n";
    size_t n = tcpSocket.send(buffer.data());
    buffer.consume(n);
    
	std::cout << "NOT IMPLEMENTED" << std::endl;
}

void HTTPConnection::CONNECT_(std::string path, std::string protocol)
{
    std::ostream os(&buffer);
    
    os << "HTTP/1.1 501 Not Implemented\r\n\r\n";
    size_t n = tcpSocket.send(buffer.data());
    buffer.consume(n);
    
	std::cout << "NOT IMPLEMENTED" << std::endl;
}

void HTTPConnection::OPTIONS_(std::string path, std::string protocol)
{
    std::ostream os(&buffer);
    
    os << "HTTP/1.1 501 Not Implemented\r\n\r\n";
    size_t n = tcpSocket.send(buffer.data());
    buffer.consume(n);
    
	std::cout << "NOT IMPLEMENTED" << std::endl;
}

void HTTPConnection::TRACE_(std::string path, std::string protocol)
{
    std::ostream os(&buffer);
    
    os << "HTTP/1.1 501 Not Implemented\r\n\r\n";
    size_t n = tcpSocket.send(buffer.data());
    buffer.consume(n);
    
	std::cout << "NOT IMPLEMENTED" << std::endl;
}

void HTTPConnection::PATCH_(std::string path, std::string protocol)
{
    std::ostream os(&buffer);
    
    os << "HTTP/1.1 501 Not Implemented\r\n\r\n";
    size_t n = tcpSocket.send(buffer.data());
    buffer.consume(n);
    
	std::cout << "NOT IMPLEMENTED" << std::endl;
}