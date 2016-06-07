// Checkers
// By Yurko Prokopets(aka YurkoFlisk)
// misc.h
// Version 1.3

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