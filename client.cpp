#pragma once

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <future>
#include "Dictionary.h"

using boost::asio::ip::tcp;

class ClientConnection
{
public:
	ClientConnection(boost::asio::io_service& io_service, tcp::resolver::iterator endpoint_iterator)
		: socket_(io_service), io_service_(io_service)
	{
		tcp::endpoint endpoint = *endpoint_iterator;
		socket_.async_connect(endpoint,
			boost::bind(&ClientConnection::handleConnect, this,
				boost::asio::placeholders::error, ++endpoint_iterator));
	}
	//~ClientConnection()
	//{
	//	std::cout << "~ClientConnection" << std::endl;
	//}

	void handleConnect(const boost::system::error_code& error,
		tcp::resolver::iterator endpoint_iterator)
	{
		if (!error)
		{
			io_service_.post(boost::bind(&ClientConnection::sendRequest, this));
		}
		else if (endpoint_iterator != tcp::resolver::iterator())
		{
			socket_.close();
			tcp::endpoint endpoint = *endpoint_iterator;
			socket_.async_connect(endpoint,
				boost::bind(&ClientConnection::handleConnect, this,
					boost::asio::placeholders::error, ++endpoint_iterator));
		}
	}

	void sendRequest()
	{
		boost::system::error_code error;
		boost::asio::streambuf buf;
		Request req;
		if (createRequest(req))
		{
			req.save(buf);
			boost::asio::async_write(socket_, buf, boost::bind(&ClientConnection::handleWrite, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			//std::cout << "scoket is open: " << socket_.is_open() << std::endl;
		}
	}

	void receiveResponse()
	{
		boost::asio::async_read(socket_, readBuf,
			boost::bind(&ClientConnection::handleRead, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}

	void handleWrite(const boost::system::error_code& error, std::size_t bytes_transferred)
	{
		if (!error)
		{
			io_service_.post(boost::bind(&ClientConnection::receiveResponse, this));
			std::cout << "Written: " << bytes_transferred << '\n';
		}
		else
		{
			std::cout << error.message() << std::endl;
		}
	}

	void handleRead(const boost::system::error_code& error, std::size_t bytes_transferred)
	{
		Response res;
		res.load(readBuf);

		std::cout << res;
	}

private:


	bool createRequest(Request& req)
	{
		int choice = -1;
		std::string key, value;

		std::cout << "1. SET\n"
			<< "2. GET\n"
			<< "3. STATS\n";

		std::cin >> choice;
		if (choice < 1 || choice > 3)
		{
			return false;
		}

		std::cout << "Key:\n";
		std::cin >> key;
		req.setKey(key);

		if (choice == 1)
		{
			std::cout << "Value:\n";
			std::cin >> value;

			req.setType(Request::type::SET);
			req.setValue(value);
		}
		else if (choice == 2)
		{
			req.setType(Request::type::GET);
		}
		else
		{
			req.setType(Request::type::STATS);
		}

		return true;
	}

	tcp::socket socket_;
	boost::asio::io_service& io_service_;
	boost::asio::streambuf readBuf;
};

int main(int argc, char* argv[])
{
	try
	{
		// if (argc != 2)
		// {
		//   std::cerr << "Usage: client <host>" << std::endl;
		//   return 1;
		// }

		// Any program that uses asio need to have at least one io_service object
		boost::asio::io_service io_service;

		// Convert the server name that was specified as a parameter to the application, to a TCP endpoint.
		// To do this, we use an ip::tcp::resolver object.
		tcp::resolver resolver(io_service);

		// A resolver takes a query object and turns it into a list of endpoints.
		// We construct a query using the name of the server, specified in argv[1],
		// and the name of the service.
		tcp::resolver::query query("127.0.0.1", "80");

		// The list of endpoints is returned using an iterator of type ip::tcp::resolver::iterator.
		// A default constructed ip::tcp::resolver::iterator object can be used as an end iterator.
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

		boost::shared_ptr<ClientConnection> client(new ClientConnection(io_service, endpoint_iterator));
		boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
		t.join();
	}
	// handle any exceptions that may have been thrown.
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
