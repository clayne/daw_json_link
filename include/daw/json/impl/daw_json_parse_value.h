// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "daw_iterator_range.h"
#include "daw_json_assert.h"
#include "daw_json_parse_array_iterator.h"
#include "daw_json_parse_name.h"
#include "daw_json_parse_real.h"
#include "daw_json_parse_string_quote.h"
#include "daw_json_parse_unsigned_int.h"
#include "daw_json_parse_value_fwd.h"
#include "daw_string_slow.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <tuple>

namespace daw::json::json_details {
	template<LiteralAsStringOpt literal_as_string, typename Range>
	constexpr void skip_quote_when_literal_as_string( Range &rng ) {
		if constexpr( literal_as_string == LiteralAsStringOpt::Always ) {
			daw_json_assert_weak( rng.front( '"' ),
			                      "Unexpected quote prior to number" );
			rng.remove_prefix( );
		} else if constexpr( literal_as_string == LiteralAsStringOpt::Maybe ) {
			daw_json_assert_weak( not rng.empty( ), "Unexpected end of stream" );
			if( rng.front( ) == '"' ) {
				rng.remove_prefix( );
			}
		} else {
			daw_json_assert_weak( rng.front( ) != '"',
			                      "Unexpected quote prior to number" );
		}
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Real>, Range &rng ) {
		using constructor_t = typename JsonMember::constructor_t;
		using element_t = typename JsonMember::base_type;

		if constexpr( KnownBounds ) {
			daw_json_assert_weak(
			  parse_policy_details::is_real_number_part( rng.front( ) ),
			  "Expected number to start with on of \"0123456789eE+-\"" );
			return constructor_t{ }( parse_real<element_t>( rng ) );
		} else {
			daw_json_assert_weak( rng.has_more( ), "Could not find value" );
			skip_quote_when_literal_as_string<JsonMember::literal_as_string>( rng );
			daw_json_assert_weak(
			  parse_policy_details::is_real_number_part( rng.front( ) ),
			  "Expected number to start with on of \"0123456789eE+-\"" );
			auto result = constructor_t{ }( parse_real<element_t>( rng ) );
			skip_quote_when_literal_as_string<JsonMember::literal_as_string>( rng );
			daw_json_assert_weak(
			  parse_policy_details::at_end_of_item( rng.front( ) ),
			  "Expected whitespace or one of \",}]\" at end of number" );
			return result;
		}
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Signed>, Range &rng ) {
		using constructor_t = typename JsonMember::constructor_t;
		using element_t = typename JsonMember::base_type;

		if constexpr( KnownBounds ) {
			daw_json_assert_weak(
			  parse_policy_details::is_real_number_part( rng.front( ) ),
			  "Expected number to start with on of \"0123456789eE+-\"" );
		} else {
			daw_json_assert_weak( rng.has_more( ), "Could not find value" );
			skip_quote_when_literal_as_string<JsonMember::literal_as_string>( rng );
			daw_json_assert_weak(
			  parse_policy_details::is_real_number_part( rng.front( ) ),
			  "Expected number to start with on of \"0123456789eE+-\"" );
		}

		element_t sign = [&]( ) -> element_t {
			if( rng.front( ) == '-' ) {
				rng.remove_prefix( );
				return -1;
			} else if( rng.front( ) == '+' ) {
				rng.remove_prefix( );
			}
			return 1;
		}( );

		if constexpr( KnownBounds ) {
			return constructor_t{ }(
			  sign *
			  parse_unsigned_integer<element_t, JsonMember::range_check>( rng ) );
		} else {
			auto result = constructor_t{ }(
			  sign *
			  parse_unsigned_integer<element_t, JsonMember::range_check>( rng ) );
			skip_quote_when_literal_as_string<JsonMember::literal_as_string>( rng );
			rng.trim_left( );
			daw_json_assert_weak(
			  parse_policy_details::at_end_of_item( rng.front( ) ),
			  "Expected whitespace or one of \",}]\" at end of number" );
			return result;
		}
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Unsigned>, Range &rng ) {
		using constructor_t = typename JsonMember::constructor_t;
		using element_t = typename JsonMember::base_type;

		if constexpr( KnownBounds ) {
			daw_json_assert_weak(
			  parse_policy_details::is_real_number_part( rng.front( ) ),
			  "Expected number to start with on of \"0123456789eE+-\"" );
			return constructor_t{ }(
			  parse_unsigned_integer<element_t, JsonMember::range_check>( rng ) );
		} else {
			daw_json_assert_weak( rng.has_more( ), "Could not find value" );
			skip_quote_when_literal_as_string<JsonMember::literal_as_string>( rng );
			daw_json_assert_weak(
			  parse_policy_details::is_real_number_part( rng.front( ) ),
			  "Expected number to start with on of \"0123456789eE+-\"" );
			auto result = constructor_t{ }(
			  parse_unsigned_integer<element_t, JsonMember::range_check>( rng ) );
			skip_quote_when_literal_as_string<JsonMember::literal_as_string>( rng );
			daw_json_assert_weak(
			  parse_policy_details::at_end_of_item( rng.front( ) ),
			  "Expected whitespace or one of \",}]\" at end of number" );
			return result;
		}
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Null>, Range &rng ) {

		using constructor_t = typename JsonMember::constructor_t;
		if constexpr( KnownBounds ) {
			if( rng.is_null( ) ) {
				return constructor_t{ }( );
			}
			return parse_value<JsonMember, true>(
			  ParseTag<JsonMember::base_expected_type>{ }, rng );
		} else if constexpr( Range::is_unchecked_input ) {
			if( not rng.has_more( ) ) {
				return constructor_t{ }( );
			} else if( rng.front( ) == 'n' ) {
				rng.remove_prefix( 4 );
				rng.trim_left_unchecked( );
				rng.remove_prefix( );
				return constructor_t{ }( );
			}
			return parse_value<JsonMember>(
			  ParseTag<JsonMember::base_expected_type>{ }, rng );
		} else {
			if( not rng.has_more( ) ) {
				return constructor_t{ }( );
			} else if( rng == "null" ) {
				rng.remove_prefix( 4 );
				daw_json_assert_weak(
				  parse_policy_details::at_end_of_item( rng.front( ) ),
				  "Unexpectd value" );
				rng.trim_left_checked( );
				return constructor_t{ }( );
			}
			return parse_value<JsonMember>(
			  ParseTag<JsonMember::base_expected_type>{ }, rng );
		}
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Bool>, Range &rng ) {
		daw_json_assert_weak( rng.size( ) >= 4, "Range to small to be a bool" );

		using constructor_t = typename JsonMember::constructor_t;

		if constexpr( KnownBounds ) {
			// We have already checked if it is a true/false
			if constexpr( Range::is_unchecked_input ) {
				return rng.front( ) == 't';
			} else {
				switch( rng.front( ) ) {
				case 't':
					return constructor_t{ }( true );
				case 'f':
					return constructor_t{ }( false );
				}
				daw_json_error( "Expected a literal true or false" );
			}
		} else {
			skip_quote_when_literal_as_string<JsonMember::literal_as_string>( rng );
			bool result = false;
			if constexpr( Range::is_unchecked_input ) {
				if( rng.front( ) == 't' ) {
					result = true;
					rng.remove_prefix( 4 );
				} else {
					rng.remove_prefix( 5 );
				}
			} else {
				if( rng == "true" ) {
					rng.remove_prefix( 4 );
					result = true;
				} else if( rng == "false" ) {
					rng.remove_prefix( 5 );
				} else {
					daw_json_error( "Invalid boolean value, expected true or false" );
				}
			}
			skip_quote_when_literal_as_string<JsonMember::literal_as_string>( rng );
			rng.trim_left( );
			daw_json_assert_weak(
			  parse_policy_details::at_end_of_item( rng.front( ) ),
			  "Unexpectd value" );
			return constructor_t{ }( result );
		}
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::StringRaw>, Range &rng ) {

		using constructor_t = typename JsonMember::constructor_t;
		if constexpr( KnownBounds ) {
			if constexpr( JsonMember::empty_is_null == JsonNullable::Nullable ) {
				if( rng.empty( ) ) {
					return constructor_t{ }( );
				}
				return constructor_t{ }( rng.begin( ), rng.size( ) );
			} else {
				return constructor_t{ }( rng.begin( ), rng.size( ) );
			}
		} else {
			auto const str = skip_string( rng );
			if constexpr( JsonMember::empty_is_null == JsonNullable::Nullable ) {
				if( str.empty( ) ) {
					return constructor_t{ }( );
				}
				return constructor_t{ }( str.begin( ), str.size( ) );
			} else {
				return constructor_t{ }( str.begin( ), str.size( ) );
			}
		}
	}

