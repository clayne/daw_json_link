// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_json_link
//

#pragma once

#include "version.h"

#include <daw/json/daw_json_exception.h>

#include <daw/daw_attributes.h>
#include <daw/daw_likely.h>
#include <daw/daw_not_null.h>

#include <cstddef>
#include <daw/stdinc/move_fwd_exch.h>
#include <exception>
#include <string_view>

#if defined( DAW_JSON_SHOW_ERROR_BEFORE_TERMINATE )
#include <iostream>
#endif

namespace daw::json {
#if defined( DAW_USE_EXCEPTIONS )
	inline constexpr bool use_daw_json_exceptions_v = true;
#else
	inline constexpr bool use_daw_json_exceptions_v = false;
#endif

	static thread_local void *daw_json_error_handler_data = nullptr;

#if defined( DAW_USE_EXCEPTIONS )
	[[noreturn, maybe_unused]] DAW_ATTRIB_NOINLINE inline void
	default_error_handler_throwing( json_exception &&jex, void * ) {
		throw std::move( jex );
	}
#endif

	[[noreturn, maybe_unused]] DAW_ATTRIB_NOINLINE inline void
	default_error_handler_terminating( json_exception &&jex, void * ) {
#if defined( DAW_JSON_SHOW_ERROR_BEFORE_TERMINATE )
		std::cerr << "Error: " << jex.reason( ) << '\n';
#else
		(void)jex;
#endif
		std::terminate( );
	}

	using daw_json_error_handler_t =
	  daw::not_null<void ( * )( json_exception &&, void * )>;

#if defined( DAW_USE_EXCEPTIONS )
	static thread_local daw_json_error_handler_t daw_json_error_handler =
	  default_error_handler_throwing;
#else
	static thread_local daw_json_error_handler_t daw_json_error_handler =
	  default_error_handler_terminating;
#endif

	inline namespace DAW_JSON_VER {
		namespace json_details {
			[[noreturn]] DAW_ATTRIB_NOINLINE inline void
			handle_error( json_exception &&jex ) {
				daw_json_error_handler.get( )( std::move( jex ), daw_json_error_handler_data );
				DAW_UNREACHABLE( );
			}
		} // namespace json_details

		[[noreturn]] DAW_ATTRIB_NOINLINE inline void
		daw_json_error( ErrorReason reason ) {
			json_details::handle_error( json_exception( reason ) );
		}

		template<typename ParseState>
		[[noreturn]] DAW_ATTRIB_NOINLINE inline void
		daw_json_error( ErrorReason reason, ParseState const &location ) {
			if( location.first ) {
				json_details::handle_error( json_exception( reason, location.first ) );
			}
			if( location.class_first ) {
				json_details::handle_error(
				  json_exception( reason, location.class_first ) );
			}
			json_details::handle_error( json_exception( reason ) );
		}

		[[noreturn]] DAW_ATTRIB_NOINLINE inline void
		daw_json_error( json_details::missing_member reason ) {
			json_details::handle_error( json_exception( reason ) );
		}

		[[noreturn]] DAW_ATTRIB_NOINLINE inline void
		daw_json_error( json_details::missing_token reason ) {
			json_details::handle_error( json_exception( reason ) );
		}

		template<typename ParseState>
		[[noreturn]] DAW_ATTRIB_NOINLINE inline void
		daw_json_error( json_details::missing_member reason,
		                ParseState const &location ) {
			if( location.class_first and location.first ) {
				static constexpr std::size_t max_len = 150;
				std::size_t const len = [&]( ) -> std::size_t {
					if( location.first == nullptr or location.class_first == nullptr ) {
						if( location.class_first == nullptr or
						    location.class_last == nullptr ) {
							return 0;
						}
						auto const dist = static_cast<std::size_t>( location.class_last -
						                                            location.class_first );
						if( dist < max_len ) {
							return dist;
						}
						return max_len;
					}
					auto const dist = static_cast<std::size_t>( location.class_first -
					                                            location.first + 1 );
					if( dist < max_len ) {
						return dist;
					}
					return max_len;
				}( );
				json_details::handle_error( json_exception(
				  reason, std::string_view( location.class_first, len ) ) );
			} else {
				json_details::handle_error( json_exception( reason ) );
			}
			DAW_UNREACHABLE( );
		}

		template<typename ParseState>
		[[noreturn]] DAW_ATTRIB_NOINLINE inline void
		daw_json_error( json_details::missing_token reason,
		                ParseState const &location ) {
			if( location.first ) {
				json_details::handle_error( json_exception( reason, location.first ) );
			}
			if( location.class_first ) {
				json_details::handle_error(
				  json_exception( reason, location.class_first ) );
			}
			json_details::handle_error( json_exception( reason ) );
		}
	} // namespace DAW_JSON_VER
} // namespace daw::json

/// @brief Ensure that Bool is true. If false pass rest of args to
/// daw_json_error
#define daw_json_ensure( Bool, ... )    \
	do {                                  \
		if( DAW_UNLIKELY( not( Bool ) ) ) { \
			daw_json_error( __VA_ARGS__ );    \
		}                                   \
	} while( false )

/// @brief Assert that Bool is true when in Checked Input mode If false pass
/// rest of args to daw_json_error
#define daw_json_assert_weak( Bool, ... )                \
	do {                                                   \
		if constexpr( not ParseState::is_unchecked_input ) { \
			if( DAW_UNLIKELY( not( Bool ) ) ) {                \
				daw_json_error( __VA_ARGS__ );                   \
			}                                                  \
		}                                                    \
	} while( false )
