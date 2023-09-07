#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/thread.hpp>
#include "Dictionary.h"

using boost::asio::ip::tcp;

class tcp_connection
  // Using shared_ptr and enable_shared_from_this 
  // because we want to keep the tcp_connection object alive 
  // as long as there is an operation that refers to it.
  : public boost::enable_shared_from_this<tcp_connection>
{
public:
  typedef boost::shared_ptr<tcp_connection> pointer;

  static pointer create(boost::asio::io_service& io_service)
  {
    return pointer(new tcp_connection(io_service));
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    read();       
  }

  //~tcp_connection()
  //{
  //  std::cout<<"~tcp_conn"<<std::endl;
  //}

private:
  tcp_connection(boost::asio::io_service& io_service)
    : socket_(io_service)
  {
  }
  
  void read()
  {
    boost::asio::async_read(socket_, readBuf,
        boost::bind(&tcp_connection::handle_read, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void handle_write(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if(error)
    {
      std::cout<<error.message()<<std::endl;
      return;
    }
    std::cout<<bytes_transferred<<std::endl;
  }

  void handle_read(const boost::system::error_code & error,
                   size_t bytes_transferred)
  {
    // if(error)
    // {
    //   std::cout<<error.message()<<std::endl;
    //   return;
    // }

    Request req;
    req.load(readBuf);
    std::cout<< req;

    if(req.getType() == Request::type::SET)
    {
      auto response = dict.set(req.getKey(), req.getValue());
      response.save(writeBuf);
      std::cout<< response;
    }
    else
    {
      auto response = dict.get(req.getKey());
      response.save(writeBuf);
      std::cout<< response;
    }

    boost::asio::async_write(socket_, writeBuf,
                             boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
  }

  tcp::socket socket_;
  std::string m_message;
  boost::asio::streambuf readBuf, writeBuf;
  Dictionary dict;
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_service& service, tcp_connection::pointer new_connection)
      : service_(service), acceptor_(service, tcp::endpoint(tcp::v4(), 80))
  {
    start_accept(new_connection);
  }

private:
  void start_accept(tcp_connection::pointer new_connection)
  {
    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&tcp_server::handle_accept, this, new_connection,
          boost::asio::placeholders::error));
  }
  void handle_accept(tcp_connection::pointer new_connection,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_connection->start();
    }
    else
    {
      std::cout<< error.message() << std::endl;
    }
    start_accept(tcp_connection::create(service_));
  }

  tcp::acceptor acceptor_;
  boost::asio::io_service& service_;
};

int main()
{
  try
  {
    boost::asio::io_service service;;
    tcp_connection::pointer new_connection =
        tcp_connection::create(service);

    tcp_server server(service, new_connection);

    boost::thread t(boost::bind(&boost::asio::io_service::run, &service));
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}

