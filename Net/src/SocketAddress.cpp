//
// SocketAddress.cpp
//
// $Id: //poco/1.4/Net/src/SocketAddress.cpp#5 $
//
// Library: Net
// Package: NetCore
// Module:  SocketAddress
//
// Copyright (c) 2005-2011, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/IPAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/Net/DNS.h"
#include "Poco/RefCountedObject.h"
#include "Poco/NumberParser.h"
#include "Poco/NumberFormatter.h"
#include "Poco/BinaryReader.h"
#include "Poco/BinaryWriter.h"
#include <algorithm>
#include <cstring>


using Poco::RefCountedObject;
using Poco::NumberParser;
using Poco::UInt16;
using Poco::InvalidArgumentException;
using Poco::Net::Impl::SocketAddressImpl;
using Poco::Net::Impl::IPv4SocketAddressImpl;
#ifdef POCO_HAVE_IPv6
using Poco::Net::Impl::IPv6SocketAddressImpl;
#endif
#ifdef POCO_OS_FAMILY_UNIX
using Poco::Net::Impl::LocalSocketAddressImpl;
#endif


namespace Poco {
namespace Net {


struct AFLT
{
	bool operator () (const IPAddress& a1, const IPAddress& a2)
	{
		return a1.af() < a2.af();
	}
};


//
// SocketAddress
//


SocketAddress::SocketAddress()
{
	newIPv4();
}


SocketAddress::SocketAddress(const IPAddress& hostAddress, Poco::UInt16 portNumber)
{
	init(hostAddress, portNumber);
}


SocketAddress::SocketAddress(Poco::UInt16 portNumber)
{
	init(IPAddress(), portNumber);
}


SocketAddress::SocketAddress(const std::string& hostAddress, Poco::UInt16 portNumber)
{
	init(hostAddress, portNumber);
}


SocketAddress::SocketAddress(const std::string& hostAddress, const std::string& portNumber)
{
	init(hostAddress, resolveService(portNumber));
}


SocketAddress::SocketAddress(Family family, const std::string& addr)
{
	init(family, addr);
}


SocketAddress::SocketAddress(const std::string& hostAndPort)
{
	init(hostAndPort);
}


SocketAddress::SocketAddress(const SocketAddress& socketAddress)
{
	if (socketAddress.family() == IPv4)
		newIPv4(reinterpret_cast<const sockaddr_in*>(socketAddress.addr()));
#if defined(POCO_HAVE_IPv6)
	else if (socketAddress.family() == IPv6)
		newIPv6(reinterpret_cast<const sockaddr_in6*>(socketAddress.addr()));
#endif
#if defined(POCO_OS_FAMILY_UNIX)
	else if (socketAddress.family() == UNIX_LOCAL)
		newLocal(reinterpret_cast<const sockaddr_un*>(socketAddress.addr()));
#endif
}


SocketAddress::SocketAddress(const struct sockaddr* sockAddr, poco_socklen_t length)
{
	if (length == sizeof(struct sockaddr_in))
		newIPv4(reinterpret_cast<const struct sockaddr_in*>(sockAddr));
#if defined(POCO_HAVE_IPv6)
	else if (length == sizeof(struct sockaddr_in6))
		newIPv6(reinterpret_cast<const struct sockaddr_in6*>(sockAddr));
#endif
#if defined(POCO_OS_FAMILY_UNIX)
	else if (length > 0 && length <= sizeof(struct sockaddr_un) && sockAddr->sa_family == AF_UNIX)
		newLocal(reinterpret_cast<const sockaddr_un*>(sockAddr));
#endif
	else throw Poco::InvalidArgumentException("Invalid address length passed to SocketAddress()");
}


SocketAddress::~SocketAddress()
{
	destruct();
}


bool SocketAddress::operator < (const SocketAddress& socketAddress) const
{
	if (family() < socketAddress.family()) return true;
	if (family() > socketAddress.family()) return false;
#if defined(POCO_OS_FAMILY_UNIX)
	if (family() == UNIX_LOCAL) return toString() < socketAddress.toString();
#endif
	if (host() < socketAddress.host()) return true;
	if (host() > socketAddress.host()) return false;
	return (port() < socketAddress.port());
}


SocketAddress& SocketAddress::operator = (const SocketAddress& socketAddress)
{
	if (&socketAddress != this)
	{
		destruct();
		if (socketAddress.family() == IPv4)
			newIPv4(reinterpret_cast<const sockaddr_in*>(socketAddress.addr()));
#if defined(POCO_HAVE_IPv6)			
		else if (socketAddress.family() == IPv6)
			newIPv6(reinterpret_cast<const sockaddr_in6*>(socketAddress.addr()));
#endif
#if defined(POCO_OS_FAMILY_UNIX)
		else if (socketAddress.family() == UNIX_LOCAL)
			newLocal(reinterpret_cast<const sockaddr_un*>(socketAddress.addr()));
#endif
	}
	return *this;
}


IPAddress SocketAddress::host() const
{
	return pImpl()->host();
}


Poco::UInt16 SocketAddress::port() const
{
	return ntohs(pImpl()->port());
}


poco_socklen_t SocketAddress::length() const
{
	return pImpl()->length();
}


const struct sockaddr* SocketAddress::addr() const
{
	return pImpl()->addr();
}


int SocketAddress::af() const
{
	return pImpl()->af();
}


SocketAddress::Family SocketAddress::family() const
{
	return static_cast<Family>(pImpl()->family());
}


std::string SocketAddress::toString() const
{
	return pImpl()->toString();
}


void SocketAddress::init(const IPAddress& hostAddress, Poco::UInt16 portNumber)
{
	if (hostAddress.family() == IPAddress::IPv4)
		newIPv4(hostAddress, portNumber);
#if defined(POCO_HAVE_IPv6)
	else if (hostAddress.family() == IPAddress::IPv6)
		newIPv6(hostAddress, portNumber);
#endif
	else throw Poco::NotImplementedException("unsupported IP address family");
}


void SocketAddress::init(const std::string& hostAddress, Poco::UInt16 portNumber)
{
	IPAddress ip;
	if (IPAddress::tryParse(hostAddress, ip))
	{
		init(ip, portNumber);
	}
	else
	{
		HostEntry he = DNS::hostByName(hostAddress);
		HostEntry::AddressList addresses = he.addresses();
		if (addresses.size() > 0)
		{
#if defined(POCO_HAVE_IPv6)
			// if we get both IPv4 and IPv6 addresses, prefer IPv4
			std::sort(addresses.begin(), addresses.end(), AFLT());
#endif
			init(addresses[0], portNumber);
		}
		else throw HostNotFoundException("No address found for host", hostAddress);
	}
}


void SocketAddress::init(Family family, const std::string& address)
{
	if (family == UNIX_LOCAL)
	{
		newLocal(address);
	}
	else
	{
		init(address);
	}
}


void SocketAddress::init(const std::string& hostAndPort)
{
	std::string host;
	std::string port;
	std::string::const_iterator it  = hostAndPort.begin();
	std::string::const_iterator end = hostAndPort.end();
	if (*it == '[')
	{
		++it;
		while (it != end && *it != ']') host += *it++;
		if (it == end) throw InvalidArgumentException("Malformed IPv6 address");
		++it;
	}
	else
	{
		while (it != end && *it != ':') host += *it++;
	}
	if (it != end && *it == ':')
	{
		++it;
		while (it != end) port += *it++;
	}
	else throw InvalidArgumentException("Missing port number");
	init(host, resolveService(port));
}


Poco::UInt16 SocketAddress::resolveService(const std::string& service)
{
	unsigned port;
	if (NumberParser::tryParseUnsigned(service, port) && port <= 0xFFFF)
	{
		return (UInt16) port;
	}
	else
	{
#if defined(POCO_VXWORKS)
		throw ServiceNotFoundException(service);
#else
		struct servent* se = getservbyname(service.c_str(), NULL);
		if (se)
			return ntohs(se->s_port);
		else
			throw ServiceNotFoundException(service);
#endif
	}
}


} } // namespace Poco::Net
