//
// url.ipp
// ~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "cinder/Cinder.h"

#include <cstring>
#include <cctype>
#include <cstdlib>
#include <system_error>
#include <iomanip>
#include <sstream>
#include <string>

namespace cinder {
namespace http {

unsigned short Url::port() const
{
  if (!port_.empty())
    return std::atoi(port_.c_str());
  if (protocol_ == "http")
    return 80;
  if (protocol_ == "https")
    return 443;
  if (protocol_ == "ftp")
    return 21;
  return 0;
}

std::string Url::path() const
{
  std::string tmp_path;
  unescape_path(path_, tmp_path);
  return tmp_path;
}
	
void Url::set_protocol( std::string protocol )
{
  protocol_.resize( protocol.length() );
  for (std::size_t i = 0; i < protocol.length(); ++i)
	protocol_[i] = std::tolower(protocol[i]);
}
	
Url& Url::append_path( std::string path )
{
	if(path_.empty() || path_ == "/") {
		if(path.front() != '/')
			set_path("/" + path);
		else
			set_path(path);
	}
	else if(path_.back() == '/' && path.front() == '/') {
		path_.pop_back();
		set_path(path_ + path);
	}
	else if(path_.back() != '/' && path.front() != '/')
		set_path(path_ + "/" + path);
	else {
		// Only one slash.
		set_path(path_ + path);
	}
	return *this;
}
	
Url& Url::add_query( std::string query )
{
	if( ! query_.empty() )
		query_.append( "&" );
	query_.append( query );
	return *this;
}
	
Url& Url::add_query( std::string key, std::string value )
{
	if( ! query_.empty() )
		query_.append( "&" );
	query_.append( key + "=" + value );
	return *this;
}

std::string Url::to_string(int components) const
{
  std::string s;

  if ((components & protocol_component) != 0 && !protocol_.empty())
  {
    s = protocol_;
    s += "://";
  }

  if ((components & user_info_component) != 0 && !user_info_.empty())
  {
    s += user_info_;
    s += "@";
  }

  if ((components & host_component) != 0)
  {
    if (ipv6_host_)
      s += "[";
    s += host_;
    if (ipv6_host_)
      s += "]";
  }

  if ((components & port_component) != 0 && !port_.empty())
  {
    s += ":";
    s += port_;
  }

  if ((components & path_component) != 0 )
  {
	s += !path_.empty() ? path_ : "/";
  }

  if ((components & query_component) != 0 && !query_.empty())
  {
    s += "?";
    s += query_;
  }

  if ((components & fragment_component) != 0 && !fragment_.empty())
  {
    s += "#";
    s += fragment_;
  }

  return s;
}
	
std::string Url::to_escaped_string(int components) const
{
  std::string s;
		
  if ((components & protocol_component) != 0 && !protocol_.empty())
  {
	  s = protocol_;
	  s += "://";
  }
		
  if ((components & user_info_component) != 0 && !user_info_.empty())
  {
	  escape_path( user_info_, s );
	  s += "@";
  }
		
  if ((components & host_component) != 0)
  {
	  if (ipv6_host_)
		  s += "[";
	  s += host_;
	  if (ipv6_host_)
		  s += "]";
  }
		
  if ((components & port_component) != 0 && !port_.empty())
  {
	  s += ":";
	  s += port_;
  }
		
  if ((components & path_component) != 0 )
  {
	  if(!path_.empty())
		  escape_path( path_, s );
	  else
		  s += "/";
  }
		
  if ((components & query_component) != 0 && !query_.empty())
  {
	  s += "?";
	  escape_path( query_, s );
  }
		
  if ((components & fragment_component) != 0 && !fragment_.empty())
  {
	  s += "#";
	  escape_path( fragment_, s );
  }
		
  return s;
}

Url Url::from_string(const char* s, asio::error_code& ec)
{
  Url new_url;

  // Protocol.
  std::size_t length = std::strcspn(s, ":");
  new_url.protocol_.assign(s, s + length);
  for (std::size_t i = 0; i < new_url.protocol_.length(); ++i)
    new_url.protocol_[i] = std::tolower(new_url.protocol_[i]);
  s += length;

  // "://".
  if (*s++ != ':')
  {
    ec = make_error_code(std::errc::invalid_argument);
    return Url();
  }
  if (*s++ != '/')
  {
    ec = make_error_code(std::errc::invalid_argument);
    return Url();
  }
  if (*s++ != '/')
  {
    ec = make_error_code(std::errc::invalid_argument);
    return Url();
  }

  // UserInfo.
  length = std::strcspn(s, "@:[/?#");
  if (s[length] == '@')
  {
    new_url.user_info_.assign(s, s + length);
    s += length + 1;
  }
  else if (s[length] == ':')
  {
    std::size_t length2 = std::strcspn(s + length, "@/?#");
    if (s[length + length2] == '@')
    {
      new_url.user_info_.assign(s, s + length + length2);
      s += length + length2 + 1;
    }
  }

  // Host.
  if (*s == '[')
  {
    length = std::strcspn(++s, "]");
    if (s[length] != ']')
    {
      ec = make_error_code(std::errc::invalid_argument);
      return Url();
    }
    new_url.host_.assign(s, s + length);
    new_url.ipv6_host_ = true;
    s += length + 1;
    if (std::strcspn(s, ":/?#") != 0)
    {
      ec = make_error_code(std::errc::invalid_argument);
      return Url();
    }
  }
  else
  {
    length = std::strcspn(s, ":/?#");
    new_url.host_.assign(s, s + length);
    s += length;
  }

  // Port.
  if (*s == ':')
  {
    length = std::strcspn(++s, "/?#");
    if (length == 0)
    {
      ec = make_error_code(std::errc::invalid_argument);
      return Url();
    }
    new_url.port_.assign(s, s + length);
    for (std::size_t i = 0; i < new_url.port_.length(); ++i)
    {
      if (!std::isdigit(new_url.port_[i]))
      {
        ec = make_error_code(std::errc::invalid_argument);
        return Url();
      }
    }
    s += length;
  }

  // Path.
  if (*s == '/')
  {
    length = std::strcspn(s, "?#");
    new_url.path_.assign(s, s + length);
    std::string tmp_path;
    if (!unescape_path(new_url.path_, tmp_path))
    {
      ec = make_error_code(std::errc::invalid_argument);
      return Url();
    }
    s += length;
  }
  else
    new_url.path_ = "/";

  // Query.
  if (*s == '?')
  {
    length = std::strcspn(++s, "#");
    new_url.query_.assign(s, s + length);
    s += length;
  }

  // Fragment.
  if (*s == '#')
    new_url.fragment_.assign(++s);

  ec = asio::error_code();
  return new_url;
}

Url Url::from_string(const char* s)
{
  asio::error_code ec;
  Url new_url(from_string(s, ec));
  if (ec)
  {
    std::system_error ex(ec);
    throw ex;
  }
  return new_url;
}

Url Url::from_string(const std::string& s, asio::error_code& ec)
{
  return from_string(s.c_str(), ec);
}

Url Url::from_string(const std::string& s)
{
  return from_string(s.c_str());
}

bool Url::unescape_path(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    switch (in[i])
    {
    case '%':
      if (i + 3 <= in.size())
      {
        unsigned int value = 0;
        for (std::size_t j = i + 1; j < i + 3; ++j)
        {
          switch (in[j])
          {
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
            value += in[j] - '0';
            break;
          case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            value += in[j] - 'a' + 10;
            break;
          case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            value += in[j] - 'A' + 10;
            break;
          default:
            return false;
          }
          if (j == i + 1)
            value <<= 4;
        }
        out += static_cast<char>(value);
        i += 2;
      }
      else
        return false;
      break;
    case '-': case '_': case '.': case '!': case '~': case '*':
    case '\'': case '(': case ')': case ':': case '@': case '&':
    case '=': case '+': case '$': case ',': case '/': case ';':
      out += in[i];
      break;
    default:
      if (!std::isalnum(in[i]))
        return false;
      out += in[i];
      break;
    }
  }
  return true;
}
	
bool Url::escape_path(const std::string& in, std::string& out )
{
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;
	
	for ( auto c : in ) {
		switch( c ) {
			case '-': case '_': case '.': case '!': case '~': case '*':
			case '\'': case '(': case ')': case ':': case '@': case '&':
			case '=': case '+': case '$': case ',': case '/': case ';':
				escaped << c;
				break;
			default: {
#if defined( CINDER_MSW ) && ! defined( NDEBUG )
				//	Visual Studio asserts on isalnum( c ) with c < 0 || c > 255 when using CRT Debug.
				//	Unfortunately that forces this differentiation for the test.
				const auto inRange = ! ( c < 0 || c > 255 );
				if( inRange && std::isalnum( c ) ) {
#else
				if( std::isalnum( c ) ) {
#endif
					escaped << c;
				}
				else {
					// Any other characters are percent-encoded
					escaped << std::uppercase;
					escaped << '%' << std::setw(2) << int((unsigned char) c);
					escaped << std::nouppercase;
				}
			}
				break;
		}
	}
	
	out += escaped.str();
	return true;
}

bool operator==(const Url& a, const Url& b)
{
  return a.protocol_ == b.protocol_
    && a.user_info_ == b.user_info_
    && a.host_ == b.host_
    && a.port_ == b.port_
    && a.path_ == b.path_
    && a.query_ == b.query_
    && a.fragment_ == b.fragment_;
}

bool operator!=(const Url& a, const Url& b)
{
  return !(a == b);
}

bool operator<(const Url& a, const Url& b)
{
  if (a.protocol_ < b.protocol_)
    return true;
  if (b.protocol_ < a.protocol_)
    return false;

  if (a.user_info_ < b.user_info_)
    return true;
  if (b.user_info_ < a.user_info_)
    return false;

  if (a.host_ < b.host_)
    return true;
  if (b.host_ < a.host_)
    return false;

  if (a.port_ < b.port_)
    return true;
  if (b.port_ < a.port_)
    return false;

  if (a.path_ < b.path_)
    return true;
  if (b.path_ < a.path_)
    return false;

  if (a.query_ < b.query_)
    return true;
  if (b.query_ < a.query_)
    return false;

  return a.fragment_ < b.fragment_;
}

} // namespace http
} // namespace cinder
