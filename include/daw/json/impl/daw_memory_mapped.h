// The MIT License (MIT)
//
// Copyright (c) 2019 Darrell Wright
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

#pragma once

#include <cstdio>
#ifndef WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>
#else
#include <algorithm>
#include <cstdio>
#include <string>
#include <tchar.h>
#include <windows.h>
#endif
#include <utility>

namespace daw {
	enum class open_mode : bool { read, read_write };

#ifndef WIN32
	template<typename T = char>
	struct memory_mapped_file {
		using value_type = T;
		using reference = T &;
		using const_reference =
		  std::conditional_t<std::is_const_v<T>, T, T const> &;
		using pointer = T *;
		using const_pointer = std::conditional_t<std::is_const_v<T>, T, T const> *;
		using size_type = size_t;

	private:
		int m_file = -1;
		pointer m_ptr = nullptr;
		size_type m_size = 0;

		void cleanup( ) noexcept {
			if( m_ptr != nullptr ) {
				munmap( m_ptr, m_size );
				m_ptr = nullptr;
			}
			m_size = 0;
			if( m_file >= 0 ) {
				close( m_file );
				m_file = -1;
			}
		}

	public:
		constexpr memory_mapped_file( ) noexcept = default;

		explicit memory_mapped_file( std::string_view file,
		                             open_mode mode = open_mode::read ) noexcept {

			(void)open( file, mode );
		}

		[[nodiscard]] bool open( std::string_view file,
		                         open_mode mode = open_mode::read ) noexcept {

			m_file =
			  ::open( file.data( ), mode == open_mode::read ? O_RDONLY : O_RDWR );
			if( m_file < 0 ) {
				return false;
			}
			{
				auto const fsz = lseek( m_file, 0, SEEK_END );
				lseek( m_file, 0, SEEK_SET );
				if( fsz <= 0 ) {
					cleanup( );
					return false;
				}
				m_size = static_cast<size_type>( fsz );
			}
			m_ptr = static_cast<pointer>(
			  mmap( nullptr, m_size,
			        mode == open_mode::read ? PROT_READ : PROT_READ | PROT_WRITE,
			        MAP_SHARED, m_file, 0 ) );

			if( m_ptr == MAP_FAILED ) {
				m_ptr = nullptr;
				cleanup( );
				return false;
			}
			return true;
		}

		[[nodiscard]] reference operator[]( size_type pos ) noexcept {
			return m_ptr[pos];
		}

		[[nodiscard]] const_reference operator[]( size_t pos ) const noexcept {
			return m_ptr[pos];
		}

		[[nodiscard]] constexpr pointer data( ) noexcept {
			return m_ptr;
		}

		[[nodiscard]] constexpr const_pointer data( ) const noexcept {
			return m_ptr;
		}

		[[nodiscard]] constexpr size_type size( ) const noexcept {
			return m_size;
		}

		constexpr explicit operator bool( ) const noexcept {
			return m_file >= 0 and m_ptr != nullptr;
		}

		memory_mapped_file( memory_mapped_file const & ) = delete;
		memory_mapped_file &operator=( memory_mapped_file const & ) = delete;

		memory_mapped_file( memory_mapped_file &&other ) noexcept
		  : m_file( std::exchange( other.m_file, -1 ) )
		  , m_ptr( std::exchange( other.m_ptr, nullptr ) )
		  , m_size( std::exchange( other.m_size, 0 ) ) {}

		memory_mapped_file &operator=( memory_mapped_file &&rhs ) noexcept {
			if( this != &rhs ) {
				m_file = std::exchange( rhs.m_file, -1 );
				m_ptr = std::exchange( rhs.m_ptr, nullptr );
				m_size = std::exchange( rhs.m_size, 0 );
			}
			return *this;
		}

		~memory_mapped_file( ) noexcept {
			cleanup( );
		}
	};
#else
	namespace mapfile_impl {
		static constexpr long CreateFileMode( open_mode m ) {
			if( m == open_mode::read ) {
				return GENERIC_READ;
			}
			return GENERIC_READ | GENERIC_WRITE;
		}

