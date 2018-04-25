//
// QueryExecutor.cpp
//
// Library: Data/MySQL
// Package: MySQL
// Module:  QueryExecutor
//
// Copyright (c) 2008, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Data/MySQL/QueryExecutor.h"
#include "Poco/Data/MySQL/MySQLException.h"


namespace Poco {
namespace Data {
namespace MySQL {


QueryExecutor::QueryExecutor(MYSQL *mysql) : _pSessionHandle(mysql)
{
}


QueryExecutor::~QueryExecutor()
{
}


void QueryExecutor::swap(QueryExecutor &other)
{
	using std::swap;

	swap(_pSessionHandle, other._pSessionHandle);
	swap(_ostr, other._ostr);
}


QueryExecutor& QueryExecutor::operator , (Manipulator manip)
{
	manip(*this);
	return *this;
}


void QueryExecutor::execute()
{
	std::string query = toString();
	int rc = mysql_query(_pSessionHandle, query.c_str());
	if (rc != 0)
	{
		std::string str;
		str += "[Comment]: ";
		str += "mysql_query";

		str += "\t[mysql_error]: ";
		str += mysql_error(_pSessionHandle);

		str += "\t[mysql_errno]: ";
		char buff[30];
		sprintf(buff, "%d", mysql_errno(_pSessionHandle));
		str += buff;

		if (query.length() > 0)
		{
			str += "\t[statemnt]: ";
			str += query;
		}

		throw StatementException(str);
	}
	_ostr.str("");
}


} } } // namespace Poco::Data::MySQL
