// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "version.h"

#include "daw/json/daw_json_switches.h"
#include "daw_json_assert.h"
#include "daw_json_parse_digit.h"

#include <daw/daw_arith_traits.h>
#include <daw/daw_attributes.h>
#include <daw/daw_cpp_feature_check.h>
#include <daw/daw_cxmath.h>
#include <daw/daw_string_view.h>
#include <daw/daw_traits.h>
#include <daw/daw_uint_buffer.h>

#include <cassert>
#include <chrono>
#include <cstdint>

namespace daw::json {
	inline namespace DAW_JSON_VER {
		namespace parse_utils {
			template<typename Result, std::size_t count>
			DAW_ATTRIB_NONNULL( )
			constexpr Result parse_unsigned( char const *digit_str ) {
				UInt64 result = UInt64( );
				for( std::size_t n = 0; n < count; ++n ) {
					auto const dig =
					  to_uint64( json_details::parse_digit( digit_str[n] ) );
					if( dig >= 10 ) {
						break;
					}
					result *= 10U;
					result += dig;
				}
				return static_cast<Result>( result );
			}

			template<typename Result>
			DAW_ATTRIB_NONNULL( )
			constexpr Result parse_unsigned2( char const *digit_str ) {
				UInt64 result = UInt64( );
				unsigned dig = json_details::parse_digit( *digit_str );
				while( dig < 10 ) {
					result *= 10U;
					result += dig;
					++digit_str;
					dig = json_details::parse_digit( *digit_str );
				}
				return static_cast<Result>( result );
			}

			constexpr bool is_number( char c ) {
				return json_details::parse_digit( c ) < 10U;
			}
		} // namespace parse_utils

		namespace datetime {
			namespace datetime_details {

				template<typename Result>
				constexpr Result parse_number( daw::string_view sv ) {
					static_assert( daw::numeric_limits<Result>::digits10 >= 4 );
					daw_json_ensure( not sv.empty( ), ErrorReason::InvalidNumber );
					Result result = 0;
					Result sign = 1;
					if( sv.front( ) == '-' ) {
						if constexpr( daw::is_signed_v<Result> ) {
							sign = -1;
						}
						sv.remove_prefix( );
					} else if( sv.front( ) == '+' ) {
						sv.remove_prefix( );
					}
					while( not sv.empty( ) ) {
						auto const dig = json_details::parse_digit( sv.pop_front( ) );
						daw_json_ensure( dig < 10U, ErrorReason::InvalidNumber );
						result *= 10;
						result += static_cast<Result>( dig );
					}
					return result * sign;
				}
			} // namespace datetime_details
			// See:
			// https://stackoverflow.com/questions/16773285/how-to-convert-stdchronotime-point-to-stdtm-without-using-time-t
			template<typename TP = std::chrono::time_point<std::chrono::system_clock,
			                                               std::chrono::milliseconds>>
			constexpr TP civil_to_time_point( std::int32_t yr, std::uint32_t mo,
			                                  std::uint32_t dy, std::uint32_t hr,
			                                  std::uint32_t mn, std::uint32_t se,
			                                  std::uint64_t ns ) {
				using Clock = typename TP::clock;
				using Duration = typename TP::duration;
				constexpr auto calc =
				  []( std::int32_t y, std::uint32_t m, std::uint32_t d, std::uint32_t h,
				      std::uint32_t min, std::uint32_t s,
				      std::uint64_t nano ) DAW_JSON_CPP23_STATIC_CALL_OP {
					  y -= static_cast<std::int32_t>( m ) <= 2;
					  std::int32_t const era = ( y >= 0 ? y : y - 399 ) / 400;
					  auto const yoe = static_cast<std::uint32_t>(
					    static_cast<std::int32_t>( y ) - era * 400 ); // [0, 399]
					  auto const doy = static_cast<std::uint32_t>(
					    ( 153 * ( static_cast<std::int32_t>( m ) +
					              ( static_cast<std::int32_t>( m ) > 2 ? -3 : 9 ) ) +
					      2 ) /
					      5 +
					    static_cast<std::int32_t>( d ) - 1 ); // [0, 365]
					  std::uint32_t const doe =
					    yoe * 365 + yoe / 4 - yoe / 100 + doy; // [0, 146096]
					  auto const days_since_epoch =
					    static_cast<int64_t>( era ) * 146097LL +
					    static_cast<std::int64_t>( doe ) - 719468LL;

					  using Days = std::chrono::duration<std::int32_t, std::ratio<86400>>;
					  auto const dur =
					    std::chrono::floor<Duration>( std::chrono::nanoseconds( nano ) );
					  return std::chrono::time_point<std::chrono::system_clock,
					                                 Duration>{ } +
					         ( Days( days_since_epoch ) + std::chrono::hours( h ) +
					           std::chrono::minutes( min ) +
					           std::chrono::seconds( static_cast<std::uint32_t>( s ) ) +
					           dur );
				  };
				// Not all clocks have the same epoch.  This should account for the
				// offset and adjust the time_point so that the days prior are in
				// relation to unix epoch.  If system_clock is used, as is the default
				// for the return value, it will be zero and should be removed by the
				// compiler
				auto result = calc( yr, mo, dy, hr, mn, se, ns );

				if constexpr( std::is_same_v<Clock, std::chrono::system_clock> ) {
					return result;
				} else {
#if defined( __cpp_lib_chrono ) and __cpp_lib_chrono >= 201907
					// We have clock_cast
					auto const match_duration =
					  std::chrono::time_point_cast<Duration>( result );
					auto const match_clock =
					  std::chrono::clock_cast<Clock>( match_duration );
					return match_clock;
#else
					// This is a guess and will not be constexpr

					// System epoch is unix epoch on(gcc/clang/msvc)
					auto const system_epoch = std::chrono::floor<std::chrono::hours>(
					  std::chrono::system_clock::now( ).time_since_epoch( ) +
					  std::chrono::minutes( 30 ) );
					auto const clock_epoch = std::chrono::floor<std::chrono::hours>(
					  Clock::now( ).time_since_epoch( ) + std::chrono::minutes( 30 ) );

					constexpr auto offset =
					  std::chrono::duration_cast<std::chrono::milliseconds>(
					    clock_epoch - system_epoch );
					return std::chrono::duration_cast<Duration>( result + offset );
#endif
				}
			}

			struct date_parts {
				std::int32_t year;
				std::uint32_t month;
				std::uint32_t day;
			};

			constexpr date_parts
			parse_iso_8601_date( daw::string_view timestamp_str ) {
				auto result = date_parts{ 0, 0, 0 };
				result.day = parse_utils::parse_unsigned<std::uint_least32_t, 2>(
				  std::data( timestamp_str.pop_back( 2U ) ) );
				daw_json_ensure( result.day >= 1 and result.day <= 31,
				                 ErrorReason::InvalidTimestamp );
				if( not parse_utils::is_number( timestamp_str.back( ) ) ) {
					timestamp_str.remove_suffix( );
				}
				result.month = parse_utils::parse_unsigned<std::uint_least32_t, 2>(
				  std::data( timestamp_str.pop_back( 2U ) ) );
				daw_json_ensure( result.month >= 1 and result.month <= 12,
				                 ErrorReason::InvalidTimestamp );
				if( not parse_utils::is_number( timestamp_str.back( ) ) ) {
					timestamp_str.remove_suffix( );
				}
				result.year =
				  datetime_details::parse_number<std::int_least32_t>( timestamp_str );
				return result;
			}

			struct time_parts {
				std::uint_least32_t hour;
				std::uint_least32_t minute;
				std::uint_least32_t second;
				std::uint64_t nanosecond;
			};

			constexpr time_parts
			parse_iso_8601_time( daw::string_view timestamp_str ) {
				auto result = time_parts{ 0, 0, 0, 0 };
				result.hour = parse_utils::parse_unsigned<std::uint_least32_t, 2>(
				  std::data( timestamp_str.pop_front( 2 ) ) );
				daw_json_ensure( result.hour <= 24, ErrorReason::InvalidTimestamp );
				if( not parse_utils::is_number( timestamp_str.front( ) ) ) {
					timestamp_str.remove_prefix( );
				}
				result.minute = parse_utils::parse_unsigned<std::uint_least32_t, 2>(
				  std::data( timestamp_str.pop_front( 2 ) ) );
				daw_json_ensure( result.minute <= 59, ErrorReason::InvalidTimestamp );
				if( timestamp_str.empty( ) ) {
					return result;
				}
				if( not parse_utils::is_number( timestamp_str.front( ) ) ) {
					timestamp_str.remove_prefix( );
				}
				result.second = parse_utils::parse_unsigned<std::uint_least32_t, 2>(
				  std::data( timestamp_str.pop_front( 2 ) ) );
				daw_json_ensure( result.second <= 60, ErrorReason::InvalidTimestamp );
				if( timestamp_str.empty( ) ) {
					return result;
				}
				if( not parse_utils::is_number( timestamp_str.front( ) ) ) {
					timestamp_str.remove_prefix( );
				}
				auto const nanosecond_str = timestamp_str.substr(
				  0, std::min( timestamp_str.size( ), std::size_t{ 9 } ) );
				result.nanosecond =
				  datetime_details::parse_number<std::uint64_t>( nanosecond_str );
				result.nanosecond *= daw::cxmath::pow10( 9 - timestamp_str.size( ) );
				return result;
			}

			template<typename TP>
			constexpr TP parse_iso8601_timestamp( daw::string_view ts ) {
				constexpr daw::string_view t_str = "T";
				auto const date_str = ts.pop_front_until( t_str );
				if( ts.empty( ) ) {
					daw_json_error(
					  ErrorReason::InvalidTimestamp ); // Invalid timestamp,
					                                   // missing T separator
				}

				date_parts const ymd = parse_iso_8601_date( date_str );
				auto time_str =
				  ts.pop_front_until( []( char c ) DAW_JSON_CPP23_STATIC_CALL_OP {
					  return not( parse_utils::is_number( c ) | ( c == ':' ) |
					              ( c == '.' ) );
				  } );
				// TODO: verify or parse timezone
				time_parts hms = parse_iso_8601_time( time_str );
				if( not( ts.empty( ) or ts.front( ) == 'Z' ) ) {
					daw_json_ensure( std::size( ts ) == 5 or std::size( ts ) == 6,
					                 ErrorReason::InvalidTimestamp );
					// The format will be (+|-)hh[:]mm
					bool sign = false;
					daw_json_ensure( not ts.empty( ), ErrorReason::InvalidTimestamp );
					switch( ts.front( ) ) {
					case '+':
						sign = true;
						break;
					case '-':
						break;
					default:
						daw_json_error( daw::json::ErrorReason::InvalidTimestamp );
					}
					ts.remove_prefix( );
					auto hr_offset = parse_utils::parse_unsigned<std::uint_least32_t, 2>(
					  std::data( ts ) );
					daw_json_ensure( hr_offset <= 24,
					                 daw::json::ErrorReason::InvalidTimestamp );
					if( ts.front( ) == ':' ) {
						ts.remove_prefix( );
					}
					auto mn_offset = parse_utils::parse_unsigned<std::uint_least32_t, 2>(
					  std::data( ts ) );
					daw_json_ensure( mn_offset <= 61,
					                 daw::json::ErrorReason::InvalidTimestamp );
					// Want to subtract offset from current time, we are converting to UTC
					if( sign ) {
						// Positive offset
						hms.hour -= hr_offset;
						hms.minute -= mn_offset;
					} else {
						// Negative offset
						hms.hour += hr_offset;
						hms.minute += mn_offset;
					}
				}
				return civil_to_time_point<TP>( ymd.year, ymd.month, ymd.day, hms.hour,
				                                hms.minute, hms.second,
				                                hms.nanosecond );
			}
			struct ymdhms {
				std::int_least32_t year;
				std::uint_least32_t month;
				std::uint_least32_t day;
				std::uint_least32_t hour;
				std::uint_least32_t minute;
				std::uint_least32_t second;
				std::uint64_t nanosecond;
			};

			template<typename Clock, typename Duration>
			constexpr ymdhms time_point_to_civil(
			  std::chrono::time_point<Clock, Duration> const &tp ) {
				auto dur_from_epoch = tp.time_since_epoch( );
				using Days =
				  std::chrono::duration<std::int_least32_t, std::ratio<86400>>;
				auto const days_since_epoch =
				  std::chrono::duration_cast<Days>( dur_from_epoch );
				std::int_least32_t z = days_since_epoch.count( );
				z += 719468;
				std::int_least32_t const era = ( z >= 0 ? z : z - 146096 ) / 146097;
				auto const doe =
				  static_cast<std::uint_least32_t>( z - era * 146097 ); // [0, 146096]
				std::uint_least32_t const yoe =
				  ( doe - doe / 1460 + doe / 36524 - doe / 146096 ) / 365; // [0, 399]
				std::int_least32_t const y =
				  static_cast<std::int_least32_t>( yoe ) + era * 400;
				std::uint_least32_t const doy =
				  doe - ( 365 * yoe + yoe / 4 - yoe / 100 );          // [0, 365]
				std::uint_least32_t const mp = ( 5 * doy + 2 ) / 153; // [0, 11]
				std::uint_least32_t const d = doy - ( 153 * mp + 2 ) / 5 + 1; // [1, 31]
				auto const m = static_cast<std::uint_least32_t>(
				  static_cast<std::int_least32_t>( mp ) +
				  ( static_cast<std::int_least32_t>( mp ) < 10 ? 3 : -9 ) ); // [1, 12]

				dur_from_epoch -= days_since_epoch;
				auto const hrs =
				  std::chrono::duration_cast<std::chrono::hours>( dur_from_epoch );
				dur_from_epoch -= hrs;
				auto const min =
				  std::chrono::duration_cast<std::chrono::minutes>( dur_from_epoch );
				dur_from_epoch -= min;
				auto const sec =
				  std::chrono::duration_cast<std::chrono::seconds>( dur_from_epoch );
				dur_from_epoch -= sec;
				auto const dur = std::chrono::duration_cast<std::chrono::nanoseconds>(
				  dur_from_epoch );
				return ymdhms{ y + ( m <= 2 ),
				               m,
				               d,
				               static_cast<std::uint_least32_t>( hrs.count( ) ),
				               static_cast<std::uint_least32_t>( min.count( ) ),
				               static_cast<std::uint_least32_t>( sec.count( ) ),
				               static_cast<std::uint64_t>( dur.count( ) ) };
			}

			constexpr std::string_view month_short_name( unsigned m ) {
				switch( m ) {
				case 1:
					return { "Jan" };
				case 2:
					return { "Feb" };
				case 3:
					return { "Mar" };
				case 4:
					return { "Apr" };
				case 5:
					return { "May" };
				case 6:
					return { "Jun" };
				case 7:
					return { "Jul" };
				case 8:
					return { "Aug" };
				case 9:
					return { "Sep" };
				case 10:
					return { "Oct" };
				case 11:
					return { "Nov" };
				case 12:
					return { "Dec" };
				default:
					DAW_UNLIKELY_BRANCH
					daw_json_error( ErrorReason::InvalidTimestamp ); // Invalid month
				}
			}

			// Formula from
			// http://howardhinnant.github.io/date_algorithms.html#weekday_from_days
			template<typename Duration>
			constexpr std::string_view short_day_of_week(
			  std::chrono::time_point<std::chrono::system_clock, Duration> tp ) {
				using days = std::chrono::duration<long, std::ratio<86400>>;
				auto const z =
				  std::chrono::duration_cast<days>( tp.time_since_epoch( ) ).count( );
				auto const dow = z >= -4L ? ( z + 4L ) % 7L : ( z + 5L ) % 7L + 6L;
				switch( dow ) {
				case 0:
					return { "Sun" };
				case 1:
					return { "Mon" };
				case 2:
					return { "Tue" };
				case 3:
					return { "Wed" };
				case 4:
					return { "Thu" };
				case 5:
					return { "Fri" };
				case 6:
					return { "Sat" };
				default:
					DAW_UNLIKELY_BRANCH
					daw_json_error( ErrorReason::InvalidTimestamp ); // Invalid month
				}
			}

			namespace datetime_details {
				constexpr std::uint_least32_t month2num( std::string_view ts ) {
					daw_json_ensure( std::size( ts ) >= 3,
					                 ErrorReason::InvalidTimestamp );
					auto const b0 = static_cast<std::uint_least32_t>(
					  static_cast<unsigned char>( ts[0] ) );
					auto const b1 = static_cast<std::uint_least32_t>(
					  static_cast<unsigned char>( ts[1] ) );
					auto const b2 = static_cast<std::uint_least32_t>(
					  static_cast<unsigned char>( ts[2] ) );
					return ( b0 << 16U ) | ( b1 << 8U ) | b2;
				}
			} // namespace datetime_details

			constexpr unsigned parse_short_month( std::string_view ts ) {
				// Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
				switch( datetime_details::month2num( ts ) ) {
				case datetime_details::month2num( "Jan" ):
					return 1;
				case datetime_details::month2num( "Feb" ):
					return 2;
				case datetime_details::month2num( "Mar" ):
					return 3;
				case datetime_details::month2num( "Apr" ):
					return 4;
				case datetime_details::month2num( "May" ):
					return 5;
				case datetime_details::month2num( "Jun" ):
					return 6;
				case datetime_details::month2num( "Jul" ):
					return 7;
				case datetime_details::month2num( "Aug" ):
					return 8;
				case datetime_details::month2num( "Sep" ):
					return 9;
				case datetime_details::month2num( "Oct" ):
					return 10;
				case datetime_details::month2num( "Nov" ):
					return 11;
				case datetime_details::month2num( "Dec" ):
					return 12;
				default:
					DAW_UNLIKELY_BRANCH
					daw_json_error( ErrorReason::InvalidTimestamp ); // Invalid month
				}
			}
		} // namespace datetime
	} // namespace DAW_JSON_VER
} // namespace daw::json