		static constexpr long PageMode( open_mode m ) {
			if( m == open_mode::read ) {
				return PAGE_READONLY;
			}
			return PAGE_READWRITE;
		}

		static constexpr long MapMode( open_mode m ) {
			if( m == open_mode::read ) {
				return FILE_MAP_READ;
			}
			return FILE_MAP_WRITE;
		}
	} // namespace mapfile_impl

	template<typename T = char>
	struct memory_mapped_file {
		using value_type = T;
		using reference = T &;
		using const_reference =
		  std::conditional_t<std::is_const_v<T>, T, T const> &;
		using pointer = T *;
		using const_pointer = std::conditional_t<std::is_const_v<T>, T, T const> *;
		using size_type = size_t;

	private:
		HANDLE m_handle = nullptr;
		size_t m_size = 0;
		pointer m_ptr = nullptr;

		void cleanup( ) noexcept {
			m_size = 0;
			if( auto tmp = std::exchange( m_ptr, nullptr ); tmp ) {
				UnmapViewOfFile( static_cast<LPVOID>( tmp ) );
			}
			if( auto tmp = std::exchange( m_handle, nullptr ); tmp ) {
				CloseHandle( m_handle );
			}
		}

	public:
		constexpr memory_mapped_file( ) noexcept = default;

		memory_mapped_file( std::string_view file,
		                    open_mode mode = open_mode::read ) noexcept {

            (void)open( file, mode );
		}

		[[nodiscard]] bool open( std::string_view file,
		                         open_mode mode = open_mode::read ) noexcept {

			{
				HANDLE file_handle =
				  CreateFile( file.data( ), mapfile_impl::CreateFileMode( mode ), 0, nullptr,
				              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
				if( file_handle == INVALID_HANDLE_VALUE ) {
					return false;
				}
				LARGE_INTEGER fsz;
				if( not GetFileSizeEx( file_handle, &fsz ) or fsz.QuadPart <= 0 ) {
					cleanup( );
					return false;
				}
				m_size = static_cast<size_t>( fsz.QuadPart );
				m_handle = CreateFileMapping( file_handle, nullptr, mapfile_impl::PageMode( mode ),
				                              fsz.u.HighPart, fsz.u.LowPart, nullptr );
				if( m_handle == NULL ) {
					cleanup( );
					return false;
				}
				CloseHandle( file_handle );
			}
			auto ptr = MapViewOfFile( m_handle, mapfile_impl::MapMode( mode ), 0, 0, 0 );
			if( ptr == nullptr ) {
				cleanup( );
				return false;
			}
			m_ptr = static_cast<pointer>( ptr );
			return true;
		}

		[[nodiscard]] reference operator[]( size_type pos ) noexcept {
			return m_ptr[pos];
		}

		[[nodiscard]] const_reference operator[]( size_t pos ) const noexcept {
			return m_ptr[pos];
		}

		[[nodiscard]] constexpr pointer data( ) noexcept {
			return m_ptr;
		}

		[[nodiscard]] constexpr const_pointer data( ) const noexcept {
			return m_ptr;
		}

		[[nodiscard]] constexpr size_type size( ) const noexcept {
			return m_size;
		}

		constexpr explicit operator bool( ) const noexcept {
			return m_size == 0 or m_ptr == nullptr or m_handle == nullptr;
		}

		memory_mapped_file( memory_mapped_file const & ) = delete;
		memory_mapped_file &operator=( memory_mapped_file const & ) = delete;

		memory_mapped_file( memory_mapped_file &&other ) noexcept
		  : m_handle( std::exchange( other.m_handle, nullptr ) )
		  , m_size( std::exchange( other.m_size, 0 ) )
		  , m_ptr( std::exchange( other.m_ptr, nullptr ) ) {}

		memory_mapped_file &operator=( memory_mapped_file &&rhs ) noexcept {
			if( this != &rhs ) {
				m_handle = std::exchange( rhs.m_handle, nullptr );
				m_size = std::exchange( rhs.m_size, 0 );
				m_ptr = std::exchange( rhs.m_ptr, nullptr );
			}
			return *this;
		}
		~memory_mapped_file( ) noexcept {
			cleanup( );
		}
	};
#endif
} // namespace daw
