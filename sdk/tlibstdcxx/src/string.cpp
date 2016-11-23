//===------------------------- string.cpp ---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "string"
#include "cstdlib"
#include "cerrno"
#include "limits"
#include "stdexcept"
#ifdef _LIBCPP_MSVCRT
#include "support/win32/support.h"
#endif // _LIBCPP_MSVCRT
#include <stdio.h>

_LIBCPP_BEGIN_NAMESPACE_STD

template class _LIBCPP_CLASS_TEMPLATE_INSTANTIATION_VIS __basic_string_common<true>;

template class _LIBCPP_CLASS_TEMPLATE_INSTANTIATION_VIS basic_string<char>;

template
    string
    operator+<char, char_traits<char>, allocator<char> >(char const*, string const&);

namespace
{

template<typename T>
inline
void throw_helper( const string& msg )
{
#ifndef _LIBCPP_NO_EXCEPTIONS
    throw T( msg );
#else
    fprintf(stderr, "%s\n", msg.c_str());
    _VSTD::abort();
#endif
}

inline
void throw_from_string_out_of_range( const string& func )
{
    throw_helper<out_of_range>(func + ": out of range");
}

inline
void throw_from_string_invalid_arg( const string& func )
{
    throw_helper<invalid_argument>(func + ": no conversion");
}

// as_integer

template<typename V, typename S, typename F>
inline
V
as_integer_helper(const string& func, const S& str, size_t* idx, int base, F f)
{
    typename S::value_type* ptr = nullptr;
    const typename S::value_type* const p = str.c_str();
    typename remove_reference<decltype(errno)>::type errno_save = errno;
    errno = 0;
    V r = f(p, &ptr, base);
    swap(errno, errno_save);
    if (errno_save == ERANGE)
        throw_from_string_out_of_range(func);
    if (ptr == p)
        throw_from_string_invalid_arg(func);
    if (idx)
        *idx = static_cast<size_t>(ptr - p);
    return r;
}

template<typename V, typename S>
inline
V
as_integer(const string& func, const S& s, size_t* idx, int base);

// string
template<>
inline
int
as_integer(const string& func, const string& s, size_t* idx, int base )
{
    // Use long as no Standard string to integer exists.
    long r = as_integer_helper<long>( func, s, idx, base, strtol );
    if (r < numeric_limits<int>::min() || numeric_limits<int>::max() < r)
        throw_from_string_out_of_range(func);
    return static_cast<int>(r);
}

template<>
inline
long
as_integer(const string& func, const string& s, size_t* idx, int base )
{
    return as_integer_helper<long>( func, s, idx, base, strtol );
}

template<>
inline
unsigned long
as_integer( const string& func, const string& s, size_t* idx, int base )
{
    return as_integer_helper<unsigned long>( func, s, idx, base, strtoul );
}

template<>
inline
long long
as_integer( const string& func, const string& s, size_t* idx, int base )
{
    return as_integer_helper<long long>( func, s, idx, base, strtoll );
}

template<>
inline
unsigned long long
as_integer( const string& func, const string& s, size_t* idx, int base )
{
    return as_integer_helper<unsigned long long>( func, s, idx, base, strtoull );
}

// as_float

template<typename V, typename S, typename F> 
inline
V
as_float_helper(const string& func, const S& str, size_t* idx, F f )
{
    typename S::value_type* ptr = nullptr;
    const typename S::value_type* const p = str.c_str();
    typename remove_reference<decltype(errno)>::type errno_save = errno;
    errno = 0;
    V r = f(p, &ptr);
    swap(errno, errno_save);
    if (errno_save == ERANGE)
        throw_from_string_out_of_range(func);
    if (ptr == p)
        throw_from_string_invalid_arg(func);
    if (idx)
        *idx = static_cast<size_t>(ptr - p);
    return r;
}

template<typename V, typename S>
inline
V as_float( const string& func, const S& s, size_t* idx = nullptr );

template<>
inline
float
as_float( const string& func, const string& s, size_t* idx )
{
    return as_float_helper<float>( func, s, idx, strtof );
}

template<>
inline
double
as_float(const string& func, const string& s, size_t* idx )
{
    return as_float_helper<double>( func, s, idx, strtod );
}

template<>
inline
long double
as_float( const string& func, const string& s, size_t* idx )
{
    return as_float_helper<long double>( func, s, idx, strtold );
}

}  // unnamed namespace

int
stoi(const string& str, size_t* idx, int base)
{
    return as_integer<int>( "stoi", str, idx, base );
}

long
stol(const string& str, size_t* idx, int base)
{
    return as_integer<long>( "stol", str, idx, base );
}

unsigned long
stoul(const string& str, size_t* idx, int base)
{
    return as_integer<unsigned long>( "stoul", str, idx, base );
}

long long
stoll(const string& str, size_t* idx, int base)
{
    return as_integer<long long>( "stoll", str, idx, base );
}

unsigned long long
stoull(const string& str, size_t* idx, int base)
{
    return as_integer<unsigned long long>( "stoull", str, idx, base );
}

float
stof(const string& str, size_t* idx)
{
    return as_float<float>( "stof", str, idx );
}

double
stod(const string& str, size_t* idx)
{
    return as_float<double>( "stod", str, idx );
}

long double
stold(const string& str, size_t* idx)
{
    return as_float<long double>( "stold", str, idx );
}

// to_string

namespace
{

// as_string

template<typename S, typename P, typename V >
inline
S
as_string(P sprintf_like, S s, const typename S::value_type* fmt, V a)
{
    typedef typename S::size_type size_type;
    size_type available = s.size();
    while (true)
    {
        int status = sprintf_like(&s[0], available + 1, fmt, a);
        if ( status >= 0 )
        {
            size_type used = static_cast<size_type>(status);
            if ( used <= available )
            {
                s.resize( used );
                break;
            }
            available = used; // Assume this is advice of how much space we need.
        }
        else
            available = available * 2 + 1;
        s.resize(available);
    }
    return s;
}

template <class S, class V, bool = is_floating_point<V>::value>
struct initial_string;

template <class V, bool b>
struct initial_string<string, V, b>
{
    string
    operator()() const
    {
        string s;
        s.resize(s.capacity());
        return s;
    }
};

}  // unnamed namespace

string to_string(int val)
{
    return as_string(snprintf, initial_string<string, int>()(), "%d", val);
}

string to_string(unsigned val)
{
    return as_string(snprintf, initial_string<string, unsigned>()(), "%u", val);
}

string to_string(long val)
{
    return as_string(snprintf, initial_string<string, long>()(), "%ld", val);
}

string to_string(unsigned long val)
{
    return as_string(snprintf, initial_string<string, unsigned long>()(), "%lu", val);
}

string to_string(long long val)
{
    return as_string(snprintf, initial_string<string, long long>()(), "%lld", val);
}

string to_string(unsigned long long val)
{
    return as_string(snprintf, initial_string<string, unsigned long long>()(), "%llu", val);
}

string to_string(float val)
{
    return as_string(snprintf, initial_string<string, float>()(), "%f", val);
}

string to_string(double val)
{
    return as_string(snprintf, initial_string<string, double>()(), "%f", val);
}

string to_string(long double val)
{
    return as_string(snprintf, initial_string<string, long double>()(), "%Lf", val);
}

_LIBCPP_END_NAMESPACE_STD
