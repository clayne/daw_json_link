// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//
// See cookbook/class.md for the 2nd example
//

#include "defines.h"

#include "daw/json/daw_json_link.h"

#include <daw/daw_read_file.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

namespace daw::cookbook_class2 {
	struct MyClass1 {
		std::string member_0;
		int member_1;
		bool member_2;
	};

	bool operator==( MyClass1 const &lhs, MyClass1 const &rhs ) {
		return std::tie( lhs.member_1, lhs.member_2 ) ==
		       std::tie( rhs.member_1, rhs.member_2 );
	}

	struct MyClass2 {
		MyClass1 a;
		unsigned b;
	};

	bool operator==( MyClass2 const &lhs, MyClass2 const &rhs ) {
		return std::tie( lhs.a, lhs.b ) == std::tie( rhs.a, rhs.b );
	}
} // namespace daw::cookbook_class2

namespace daw::json {
	template<>
	struct json_data_contract<daw::cookbook_class2::MyClass1> {
#if defined( DAW_JSON_CNTTP_JSON_NAME )
		using type =
		  json_member_list<json_string<"member0">, json_number<"member1", int>,
		                   json_bool<"member2">>;
#else
		static constexpr char const member0[] = "member0";
		static constexpr char const member1[] = "member1";
		static constexpr char const member2[] = "member2";
		using type =
		  json_member_list<json_string<member0>, json_number<member1, int>,
		                   json_bool<member2>>;
#endif
		static inline auto
		to_json_data( daw::cookbook_class2::MyClass1 const &value ) {
			return std::forward_as_tuple( value.member_0, value.member_1,
			                              value.member_2 );
		}
	};

	template<>
	struct json_data_contract<daw::cookbook_class2::MyClass2> {
#if defined( DAW_JSON_CNTTP_JSON_NAME )
		using type =
		  json_member_list<json_class<"a", daw::cookbook_class2::MyClass1>,
		                   json_number<"b", unsigned>>;
#else
		static constexpr char const a[] = "a";
		static constexpr char const b[] = "b";
		using type = json_member_list<json_class<a, daw::cookbook_class2::MyClass1>,
		                              json_number<b, unsigned>>;
#endif
		static inline auto
		to_json_data( daw::cookbook_class2::MyClass2 const &value ) {
			return std::forward_as_tuple( value.a, value.b );
		}
	};
} // namespace daw::json

int main( int argc, char **argv )
#if defined( DAW_USE_EXCEPTIONS )
  try
#endif
{
	if( argc <= 1 ) {
		puts( "Must supply path to cookbook_class2.json file\n" );
		exit( EXIT_FAILURE );
	}
	auto data = daw::read_file( argv[1] ).value( );

	auto const cls = daw::json::from_json<daw::cookbook_class2::MyClass2>( data );

	test_assert( cls.a.member_0 == "this is a test", "Unexpected value" );
	test_assert( cls.a.member_1 == 314159, "Unexpected value" );
	test_assert( cls.a.member_2 == true, "Unexpected value" );
	test_assert( cls.b == 1234, "Unexpected value" );
	auto const str = daw::json::to_json( cls );
	puts( str.c_str( ) );

	auto const cls2 = daw::json::from_json<daw::cookbook_class2::MyClass2>( str );

	test_assert( cls == cls2, "Unexpected round trip error" );
}
#if defined( DAW_USE_EXCEPTIONS )
catch( daw::json::json_exception const &jex ) {
	std::cerr << "Exception thrown by parser: " << jex.reason( ) << '\n';
	exit( 1 );
} catch( std::exception const &ex ) {
	std::cerr << "Unknown exception thrown during testing: " << ex.what( )
	          << '\n';
	exit( 1 );
} catch( ... ) {
	std::cerr << "Unknown exception thrown during testing\n";
	throw;
}
#endif
