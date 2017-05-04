//
//  StringUtils.h
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/11/17.
//
//

#ifndef Strings_h
#define Strings_h

#include <ctype.h>
#include <fstream>
#include <set>
#include <sstream>
#include <stdarg.h>
#include <string>
#include <vector>

#include "MathHelpers.hpp"

/**
	@file strings.h
	Basic string operations, like split(), join(), etc etc.
 */

/**
	@defgroup stringutil String Utilities
 */

/**
	@namespace core::strings
	@brief core::util::strings is a toolbox of convenient string-related functions
 */
namespace core {

	/**
	 @brief Convert @a v to a string
	 @return the input parameter as a string.
	 */
	template< class T >
	std::string str( T v )
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	/**
	 @brief Convert @a v to a string
	 @return the input parameter as a string with real precision @a prec
	 */
	template< class T >
	std::string str( T v, int prec )
	{
		std::stringstream ss;
		ss << std::setprecision( prec ) << std::fixed << v;
		return ss.str();
	}

	/**
	 @brief Convert pointer value @a v to a string
	 */
	template< class T >
	std::string str( T *v )
	{
		std::stringstream ss;
		ss << "0x" << std::hex << v;
		return ss.str();
	}

	/**
	 @brief Convert a boolean to string
	 @return "true" or "false" according to @a v
	 */
	inline std::string str( bool v )
	{
		return v ? "true" : "false";
	}

	/**
	 @brief Convert a const chararacter pointer to a string
	 */
	inline std::string str( const char *s )
	{
		return std::string( s );
	}

	/**
	 @brief Convert a non-const chararacter pointer to a string
	 */
	inline std::string str( char *s )
	{
		return std::string( s );
	}

	/**
	 @brief identity function string->string
	 */
	inline std::string str( const std::string &s )
	{
		return s;
	}


	namespace strings {

	typedef std::vector< std::string > stringvec;
	typedef std::vector< char > charvec;
	typedef std::set< std::string > stringset;
	typedef std::set< char > charset;

	enum numeric_base { Binary, Octal, Decimal, Hexadecimal };

	/**
	 @brief generate a lowercase version of a string
	 @ingroup stringutil
	 @return a lowercased version of @a str
	 */
	inline std::string
	lowercase( const std::string &str )
	{
		using namespace std;

		string out;
		std::size_t l = str.length();
		for (int i = 0; i < l; i++)
			out.push_back( tolower( str[i] ) );

		return out;
	}


	/**
	 @brief generate an uppercase version of a string
	 @ingroup stringutil
	 @return an uppercased version of @a str
	 */
	inline std::string
	uppercase( const std::string &str )
	{
		using namespace std;

		string out;
		std::size_t l = str.length();
		for (int i = 0; i < l; i++)
			out.push_back( toupper( str[i] ) );

		return out;
	}


	/**
	 @brief Variant of sprintf for std::string
	 @ingroup stringutil

	 works like sprintf, but is reasonably "safe"

	 @note You're better off using std::stringstream and overloaded insertion operators.
	 */
	inline std::string format( const char *format, ... )
	{
		using namespace std;

		char *buffer = NULL;
		va_list ap;
		va_start( ap, format );
		vasprintf( &buffer, format, ap );
		va_end( ap );

		string str( buffer );
		free( buffer );

		return str;
	}

	/**
	 @brief multi replace for std::string
	 @ingroup stringutil
	 replaces all instances of @a token in @a src with @a with
	 */
	inline std::string
	replaceAll( const std::string &src, const std::string &token, const std::string &with )
	{
		using namespace std;

		string out = src;

		while ( true )
		{
			string::size_type position = out.find(token);
			if ( position == string::npos ) break;

			//if we're here there's a hash to remove
			out.replace(position, token.length(), with);
		}

		return out;
	}

	/**
	 @brief case-insensitive multi replace for std::string
	 @ingroup stringutil
	 replaces all instances of @a token in @a src with @a with, case insensitive
	 */
	inline std::string
	iReplaceAll( const std::string &src, const std::string &token,  const std::string &with )
	{
		using namespace std;

		string lSrc = lowercase( src );
		string lToken = lowercase( token );
		string ret = src;

		while ( true )
		{
			string::size_type position = lSrc.find(lToken);
			if ( position == string::npos ) break;

			ret.replace( position, token.length(), with);
			lSrc.replace( position, token.length(), with );
		}

		return ret;
	}

