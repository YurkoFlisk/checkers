/*
========================================================================
Copyright (c) 2016-2017 Yurko Prokopets(aka YurkoFlisk)

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

// svector.h, version 1.7

#pragma once
#ifndef _SVECTOR_H
#define _SVECTOR_H
#include <algorithm>

template<typename Elem, int Capacity>
class SVector
{
public:
	inline SVector(void);
	inline SVector(const SVector<Elem, Capacity>&);
	inline SVector(int, const Elem&);
	inline SVector<Elem, Capacity>& operator=(const SVector<Elem, Capacity>&);
	inline Elem* begin(void);
	inline Elem* end(void);
	inline const Elem* begin(void) const;
	inline const Elem* end(void) const;
	inline bool empty(void) const noexcept;
	inline void resize(int) noexcept;
	inline int size(void) const noexcept;
	inline void add(const Elem&);
	template<typename... Args>
	inline void emplace(Args...);
	inline void pop(void);
	inline void clear(void) noexcept;
	inline const Elem& front(void) const;
	inline const Elem& back(void) const;
	inline const Elem& operator[](int) const;
	inline Elem& operator[](int);
	inline void swap(SVector<Elem, Capacity>&);
private:
	int siz;
	Elem elems[Capacity];
};

template<typename Elem, int Capacity>
SVector<Elem, Capacity>::SVector(void)
	: siz(0)
{}

template<typename Elem, int Capacity>
inline SVector<Elem, Capacity>::SVector(const SVector<Elem, Capacity>& sv)
	: siz(sv.siz)
{
	std::copy_n(sv.elems, siz, elems);
}

template<typename Elem, int Capacity>
inline SVector<Elem, Capacity>::SVector(int cnt, const Elem& elem)
	: siz(cnt)
{
	std::fill_n(elems, siz, elem);
}

template<typename Elem, int Capacity>
inline SVector<Elem, Capacity>& SVector<Elem, Capacity>::operator=(const SVector<Elem, Capacity>& sv)
{
	siz = sv.siz;
	std::copy_n(sv.elems, sv.siz, elems);
	return *this;
}

template<typename Elem, int Capacity>
inline Elem* SVector<Elem, Capacity>::begin(void)
{
	return elems;
}

template<typename Elem, int Capacity>
inline Elem* SVector<Elem, Capacity>::end(void)
{
	return elems + siz;
}

template<typename Elem, int Capacity>
inline const Elem* SVector<Elem, Capacity>::begin(void) const
{
	return elems;
}

template<typename Elem, int Capacity>
inline const Elem* SVector<Elem, Capacity>::end(void) const
{
	return elems + siz;
}

template<typename Elem, int Capacity>
inline bool SVector<Elem, Capacity>::empty(void) const noexcept
{
	return siz == 0;
}

template<typename Elem, int Capacity>
inline void SVector<Elem, Capacity>::resize(int _siz) noexcept
{
	siz = _siz;
}

template<typename Elem, int Capacity>
inline int SVector<Elem, Capacity>::size(void) const noexcept
{
	return siz;
}

template<typename Elem, int Capacity>
inline void SVector<Elem, Capacity>::add(const Elem& elem)
{
	elems[siz++] = elem;
}

template<typename Elem, int Capacity>
template<typename ...Args>
inline void SVector<Elem, Capacity>::emplace(Args ...args)
{
	new(&(elems[siz++])) Elem{ std::forward<Args>(args)... };
}

template<typename Elem, int Capacity>
inline void SVector<Elem, Capacity>::pop(void)
{
	--siz;
}

template<typename Elem, int Capacity>
inline void SVector<Elem, Capacity>::clear(void) noexcept
{
	siz = 0;
}

template<typename Elem, int Capacity>
inline const Elem& SVector<Elem, Capacity>::front(void) const
{
	return elems[0];
}

template<typename Elem, int Capacity>
inline const Elem& SVector<Elem, Capacity>::back(void) const
{
	return elems[siz - 1];
}

template<typename Elem, int Capacity>
inline const Elem& SVector<Elem, Capacity>::operator[](int idx) const
{
	return elems[idx];
}

template<typename Elem, int Capacity>
inline Elem& SVector<Elem, Capacity>::operator[](int idx)
{
	return elems[idx];
}

template<typename Elem, int Capacity>
inline void SVector<Elem, Capacity>::swap(SVector<Elem, Capacity>& sv)
{
	const int mx = (siz < sv.siz ? sv.siz : siz);
	for (int i = 0; i < mx; ++i)
		std::swap(elems[i], sv.elems[i]);
	std::swap(siz, sv.siz);
}

template<typename Elem, int Capacity>
inline bool operator==(const SVector<Elem, Capacity>& sv1, const SVector<Elem, Capacity>& sv2)
{
	if (sv1.size() != sv2.size())
		return false;
	for (int i = 0; i < sv1.size(); ++i)
		if (sv1[i] != sv2[i])
			return false;
	return true;
}

template<typename Elem, int Capacity>
inline bool operator!=(const SVector<Elem, Capacity>& sv1, const SVector<Elem, Capacity>& sv2)
{
	return !(sv1 == sv2);
}

template<typename Elem, int Capacity>
inline void swap(SVector<Elem, Capacity>& sv1, SVector<Elem, Capacity>& sv2)
{
	sv1.swap(sv2);
}

#endif