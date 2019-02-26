// The MIT License (MIT)
//
// Copyright (c) 2018-2019 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <fstream>
#include <iostream>
#include <vector>

#include <daw/daw_benchmark.h>

#include "daw/json/daw_json_link.h"

struct City {
	daw::string_view country;
	daw::string_view name;
	float lat;
	float lng;
};

#ifdef USECPP20
auto describe_json_class( City ) noexcept {
	using namespace daw::json;
	return json_parser_t<json_string<"country", daw::string_view>,
	                     json_string<"name", daw::string_view>,
	                     json_number<"lat", float, NullValueOpt::never>,
	                     json_number<"lng", float, NullValueOpt::never>>{};
}
#else
auto describe_json_class( City ) noexcept {
	static constexpr char names0[] = "country";
	static constexpr char names1[] = "name";
	static constexpr char names2[] = "lat";
	static constexpr char names3[] = "lng";
	using namespace daw::json;
	return json_parser_t<json_string<names0, daw::string_view>,
	                     json_string<names1, daw::string_view>,
	                     json_number<names2, float, NullValueOpt::never>,
	                     json_number<names3, float, NullValueOpt::never>>{};
}
#endif
int main( int argc, char **argv ) {
	using namespace daw::json;
	if( argc < 2 ) {
		std::cerr << "Must supply a filename to open\n";
		exit( 1 );
	}
	std::cout << "Opening file: '" << argv[1] << "'\n";
	auto in_file = std::ifstream( argv[1] );
	if( !in_file ) {
		std::cerr << "Could not open input file\n";
		exit( 1 );
	}
	auto json_data = std::string( std::istream_iterator<char>( in_file ),
	                              std::istream_iterator<char>( ) );
	in_file.close( );
	auto json_sv = daw::string_view( json_data );
	std::cout << "File size(B): " << json_data.size( ) << " " << daw::utility::to_bytes_per_second( json_data.size( ) ) << '\n';

	auto count = *daw::bench_n_test<4>(
	  "cities parsing 1",
	  []( auto &&sv ) {
		  auto const data = daw::json::from_json_array<json_class<no_name, City>>( sv );
		  return data.size( );
	  },
	  json_sv );

	std::cout << "element count: " << count << '\n';

	using iterator_t = daw::json::json_array_iterator<json_class<no_name, City>>;

	auto data = std::vector<City>( );

	auto count2 = *daw::bench_n_test<4>(
	  "cities parsing 2",
	  [&]( auto &&sv ) {
		  data.clear( );
		  std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
			std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
			std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
			std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
			std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
			std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
			std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
			std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
			std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
			std::copy( iterator_t( sv ), iterator_t( ), std::back_inserter( data ) );
		  return data.size( );
	  },
	  json_sv );

	std::cout << "element count 2: " << count2 << '\n';
	auto count3 =
	  *daw::bench_n_test<4>( "cities parsing 3",
	                         []( auto &&sv ) {
		                         return static_cast<size_t>( std::distance(
		                           iterator_t( sv ), iterator_t( ) ) );
	                         },
	                         json_sv );

	std::cout << "element count 3: " << count3 << '\n';
}