	/**
	 @brief string tokenizer
	 @ingroup stringutil
	 splits @a str by @a delimeter.
	 @return the tokens between the delimeter, minus the delimeter.
	 */
	inline stringvec
	split( const std::string &str, const std::string &delimeter )
	{
		using namespace std;

		stringvec tokens;

		size_t start = 0, next = string::npos;
		while (true)
		{
			bool done = false;
			string ss; //substring
			next = str.find(delimeter, start);
			if (next != string::npos)
			{
				ss = str.substr(start, next - start);
			}
			else
			{
				ss = str.substr(start, (str.length() - start));
				done = true;
			}

			tokens.push_back(ss);
			if (done) break;

			start = next + delimeter.length();
		}

		return tokens;
	}

	/**
	 @brief string tokenizer
	 @ingroup stringutil
	 @return the tokens between the delimeter, minus the delimeter.
	 */
	inline stringvec
	split( const std::string &str, char delimeter)
	{
		using namespace std;

		stringvec tokens;

		size_t start = 0, next = string::npos;
		while (true)
		{
			bool done = false;
			string ss; //substring
			next = str.find(delimeter, start);
			if (next != string::npos)
			{
				ss = str.substr(start, next - start);
			}
			else
			{
				ss = str.substr(start, (str.length() - start));
				done = true;
			}

			tokens.push_back(ss);
			if (done) break;

			start = next + 1;
		}

		return tokens;
	}

	/**
	 @brief string tokenizer
	 @ingroup stringutil
	 @param str The string to tokenize
	 @param delimeters the character delimeters you're splitting against
	 @param includeDelimeters if true, the returned vector will include the delimeters between tokens

	 @returns the tokens between the delimeter chars, separated by delimeters if @a includeDelimeters is true
	 */
	inline stringvec
	split( const std::string &str, const charset &delimeters, bool includeDelimeters = true )
	{
		using namespace std;

		string currentToken;
		stringvec tokens;
		charset::const_iterator notFound( delimeters.end() );

		for ( string::const_iterator it( str.begin() ), end( str.end() ); it != end; ++it )
		{
			char c(*it);
			if ( delimeters.find( c ) == notFound )
			{
				currentToken.append( 1u, c );
			}
			else
			{
				tokens.push_back( currentToken );
				if ( includeDelimeters ) tokens.push_back( string( 1u, c ));
				currentToken.clear();
			}
		}

		/*
		 Append whatever's lingering around
		 */
		if ( !currentToken.empty() ) tokens.push_back( currentToken );

		return tokens;
	}

	/**
	 @brief string tokenizer
	 @ingroup stringutil
	 @returns the tokens and delimeters ina vector

	 If your input string is "The Quick Brown Fox Jumped Over The Lazy Dog"
	 and your delimeters are {" ", "The" } the result vector would be:
	 { "The", " ", "Quick", " ", "Brown", " ", "Fox", " ", "Jumped",
	 " ", "Over", " ", "The", " ", "Lazy", " ", "Dog" }

	 */
	inline stringvec
	split( const std::string &str, const stringset &delimeters )
	{
		using namespace std;

		stringvec tokens;
		string nonDelimeter;

		size_t start = 0, next = string::npos;
		while (true)
		{
			string delim;

			for (stringset::const_iterator it = delimeters.begin(), end( delimeters.end() ); it != end; ++it)
			{
				next = str.find(*it, start);
				if (next == start)
				{
					delim = *it;
					break; //get out
				}
			}

			//if we found a delimeter
			if (delim.length())
			{
				start += delim.length();

				if (nonDelimeter.length())
				{
					tokens.push_back(nonDelimeter);
					nonDelimeter.clear();
				}

				tokens.push_back(delim);
			}
			else
			{
				nonDelimeter.push_back(str[start]);

				start++;
				if (start >= str.length()) break;
			}
		}

		return tokens;
	}

	/**
	 @brief string concatenation
	 @ingroup stringutil
	 @return each string in @a tokens with the string @a delimeter in between as one string
	 */
	inline std::string
	join( const stringvec &tokens, const std::string &delimeter )
	{
		using namespace std;

		string joined;
		for ( stringvec::const_iterator it = tokens.begin(); it != tokens.end(); ++it )
		{
			joined += *it;

			stringvec::const_iterator next = it;
			++next;
			if (next != tokens.end())
				joined += delimeter;
		}

		return joined;
	}

	template<typename T>
	std::string join( const std::vector<T> &tokens, const std::string &delimeter) {
		using namespace std;

		string joined;
		for ( auto it = tokens.begin(); it != tokens.end(); ++it )
		{
			joined += str(*it);

			auto next = it;
			++next;
			if (next != tokens.end())
				joined += delimeter;
		}

		return joined;
	}