	template<bool IsUncheckedInput>
	struct RangeHelper {
		static constexpr bool is_unchecked_input = IsUncheckedInput;
	};

	template<bool IsUncheckedInput>
	[[nodiscard]] inline constexpr unsigned to_nibble( unsigned chr ) {
		using Range = RangeHelper<IsUncheckedInput>;
		auto const b = static_cast<int>( chr );
		int const maskLetter = ( ( '9' - b ) >> 31 );
		int const maskSmall = ( ( 'Z' - b ) >> 31 );
		int const offset = '0' + ( maskLetter & int( 'A' - '0' - 10 ) ) +
		                   ( maskSmall & int( 'a' - 'A' ) );
		auto const result = static_cast<unsigned>( b - offset );
		daw_json_assert_weak( result < 16U, "Expected a hex nibble" );
		return result;
	}

	static_assert( to_nibble<true>( static_cast<unsigned>( '0' ) ) == 0U );
	static_assert( to_nibble<true>( static_cast<unsigned>( '9' ) ) == 9U );
	static_assert( to_nibble<true>( static_cast<unsigned>( 'a' ) ) == 10U );
	static_assert( to_nibble<true>( static_cast<unsigned>( 'A' ) ) == 10U );
	static_assert( to_nibble<true>( static_cast<unsigned>( 'f' ) ) == 15U );
	static_assert( to_nibble<true>( static_cast<unsigned>( 'F' ) ) == 15U );

