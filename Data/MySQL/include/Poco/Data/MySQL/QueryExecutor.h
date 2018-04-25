//
// QueryExecutor.h
//
// Library: Data/MySQL
// Package: MySQL
// Module:  QueryExecutor
//
// Definition of the QueryExecutor class.
//
// Copyright (c) 2008, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#ifndef Data_MySQL_QueryExecutor_INCLUDED
#define Data_MySQL_QueryExecutor_INCLUDED


#include <mysql.h>
#include <string>
#include <sstream>
#include "Poco/Data/Data.h"


namespace Poco {
namespace Data {
namespace MySQL {


class QueryExecutor
	/// MySQL query executor
{
public:
	typedef void (*Manipulator)(QueryExecutor&);

	QueryExecutor(MYSQL* mysql);

	~QueryExecutor();
		/// Destroys the query.

	QueryExecutor& operator = (const QueryExecutor& query);
		/// Assignment operator.

	void swap(QueryExecutor& other);
		/// Swaps the query with another one.

	template <typename T>
	QueryExecutor& operator << (const T& t)
		/// Concatenates data with the SQL query string.
	{
		_ostr << t;
		return *this;
	}

	QueryExecutor& operator , (Manipulator manip);
		/// Handles manipulators, such as now, etc.

	std::string toString() const;
		/// Creates a string from the accumulated SQL statement.

	void execute();
		/// Exuecute the query.

private:
	QueryExecutor(const QueryExecutor& query);

	MYSQL* _pSessionHandle;
	std::ostringstream _ostr;
};


namespace Keywords {


//
// Manipulators
//

inline void Data_API queryNow(QueryExecutor& query)
	/// Enforces immediate execution of the query.
{
	query.execute();
}


} // namespace Keywords


inline std::string QueryExecutor::toString() const
{
	return _ostr.str();
}


} } } // namespace Poco::Data::MySQL


#endif // QUERYEXECUTOR_H