	/**
	 @brief Strip whitespace from left side of string
	 @ingroup stringutil
	 @return @a str with whitespace stripped from the left side
	 */
	inline std::string
	lStrip( const std::string &str )
	{
		using namespace std;

		//now, cut out trailing whitespace
		size_t start = 0;
		while (isspace(str[start]))
		{
			start++;
		}

		//copy back into line
		return str.substr(start, str.size() - start);
	}

	/**
	 @brief Strip whitespace from right side of string
	 @ingroup stringutil
	 @return @a str with whitespace stripped from the right side
	 */
	inline std::string
	rStrip( const std::string &str )
	{
		using namespace std;

		//now, cut out trailing whitespace
		size_t end = str.size() - 1;
		while (isspace(str[end]))
		{
			end--;
		}

		//copy back into line
		return str.substr(0, end + 1);
	}

	/**
	 @brief Strip whitespace from both sides of string
	 @ingroup stringutil
	 @return @a str with whitespace stripped from both sides
	 */
	inline std::string
	strip( const std::string &str )
	{
		return rStrip(lStrip(str));
	}

	/**
	 @brief determine if a string contains a character
	 @ingroup stringutil
	 @return true if @a str contains @a c
	 */
	inline bool
	contains( const std::string &str, char c)
	{
		for (int i = 0; i < (int)str.length(); i++)
			if (str[i] == c) return true;

		return false;
	}

	/**
	 @brief determine if a string contains a string
	 @ingroup stringutil
	 @return true if @a str contains @a substr
	 */
	inline bool
	contains( const std::string &str, const std::string &substr)
	{
		return (str.find( substr ) != std::string::npos );
	}

	/**
	 @brief Determine if a string begins with a particular string
	 @ingroup stringutil
	 @return true if @a str begins with @a substr
	 */
	inline bool
	startsWith( const std::string &str, const std::string &substr )
	{
		return (str.find( substr ) == 0 );
	}

	/**
	 @brief Determine if a string ends with a particular string
	 @ingroup stringutil
	 @return true if @a str ends with @a substr
	 */
	inline bool
	endsWith( const std::string &str, const std::string &substr )
	{
		return (str.find( substr ) == str.length() - substr.length() );
	}

	/**
	 @brief find a "closing symbol" in a string.
	 @ingroup stringutil

	 Finds corresponding closing paren for a paren at a given position.
	 @return -1 if there isn't one. Will still work if @a openPos is after
	 the opening paren, but before any other opening parens.

	 given parenthetized string "( 1 + ( 3 / 4 ))"

	 @verbatim
	 openPos: 0						return: 15
	 openPos: 1,2,3,4,5				return: 15
	 openPos: 6						return: 14
	 openPos: 7,8,9,10,11,12,13		return: 14
	 openPos: 15						return: -1
	 @endverbatim

	 @a openSymbol is the open delimeter. Function knows that for:
	 @verbatim
		open: '(' 	close is: ')'
		open: '['	close is: ']'
		open: '{'	close is: '}'
		open: '<'	close is: '>'
		open: '"'   close is: '"'
		open: '     close is: '
	 @endverbatim
	 */
	inline int
	findClosingSymbol( const std::string &str, int openPos, char openSymbol = '(')
	{
		using namespace std;

		char closeSymbol = '0';
		switch (openSymbol)
		{
			case '(':
				closeSymbol = ')';
				break;

			case '[':
				closeSymbol = ']';
				break;

			case '{':
				closeSymbol = '}';
				break;

			case '<':
				closeSymbol = '>';
				break;

			case '"':
				closeSymbol = '"';

			default:
				return -1;
		}

		int sum, i;

		i = -1;
		sum = 0;
		for (i = openPos; i < (int)str.length(); i++)
		{
			if ( str[i] == openSymbol ) { sum++; continue; }
			if ( str[i] == closeSymbol ) sum--;

			//now where are we?
			if (sum == 0) break;
		}

		return i;
	}


