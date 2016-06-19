/*
========================================================================
Copyright (c) 2016 Yurko Prokopets(aka YurkoFlisk)

This file is part of Checkers source code

Checkers is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Checkers is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Checkers.If not, see <http://www.gnu.org/licenses/>
========================================================================
*/

// misc.h, version 1.4

#pragma once
#ifndef _MISC_H
#define _MISC_H
#include <string>
#include <exception>

template<typename Value>
class tmp_assign
{
private:
	Value* ptr;
	Value old;
	tmp_assign(void) = delete;
	tmp_assign(const tmp_assign&) = delete;
	tmp_assign& operator=(const tmp_assign&) = delete;
public:
	tmp_assign(Value& value, const Value& assigned)
		: ptr(&value), old(value)
	{
		value = assigned;
	}
	tmp_assign(Value& value, Value&& assigned)
		: ptr(&value), old(std::move(value))
	{
		value = std::move(assigned);
	}
	~tmp_assign(void)
	{
		*ptr = old;
	}
};

enum class error_type : std::int8_t {FATAL_ERROR = 0, WARNING};

class checkers_error: std::exception
{
public:
	checkers_error(void) noexcept
		: msg(""), type(error_type::FATAL_ERROR) {}
	checkers_error(std::string str, error_type t = error_type::FATAL_ERROR) noexcept
		: msg(str), type(t) {}
	virtual ~checkers_error(void) noexcept
	{}
	virtual const char* what(void) const noexcept
	{return msg.c_str();}
	error_type get_error_type(void) const noexcept
	{return type;}
private:
	error_type type;
	std::string msg;
};

#endif