	template<typename Range>
	[[nodiscard]] inline constexpr std::uint16_t byte_from_nibbles( Range &rng ) {
		auto const n0 = to_nibble<Range::is_unchecked_input>(
		  static_cast<unsigned>( rng.front( ) ) );
		rng.remove_prefix( );
		auto const n1 = to_nibble<Range::is_unchecked_input>(
		  static_cast<unsigned>( rng.front( ) ) );
		rng.remove_prefix( );
		return static_cast<std::uint16_t>( ( n0 << 4U ) | n1 );
	}

	template<typename Range>
	[[nodiscard]] static constexpr char *decode_utf16( Range &rng, char *it ) {
		daw_json_assert_weak( rng.front( "uU" ), "Expected rng to start with a u" );
		rng.remove_prefix( );
		std::uint32_t cp = static_cast<std::uint32_t>( byte_from_nibbles( rng ) )
		                   << 8U;
		cp |= static_cast<std::uint32_t>( byte_from_nibbles( rng ) );
		if( cp <= 0x7FU ) {
			*it++ = static_cast<char>( cp );
			return it;
		}
		if( 0xD800U <= cp and cp <= 0xDBFFU ) {
			cp = ( cp - 0xD800U ) * 0x400U;
			rng.remove_prefix( );
			daw_json_assert_weak( rng.front( "uU" ),
			                      "Expected rng to start with a \\u" );
			rng.remove_prefix( );
			auto trailing = static_cast<std::uint32_t>( byte_from_nibbles( rng ) )
			                << 8U;
			trailing |= static_cast<std::uint32_t>( byte_from_nibbles( rng ) );
			trailing = static_cast<std::uint32_t>( trailing - 0xDC00U );
			cp += trailing;
			cp += 0x10000;
		}
		// UTF32-> UTF8
		if( cp >= 0x10000U ) {
			// 4 bytes
			char const enc3 =
			  static_cast<char>( ( cp & 0b0011'1111U ) | 0b1000'0000U );
			char const enc2 =
			  static_cast<char>( ( ( cp >> 6U ) & 0b0011'1111U ) | 0b1000'0000U );
			char const enc1 =
			  static_cast<char>( ( ( cp >> 12U ) & 0b0011'1111U ) | 0b1000'0000U );
			char const enc0 = static_cast<char>( ( cp >> 18U ) | 0b1111'0000U );
			*it++ = enc0;
			*it++ = enc1;
			*it++ = enc2;
			*it++ = enc3;
			return it;
		}
		if( cp >= 0x800U ) {
			// 3 bytes
			char const enc2 =
			  static_cast<char>( ( cp & 0b0011'1111U ) | 0b1000'0000U );
			char const enc1 =
			  static_cast<char>( ( ( cp >> 6U ) & 0b0011'1111U ) | 0b1000'0000U );
			char const enc0 = static_cast<char>( ( cp >> 12U ) | 0b1110'0000U );
			*it++ = enc0;
			*it++ = enc1;
			*it++ = enc2;
			return it;
		}
		// cp >= 0x80U
		// 2 bytes
		char const enc1 = static_cast<char>( ( cp & 0b0011'1111U ) | 0b1000'0000U );
		char const enc0 = static_cast<char>( ( cp >> 6U ) | 0b1100'0000U );
		*it++ = enc0;
		*it++ = enc1;
		return it;
	}

	template<typename Range, typename Appender>
	static constexpr void decode_utf16( Range &rng, Appender &app ) {
		daw_json_assert_weak( rng.front( "uU" ), "Expected rng to start with a u" );
		rng.remove_prefix( );
		std::uint32_t cp = static_cast<std::uint32_t>( byte_from_nibbles( rng ) )
		                   << 8U;
		cp |= static_cast<std::uint32_t>( byte_from_nibbles( rng ) );
		if( cp <= 0x7FU ) {
			app( static_cast<char>( cp ) );
			return;
		}
		if( 0xD800U <= cp and cp <= 0xDBFFU ) {
			cp = ( cp - 0xD800U ) * 0x400U;
			rng.remove_prefix( );
			daw_json_assert_weak( rng.front( "uU" ),
			                      "Expected rng to start with a \\u" );
			rng.remove_prefix( );
			auto trailing = static_cast<std::uint32_t>( byte_from_nibbles( rng ) )
			                << 8U;
			trailing |= static_cast<std::uint32_t>( byte_from_nibbles( rng ) );
			trailing = static_cast<std::uint32_t>( trailing - 0xDC00U );
			cp += trailing;
			cp += 0x10000;
		}
		// UTF32-> UTF8
		if( cp >= 0x10000U ) {
			// 4 bytes
			char const enc3 =
			  static_cast<char>( ( cp & 0b0011'1111U ) | 0b1000'0000U );
			char const enc2 =
			  static_cast<char>( ( ( cp >> 6U ) & 0b0011'1111U ) | 0b1000'0000U );
			char const enc1 =
			  static_cast<char>( ( ( cp >> 12U ) & 0b0011'1111U ) | 0b1000'0000U );
			char const enc0 = static_cast<char>( ( cp >> 18U ) | 0b1111'0000U );
			app( enc0 );
			app( enc1 );
			app( enc2 );
			app( enc3 );
			return;
		}
		if( cp >= 0x800U ) {
			// 3 bytes
			char const enc2 =
			  static_cast<char>( ( cp & 0b0011'1111U ) | 0b1000'0000U );
			char const enc1 =
			  static_cast<char>( ( ( cp >> 6U ) & 0b0011'1111U ) | 0b1000'0000U );
			char const enc0 = static_cast<char>( ( cp >> 12U ) | 0b1110'0000U );
			app( enc0 );
			app( enc1 );
			app( enc2 );
			return;
		}
		// cp >= 0x80U
		// 2 bytes
		char const enc1 = static_cast<char>( ( cp & 0b0011'1111U ) | 0b1000'0000U );
		char const enc0 = static_cast<char>( ( cp >> 6U ) | 0b1100'0000U );
		app( enc0 );
		app( enc1 );
	}

	template<typename... Args>
	std::true_type is_string( std::basic_string<Args...> const & ) {
		return { };
	}
	template<typename T>
	std::false_type is_string( T const & ) {
		return { };
	}

	// Fast path for parsing escaped strings to a std::string with the default
	// appender
	template<typename JsonMember, typename String, typename Range>
	[[nodiscard, maybe_unused]] DAW_ATTRIBUTE_FLATTEN inline constexpr String
	parse_string_known_stdstring( Range &rng ) {
		String result2 = std::string( rng.size( ), '\0' );

		auto &result = [&result2]( ) -> std::string & {
			if constexpr( std::is_same_v<String, std::string> ) {
				return result2;
			} else {
				return *result2;
			}
		}( );

		char *it = result.data( );

		bool const has_quote = rng.front( '"' );
		if( has_quote ) {
			rng.remove_prefix( );
		}
		while( rng.front( ) != '"' ) {
			while( rng.front( ) != '"' and rng.front( ) != '\\' ) {
				daw_json_assert_weak( not rng.empty( ), "Unexpected end of data" );
				*it++ = rng.front( );
				rng.remove_prefix( );
			}
			if( rng.front( ) == '\\' ) {
				daw_json_assert_weak( rng.front( ) >= 0x20, "Invalid codepoint" );
				rng.remove_prefix( );
				switch( rng.front( ) ) {
				case 'b':
					*it++ = '\b';
					rng.remove_prefix( );
					break;
				case 'f':
					*it++ = '\f';
					rng.remove_prefix( );
					break;
				case 'n':
					*it++ = '\n';
					rng.remove_prefix( );
					break;
				case 'r':
					*it++ = '\r';
					rng.remove_prefix( );
					break;
				case 't':
					*it++ = '\t';
					rng.remove_prefix( );
					break;
				case 'U': // Sometimes people put crap
				case 'u':
					it = decode_utf16( rng, it );
					break;
				case '\\':
				case '/':
				case '"':
					*it++ = rng.front( );
					rng.remove_prefix( );
					break;
				default:
					if constexpr( JsonMember::eight_bit_mode ==
					              EightBitModes::DisallowHigh ) {
						daw_json_assert_weak(
						  static_cast<unsigned>( rng.front( ) ) >= 0x20U and
						    static_cast<unsigned>( rng.front( ) ) <= 0x7FU,
						  "string support limited to 0x20 < chr <= 0x7F when "
						  "DisallowHighEightBit is true" );
					}
					*it++ = rng.front( );
					rng.remove_prefix( );
				}
			} else {
				daw_json_assert_weak( not has_quote or rng.front( '"' ),
				                      "Unexpected end of string" );
			}
			daw_json_assert_weak( not has_quote or rng.has_more( ),
			                      "Unexpected end of data" );
		}
		auto const sz =
		  static_cast<std::size_t>( std::distance( result.data( ), it ) );
		daw_json_assert_weak( result.size( ) >= sz, "Unexpected string state" );
		result.resize( sz );
		return result2;
	}

	//**************************
	template<typename JsonMember>
	using can_fast_path = std::conjunction<
	  std::disjunction<
	    std::is_same<std::string, json_result<JsonMember>>,
	    std::is_same<std::optional<std::string>, json_result<JsonMember>>>,
	  std::is_same<typename JsonMember::appender_t, basic_appender<std::string>>,
	  std::disjunction<
	    std::is_same<typename JsonMember::constructor_t,
	                 daw::construct_a_t<std::string>>,
	    std::is_same<typename JsonMember::constructor_t,
	                 daw::construct_a_t<std::optional<std::string>>>>>;

	template<typename JsonMember>
	inline constexpr bool can_fast_path_v = can_fast_path<JsonMember>::value;

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::StringEscaped>, Range &rng ) {
		if constexpr( can_fast_path_v<JsonMember> ) {
			// We know that we are constructing a std::string or
			// std::optional<std::string> We can take advantage of this and reduce the
			// allocator time by presizing the string up front and then using a
			// pointer to the data( ).
			if constexpr( KnownBounds ) {
				if( needs_slow_path( rng.counter ) ) {
					return parse_string_known_stdstring<JsonMember,
					                                    json_result<JsonMember>>( rng );
				}
				if constexpr( std::is_same_v<std::string, json_result<JsonMember>> ) {
					return std::string( rng.first, rng.size( ) );
				} else {
					return std::optional<std::string>( std::in_place, rng.first,
					                                   rng.size( ) );
				}
			} else {
				auto rng2 = skip_string( rng );
				if( needs_slow_path( rng2.counter ) ) {
					return parse_string_known_stdstring<JsonMember,
					                                    json_result<JsonMember>>( rng2 );
				}
				if constexpr( std::is_same_v<std::string, json_result<JsonMember>> ) {
					return std::string( rng2.first, rng2.size( ) );
				} else {
					return std::optional<std::string>( std::in_place, rng2.first,
					                                   rng2.size( ) );
				}
			}
		} else {
			using constructor_t = typename JsonMember::constructor_t;
			using appender_t = typename JsonMember::appender_t;
			constexpr EightBitModes eight_bit_mode = JsonMember::eight_bit_mode;

			auto result = constructor_t{ }( );
			auto app = [&] {
				if constexpr( std::is_same_v<typename JsonMember::parse_to_t,
				                             typename JsonMember::base_type> ) {
					return daw::construct_a<appender_t>( result );
				} else {
					return daw::construct_a<appender_t>( *result );
				}
			}( );

			bool const has_quote = rng.front( '"' );
			if( has_quote ) {
				rng.remove_prefix( );
			}
			daw_json_assert_weak( not rng.empty( ), "Unexpected end of data" );
			while( rng.front( ) != '"' ) {
				while( rng.front( ) != '"' and rng.front( ) != '\\' ) {
					daw_json_assert_weak( not rng.empty( ), "Unexpected end of data" );
					app( rng.front( ) );
					rng.remove_prefix( );
				}
				if( rng.front( ) == '\\' ) {
					daw_json_assert_weak( rng.front( ) >= 0x20, "Invalid codepoint" );
					rng.remove_prefix( );
					switch( rng.front( ) ) {
					case 'b':
						app( '\b' );
						rng.remove_prefix( );
						break;
					case 'f':
						app( '\f' );
						rng.remove_prefix( );
						break;
					case 'n':
						app( '\n' );
						rng.remove_prefix( );
						break;
					case 'r':
						app( '\r' );
						rng.remove_prefix( );
						break;
					case 't':
						app( '\t' );
						rng.remove_prefix( );
						break;
					case 'U': // Sometimes people put crap
					case 'u':
						decode_utf16( rng, app );
						break;
					case '\\':
					case '/':
					case '"':
						app( rng.front( ) );
						rng.remove_prefix( );
						break;
					default:
						if constexpr( eight_bit_mode == EightBitModes::DisallowHigh ) {
							daw_json_assert_weak(
							  static_cast<unsigned>( rng.front( ) ) >= 0x20U and
							    static_cast<unsigned>( rng.front( ) ) <= 0x7FU,
							  "string support limited to 0x20 < chr <= 0x7F when "
							  "DisallowHighEightBit is true" );
						}
						app( rng.front( ) );
						rng.remove_prefix( );
					}
				} else {
					daw_json_assert_weak( not has_quote or rng.front( '"' ),
					                      "Unexpected end of string" );
				}
				daw_json_assert_weak( not has_quote or rng.has_more( ),
				                      "Unexpected end of data" );
			}

			if constexpr( not KnownBounds ) {
				daw_json_assert_weak( not has_quote or rng.front( '"' ),
				                      "Unexpected state, no \"" );
				rng.remove_prefix( );
			}
			return result;
		}
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Date>, Range &rng ) {

		daw_json_assert_weak( rng.has_more( ), "Could not find value" );
		auto str = skip_string( rng );
		using constructor_t = typename JsonMember::constructor_t;
		return constructor_t{ }( str.begin( ), str.size( ) );
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Custom>, Range &rng ) {

		if( rng.front( ) != '"' and rng.class_first != nullptr and
		    rng.begin( ) > rng.class_first and *std::prev( rng.begin( ) ) == '"' ) {
			rng.first = std::prev( rng.first );
		}
		auto const str = skip_value( rng );

		using constructor_t = typename JsonMember::from_converter_t;
		return constructor_t{ }( std::string_view( str.begin( ), str.size( ) ) );
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Class>, Range &rng ) {

		using element_t = typename JsonMember::base_type;
		daw_json_assert_weak( rng.has_more( ), "Attempt to parse empty string" );

#if defined( __cpp_constexpr_dynamic_alloc ) or                                \
  defined( DAW_JSON_NO_CONST_EXPR )
		// This relies on non-trivial dtor's being allowed.  So C++20 constexpr or
		// not in a constant expression.  It does allow for construction of
		// classes without move/copy special members
		if constexpr( not KnownBounds ) {
			auto const oe = daw::on_exit_success( [&] { rng.trim_left_checked( ); } );
		}
		return json_data_contract_trait_t<element_t>::template parse<element_t>(
		  rng );
#else
		if constexpr( KnownBounds ) {
			return json_data_contract_trait_t<element_t>::template parse<element_t>(
			  rng );
		} else {
			auto result =
			  json_data_contract_trait_t<element_t>::template parse<element_t>( rng );
			// TODO: make trim_left
			rng.trim_left_checked( );
			return result;
		}
#endif
	}

	/**
	 * Parse a key_value pair encoded as a json object where the keys are the
	 * member names
	 * @tparam JsonMember json_key_value type
	 * @tparam Range Input range typee
	 * @param rng Range of input to parse
	 * @return Constructed key_value container
	 */
	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::KeyValue>, Range &rng ) {

		static_assert( JsonMember::expected_type == JsonParseTypes::KeyValue,
		               "Expected a json_key_value" );
		daw_json_assert_weak(
		  rng.front( '{' ),
		  "Expected keyvalue type to be of class type and beging with '{'" );

		rng.remove_prefix( );
		// We are inside a KV map, we can expected a quoted name next
		rng.move_to_next_of( '"' );

		auto array_container = typename JsonMember::constructor_t{ }( );
		auto container_appender =
		  typename JsonMember::appender_t( array_container );

		using key_t = typename JsonMember::json_key_t;
		using value_t = typename JsonMember::json_element_t;
		while( rng.front( ) != '}' ) {
			daw_json_assert_weak( rng.has_more( ), "Unexpected end of range" );
			auto key = parse_value<key_t>( ParseTag<key_t::expected_type>{ }, rng );
			name::name_parser::trim_end_of_name( rng );
			container_appender(
			  std::move( key ),
			  parse_value<value_t>( ParseTag<value_t::expected_type>{ }, rng ) );

			rng.clean_tail( );
			if constexpr( not KnownBounds ) {
				daw_json_assert_weak( rng.has_more( ), "Unexpected end of data" );
			}
		}
		if constexpr( not KnownBounds ) {
			daw_json_assert_weak( rng.front( '}' ),
			                      "Expected keyvalue type to end with a '}'" );
			rng.remove_prefix( );
			rng.trim_left_checked( );
		}
		return array_container;
	}

	/**
	 * Parse a key_value pair encoded as a json object where the keys are the
	 * member names
	 * @tparam JsonMember json_key_value type
	 * @tparam Range Input Range type
	 * @param rng Range of input to parse
	 * @return Constructed key_value container
	 */
	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::KeyValueArray>, Range &rng ) {

		static_assert( JsonMember::expected_type == JsonParseTypes::KeyValueArray,
		               "Expected a json_key_value" );
		daw_json_assert_weak(
		  rng.front( '[' ),
		  "Expected keyvalue type to be of class type and beging with '{'" );

		rng.remove_prefix( );

		auto array_container = typename JsonMember::constructor_t{ }( );
		auto container_appender =
		  typename JsonMember::appender_t( array_container );

		using key_t = typename JsonMember::json_key_t;
		using value_t = typename JsonMember::json_value_t;
		while( rng.front( ) != ']' ) {
			// We are in an object find {
			rng.move_to_next_of( '{' );
			rng.remove_prefix( );
			rng.move_to_next_of( "\"}" );
			daw_json_assert_weak( rng.front( '"' ), "Expected name of key member" );
			rng.remove_prefix( );
			auto const key_name = name::name_parser::parse_nq( rng );
			(void)key_name;
			daw_json_assert_weak( key_t::name == key_name,
			                      "Expected value name to match" );
			auto key = parse_value<key_t>( ParseTag<key_t::expected_type>{ }, rng );
			rng.move_to_next_of( '"' ); // Next
			daw_json_assert_weak( rng.front( '"' ), "Expected name of value member" );
			rng.remove_prefix( );
			auto const value_name = name::name_parser::parse_nq( rng );
			(void)value_name;
			daw_json_assert_weak( value_t::name == value_name,
			                      "Expected value name to match" );
			container_appender(
			  std::move( key ),
			  parse_value<value_t>( ParseTag<value_t::expected_type>{ }, rng ) );

			rng.move_to_next_of( '}' );
			rng.remove_prefix( );
			rng.trim_left_checked( );
			if constexpr( not KnownBounds ) {
				daw_json_assert_weak( rng.has_more( ), "Unexpected end of data" );
			}
			// End of object }
		}
		if constexpr( not KnownBounds ) {
			daw_json_assert_weak( rng.front( ']' ),
			                      "Expected keyvalue type to end with a '}'" );
			rng.remove_prefix( );
			rng.trim_left_checked( );
		}
		return array_container;
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Array>, Range &rng ) {
		rng.trim_left( );
		daw_json_assert_weak( rng.front( '[' ),
		                      "Expected array to start with a '['" );

		rng.remove_prefix( );
		rng.trim_left_unchecked( );
		using container_t = typename JsonMember::base_type;
		using element_t = typename JsonMember::json_element_t::parse_to_t;
		using iterator_t = json_parse_value_array_iterator<JsonMember, Range>;
		constexpr auto last = iterator_t( );

		if constexpr( std::conjunction_v<
		                std::is_same<typename JsonMember::appender_t,
		                             basic_appender<container_t>>,
		                std::is_same<typename JsonMember::constructor_t,
		                             daw::construct_a_t<container_t>>,
		                is_range_constructible<container_t, element_t>> ) {
			// We are using the default constructor and appender.  This should allow
			// for constructing the vector directly when it is constructable from a
			// Iterator pair
			if constexpr( daw::traits::is_vector_v<container_t> ) {
				if constexpr( KnownBounds ) {
					auto first = iterator_t( rng );
					auto result = std::vector<element_t>( );
					result.reserve( rng.counter );
					result.insert( result.end( ), first, last );
					return result;
				} else {
					auto first = iterator_t( rng );
					return container_t( first, last );
				}
			} else {
				auto first = iterator_t( rng );
				return container_t( first, last );
			}
		} else {
			auto array_container = typename JsonMember::constructor_t{ }( );
			auto container_appender =
			  typename JsonMember::appender_t( array_container );
			auto first = iterator_t( rng );

			while( first != last ) {
				container_appender( *first );
				++first;
			}
			return array_container;
		}
	} // namespace daw::json::json_details

	template<JsonBaseParseTypes BPT, typename JsonMembers, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMembers>
	parse_variant_value( Range &rng ) {

		using element_t = typename JsonMembers::json_elements;
		constexpr std::size_t idx =
		  element_t::base_map[static_cast<int_fast8_t>( BPT )];
		if constexpr( idx < std::tuple_size_v<typename element_t::element_map_t> ) {
			using JsonMember =
			  std::tuple_element_t<idx, typename element_t::element_map_t>;
			return parse_value<JsonMember>(
			  ParseTag<JsonMember::base_expected_type>{ }, rng );
		} else {
			daw_json_error( "Unexpected JSON Variant type." );
		}
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::Variant>, Range &rng ) {

		switch( rng.front( ) ) {
		case '{':
			return parse_variant_value<JsonBaseParseTypes::Class, JsonMember>( rng );
		case '[':
			return parse_variant_value<JsonBaseParseTypes::Array, JsonMember>( rng );
		case 't':
		case 'f':
			return parse_variant_value<JsonBaseParseTypes::Bool, JsonMember>( rng );
		case '"':
			return parse_variant_value<JsonBaseParseTypes::String, JsonMember>( rng );
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '+':
		case '-':
			return parse_variant_value<JsonBaseParseTypes::Number, JsonMember>( rng );
		}
		daw_json_error( "Unexcepted data at start of json member" );
	}

	template<typename Result, typename TypeList, std::size_t pos = 0,
	         typename Range>
	constexpr Result parse_visit( std::size_t idx, Range &rng ) {
		if( idx == pos ) {
			using JsonMember = std::tuple_element_t<pos, TypeList>;
			return { parse_value<JsonMember>( ParseTag<JsonMember::expected_type>{ },
			                                  rng ) };
		}
		if constexpr( pos + 1 < std::tuple_size_v<TypeList> ) {
			return parse_visit<Result, TypeList, pos + 1>( idx, rng );
		} else {
			// Should never happen
			std::abort( );
		}
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	[[nodiscard, maybe_unused]] inline constexpr json_result<JsonMember>
	parse_value( ParseTag<JsonParseTypes::VariantTagged>, Range &rng ) {

		using tag_member = typename JsonMember::tag_member;
		auto [is_found, rng2] = find_range<typename Range::policy>(
		  daw::string_view( rng.class_first, rng.last ),
		  daw::string_view( as_cstr( tag_member::name ) ) );

		daw_json_assert( is_found, "Tag Member is manditory" );
		auto index = typename JsonMember::switcher{ }(
		  parse_value<tag_member>( ParseTag<tag_member::expected_type>{ }, rng2 ) );

		return parse_visit<json_result<JsonMember>,
		                   typename JsonMember::json_elements::element_map_t>(
		  index, rng );
	}

	template<typename JsonMember, bool KnownBounds, typename Range>
	constexpr basic_json_value<typename Range::as_checked>
	parse_value( ParseTag<JsonParseTypes::Unknown>, Range &rng ) {
		using result_rng = typename Range::as_checked;
		if constexpr( KnownBounds ) {
			return { result_rng( rng.first, rng.last ) };
		} else {
			auto value_rng = skip_value( rng );
			return { result_rng( value_rng.first, value_rng.last ) };
		}
	}
} // namespace daw::json::json_details