	/**
	 @brief find a "closing symbol" in a stringvec.

	 Finds corresponding closing paren for a paren at a given position.
	 @return -1 if there isn't one. Will still work if @a openPos is after
	 the opening paren, but before any other opening parens.

	 given parenthetized string "( 1 + ( 3 / 4 ))"

	 @verbatim
	 openPos: 0						return: 15
	 openPos: 1,2,3,4,5				return: 15
	 openPos: 6						return: 14
	 openPos: 7,8,9,10,11,12,13		return: 14
	 openPos: 15						return: -1
	 @endverbatim

	 @a openSymbol is the open delimeter. Function knows that for:
	 @verbatim
		open: '(' 	close is: ')'
		open: '['	close is: ']'
		open: '{'	close is: '}'
		open: '<'	close is: '>'
		open: '"'   close is: '"'
		open: '     close is: '
	 @endverbatim

	 */
	inline int findClosingSymbol( const stringvec &strlist, int openPos, const std::string &openSymbol = "(")
	{
		using namespace std;

		string closeSymbol = "0";
		int sum = 0, i = -1;

		if (openSymbol == "(") closeSymbol = ")";
		else if (openSymbol == "[") closeSymbol = "]";
		else if (openSymbol == "{") closeSymbol = "}";
		else if (openSymbol == "<") closeSymbol = ">";
		else if (openSymbol == "\"" ) closeSymbol = "\"";
		else if (openSymbol == "'" ) closeSymbol = "'";
		else
		{
			return -1;
		}

		for (i = openPos; i < (int)strlist.size(); i++)
		{
			if ( strlist[i] == openSymbol ) { sum++; continue; }
			else if ( strlist[i] == closeSymbol ) sum--;

			//now where are we?
			if (sum == 0) break;
		}

		return i;
	}

	/**
	 @brief remove all characters in an input string from another string
	 @ingroup stringutil
	 @return a string made up of @a str, but with all chars in @a set cut out.
	 */
	inline std::string
	removeSet( const std::string &str, const std::string &set )
	{
		using namespace std;

		string out;
		std::size_t l = str.length();
		for (int i = 0; i < l; i++)
		{
			if ( set.find( str[i] ) == string::npos )
				out.push_back( str[i] );
		}

		return out;
	}


	/**
	 @brief remove a subset of a string
	 @ingroup stringutil
	 @return str with the substring starting at @a startingAt @a charCount long cut out
	 */
	inline std::string
	remove( const std::string &str, int startingAt, int charCount )
	{
		using namespace std;

		string out;
		for (int i = 0; i < startingAt; i++)
			out.push_back( str[i] );

		for (int i = startingAt + charCount; i < (int)str.length(); i++)
			out.push_back( str[i] );

		return out;
	}

	/**
	 @brief insert a subset of a string into another string
	 @ingroup stringutil
	 */
	inline void
	moveInto( std::string &src, std::string &dest, int startingAt, int charCount )
	{
		using namespace std;

		for (int i = startingAt; i < startingAt + charCount; i++)
		{
			if ( i < (int)src.length() )
				dest.push_back( src[i] );
		}

		src = remove( src, startingAt, charCount );
	}

	/**
	 @brief convert a canonical four-byte int code to a string.
	 @ingroup stringutil

	 Converts a canonical int-code to string; such that:
	 @code
	 const unsigned int foobar = 'APPL';
	 cout << intToString( foobar ) << endl;
	 @endcode

	 Would print APPL.
	 */
	inline std::string
	intCodeToString( unsigned int code )
	{
		char buf[5];
		buf[0] = (char)((code & 0xFF000000) >> 24);
		buf[1] = (char)((code & 0x00FF0000) >> 16);
		buf[2] = (char)((code & 0x0000FF00) >> 8);
		buf[3] = (char)( code & 0x000000FF);
		buf[4] = '\0';

		return std::string(buf);
	}

	/**
	 @brief calculate a hash code for a string.
	 @ingroup stringutil
	 @return a hash-code for a string @a s.
	 @note the hash algorithm is adapted from java.lang.String
	 */
	inline std::size_t
	hash( const std::string &s )
	{
		std::size_t n = s.length(), h = 0;
		for ( std::size_t i = 0; i < n; i++ )
		{
			h += s[i] * static_cast<size_t>(powi( 31, static_cast<int>(n - 1 - i)));
		}

		h += s[n - 1];

		return h;
	}

	inline std::string
	readFile( const std::string &filename )
	{
		using namespace std;

		fstream file( filename.c_str(), ios::in | ios::binary | ios::ate );
		long size = file.tellg();
		file.seekg(0, ios::beg);

		if (size <= 0)
		{
			return std::string();
		}

		char *buf = new char[size];
		file.read(buf, size);
		file.close();

		string text( buf, size );
		delete [] buf;

		return text;
	}


	inline stringvec
	readFileIntoLines( const std::string &filename )
	{
		std::string buf = readFile( filename );
		if ( buf.size() == 0 ) return stringvec();

		stringvec lines = split( buf, '\n' );
		
		for ( stringvec::iterator line( lines.begin() ), end( lines.end() ); line != end; ++line )
		{
			*line = strip( *line );
		}
		
		return lines;
	}
	
}} // end namespace core::strings


#endif /* Strings_h */
