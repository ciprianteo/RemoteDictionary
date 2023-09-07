#include <string>
#include <iostream>
#include <map>
#include <mutex>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/asio/streambuf.hpp>

#pragma once

class Request
{
public:
	enum class type
	{
		SET,
		GET,
		STATS
	};

	Request(type t = type::SET, const std::string& key = "", const std::string& value = "")
		:type(t), key(key), value(value)
	{}

	void setValue(const std::string& val)
	{
		value = val;
	}

	void setType(const type& t)
	{
		type = t;
	}

	void setKey(const std::string& k)
	{
		key = k;
	}

	type getType() const
	{
		return type;
	}

	std::string getKey()
	{
		return key;
	}

	std::string getValue() const
	{
		return value;
	}

	friend std::ostream& operator<<(std::ostream& os, const Request& req)
	{
		if (req.type == Request::type::SET)
		{
			return os << "SET " << "Key: " << req.key << " Value: " << req.value << std::endl;
		}
		else
		{
			return os << "GET " << "Key: " << req.key << std::endl;
		}
		 
	}


	void save(boost::asio::streambuf& buff)
	{
		boost::archive::binary_oarchive oa{ buff };
		oa << *this;
	}

	void load(boost::asio::streambuf& buff)
	{
		boost::archive::binary_iarchive ia{ buff };
		ia >> *this;
	}

private:
	friend class boost::serialization::access;

	template <typename Archive>
	friend void serialize(Archive& ar, Request& req, const unsigned int file_version)
	{
		ar& req.type;
		ar& req.key;
		ar& req.value;
	}

	type type;
	std::string key;
	std::string value;
	
};

class Response
{
public:
	enum class status
	{
		SUCCESS,
		FAILURE
	};

	Response(status stat = status::FAILURE, std::string mess = "")
		:status(stat), message(mess)
	{}

	status getStatus() const
	{
		return status;
	}

	std::string getMessage() const
	{
		return message;
	}

	friend std::ostream& operator<<(std::ostream& os, const Response& res)
	{
		return os << (res.status == Response::status::FAILURE ? "FAILURE " : "SUCCESS ") << "Message: " << res.message << std::endl;
	}


	void save(boost::asio::streambuf& buff)
	{
		boost::archive::binary_oarchive oa{ buff };
		oa << *this;
	}

	void load(boost::asio::streambuf& buff)
	{
		boost::archive::binary_iarchive ia{ buff };
		ia >> *this;
	}


private:
	friend class boost::serialization::access;

	template <typename Archive>
	friend void serialize(Archive& ar, Response& resp, const unsigned int file_version)
	{
		ar& resp.status;
		ar& resp.message;
	}

	status status;
	std::string message;
};

class Dictionary
{
public:

	Response set(const std::string& key, const std::string& value)
	{
		const std::lock_guard<std::mutex> lock(mutex);
		auto res = dict.insert(std::make_pair(key, value));
		if (res.second)
		{
			return Response(Response::status::SUCCESS);
		}
		else
		{
			return Response(Response::status::FAILURE);
		}
	}

	Response get(const std::string& key)
	{
		const std::lock_guard<std::mutex> lock(mutex);
		++getOperations;
		auto res = dict.find(key);
		if (res != dict.end())
		{	
			return Response(Response::status::SUCCESS, res->second);
		}
		else
		{
			++failedGet;
			return Response(Response::status::FAILURE, "Key not found!");
		}
	}

	Response stats()
	{
		const std::lock_guard<std::mutex> lock(mutex);
		std::string message = "Total Get operations: " + std::to_string(getOperations) + '\n';
		message += "\tSuccessfull: " + std::to_string(getOperations - failedGet) + '\n';
		message += "\tFailed: " + std::to_string(failedGet) + '\n';

		return Response(Response::status::SUCCESS, message);
	}

private:
	std::mutex mutex;
	std::map<std::string, std::string> dict;
	size_t getOperations = 0;
	size_t failedGet = 0;
};

