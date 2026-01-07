/*
 * Check handle for fvdecheck
 *
 * Copyright (C) 2011-2025, Omar Choudary <choudary.omar@gmail.com>
 *                          Joachim Metz <joachim.metz@gmail.com>
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <byte_stream.h>
#include <file_stream.h>
#include <memory.h>
#include <narrow_string.h>
#include <system_string.h>
#include <types.h>
#include <wide_string.h>

#include "check_handle.h"
#include "fvdecheck_extent.h"
#include "fvdetools_libbfio.h"
#include "fvdetools_libcerror.h"
#include "fvdetools_libfguid.h"
#include "fvdetools_libfvde.h"
#include "fvdetools_libuna.h"

/* Include internal libfvde headers for segment descriptor access */
#include "../libfvde/libfvde_segment_descriptor.h"
#include "../libfvde/libfvde_logical_volume_descriptor.h"

/* Forward declarations for internal libfvde structures */
typedef struct libfvde_volume_header libfvde_volume_header_t;
typedef struct libfvde_metadata libfvde_metadata_t;

/* Minimal internal structure definitions needed for metadata access */
struct libfvde_volume_header
{
	uint8_t signature[ 8 ];
	uint32_t checksum;
	uint32_t initial_value;
	uint64_t physical_volume_size;
	uint64_t unknown1;
	uint64_t unknown2;
	uint8_t physical_volume_identifier[ 16 ];
	uint32_t block_size;
	uint64_t metadata_size;
	uint64_t metadata_offsets[ 4 ];
};

struct libfvde_metadata
{
	uint64_t encrypted_metadata1_offset;
	uint16_t encrypted_metadata1_volume_index;
	uint64_t encrypted_metadata2_offset;
	uint16_t encrypted_metadata2_volume_index;
	uint64_t encrypted_metadata_size;
};

struct libfvde_internal_volume
{
	libfvde_volume_header_t *volume_header;
	libfvde_metadata_t *metadata;
	/* Additional fields exist but we don't need to list them all */
};

#if !defined( LIBFVDE_HAVE_BFIO )

extern \
int libfvde_volume_open_file_io_handle(
     libfvde_volume_t *volume,
     libbfio_handle_t *file_io_handle,
     int access_flags,
     libfvde_error_t **error );

extern \
int libfvde_volume_open_physical_volume_files_file_io_pool(
     libfvde_volume_t *handle,
     libbfio_pool_t *file_io_pool,
     libcerror_error_t **error );

#endif /* !defined( LIBFVDE_HAVE_BFIO ) */

/* External declarations for internal libfvde functions */
extern \
int libfvde_logical_volume_get_logical_volume_descriptor(
     libfvde_logical_volume_t *logical_volume,
     libfvde_logical_volume_descriptor_t **logical_volume_descriptor,
     libcerror_error_t **error );

extern \
int libfvde_logical_volume_descriptor_get_number_of_segment_descriptors(
     libfvde_logical_volume_descriptor_t *logical_volume_descriptor,
     int *number_of_segment_descriptors,
     libcerror_error_t **error );

extern \
int libfvde_logical_volume_descriptor_get_segment_descriptor_by_index(
     libfvde_logical_volume_descriptor_t *logical_volume_descriptor,
     int segment_descriptor_index,
     libfvde_segment_descriptor_t **segment_descriptor,
     libcerror_error_t **error );

#define CHECK_HANDLE_NOTIFY_STREAM		stdout

/* Copies a string of a decimal value to a 64-bit value
 * Returns 1 if successful or -1 on error
 */
int fvdetools_system_string_copy_from_64_bit_in_decimal(
     const system_character_t *string,
     size_t string_size,
     uint64_t *value_64bit,
     libcerror_error_t **error )
{
	static char *function              = "fvdetools_system_string_copy_from_64_bit_in_decimal";
	size_t string_index                = 0;
	system_character_t character_value = 0;
	uint8_t maximum_string_index       = 20;
	int8_t sign                        = 1;

	if( string == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid string.",
		 function );

		return( -1 );
	}
	if( string_size > (size_t) SSIZE_MAX )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid string size value exceeds maximum.",
		 function );

		return( -1 );
	}
	if( value_64bit == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid value 64-bit.",
		 function );

		return( -1 );
	}
	*value_64bit = 0;

	if( string[ string_index ] == (system_character_t) '-' )
	{
		string_index++;
		maximum_string_index++;

		sign = -1;
	}
	else if( string[ string_index ] == (system_character_t) '+' )
	{
		string_index++;
		maximum_string_index++;
	}
	while( string_index < string_size )
	{
		if( string[ string_index ] == 0 )
		{
			break;
		}
		if( string_index > (size_t) maximum_string_index )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_LARGE,
			 "%s: string too large.",
			 function );

			return( -1 );
		}
		*value_64bit *= 10;

		if( ( string[ string_index ] >= (system_character_t) '0' )
		 && ( string[ string_index ] <= (system_character_t) '9' ) )
		{
			character_value = (system_character_t) ( string[ string_index ] - (system_character_t) '0' );
		}
		else
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
			 "%s: unsupported character value: %" PRIc_SYSTEM " at index: %d.",
			 function,
			 string[ string_index ],
			 (int) string_index );

			return( -1 );
		}
		*value_64bit += character_value;

		string_index++;
	}
	if( sign == -1 )
	{
		*value_64bit *= (uint64_t) -1;
	}
	return( 1 );
}

/* Creates a check handle
 * Make sure the value check_handle is referencing, is set to NULL
 * Returns 1 if successful or -1 on error
 */
int check_handle_initialize(
     check_handle_t **check_handle,
     int unattended_mode,
     libcerror_error_t **error )
{
	static char *function = "check_handle_initialize";

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( *check_handle != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid check handle value already set.",
		 function );

		return( -1 );
	}
	*check_handle = memory_allocate_structure(
	                 check_handle_t );

	if( *check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create check handle.",
		 function );

		goto on_error;
	}
	if( memory_set(
	     *check_handle,
	     0,
	     sizeof( check_handle_t ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear check handle.",
		 function );

		memory_free(
		 *check_handle );

		*check_handle = NULL;

		return( -1 );
	}
	if( fvdecheck_volume_state_initialize(
	     &( ( *check_handle )->volume_state ),
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to initialize volume state.",
		 function );

		goto on_error;
	}
	( *check_handle )->notify_stream   = CHECK_HANDLE_NOTIFY_STREAM;
	( *check_handle )->unattended_mode = unattended_mode;
	( *check_handle )->processing_order = CHECK_HANDLE_ORDER_ASCENDING;

	return( 1 );

on_error:
	if( *check_handle != NULL )
	{
		memory_free(
		 *check_handle );

		*check_handle = NULL;
	}
	return( -1 );
}

/* Frees a check handle
 * Returns 1 if successful or -1 on error
 */
int check_handle_free(
     check_handle_t **check_handle,
     libcerror_error_t **error )
{
	static char *function = "check_handle_free";
	int result            = 1;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( *check_handle != NULL )
	{
		if( ( *check_handle )->physical_volume_file_io_pool != NULL )
		{
			if( check_handle_close(
			     *check_handle,
			     error ) != 0 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_CLOSE_FAILED,
				 "%s: unable to close check handle.",
				 function );

				result = -1;
			}
		}
		if( ( *check_handle )->volume_state != NULL )
		{
			if( fvdecheck_volume_state_free(
			     &( ( *check_handle )->volume_state ),
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
				 "%s: unable to free volume state.",
				 function );

				result = -1;
			}
		}
		if( memory_set(
		     ( *check_handle )->key_data,
		     0,
		     16 ) == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_MEMORY,
			 LIBCERROR_MEMORY_ERROR_SET_FAILED,
			 "%s: unable to clear key data.",
			 function );

			result = -1;
		}
		memory_free(
		 *check_handle );

		*check_handle = NULL;
	}
	return( result );
}

/* Signals the check handle to abort
 * Returns 1 if successful or -1 on error
 */
int check_handle_signal_abort(
     check_handle_t *check_handle,
     libcerror_error_t **error )
{
	static char *function = "check_handle_signal_abort";

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( check_handle->volume != NULL )
	{
		if( libfvde_volume_signal_abort(
		     check_handle->volume,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to signal volume to abort.",
			 function );

			return( -1 );
		}
	}
	check_handle->abort = 1;

	return( 1 );
}

/* Sets the key
 * Returns 1 if successful or -1 on error
 */
int check_handle_set_key(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error )
{
	static char *function   = "check_handle_set_key";
	size_t string_length    = 0;
	uint32_t base16_variant = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	string_length = system_string_length(
	                 string );

	if( memory_set(
	     check_handle->key_data,
	     0,
	     16 ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear key data.",
		 function );

		goto on_error;
	}
	base16_variant = LIBUNA_BASE16_VARIANT_RFC4648;

#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
	if( _BYTE_STREAM_HOST_IS_ENDIAN_BIG )
	{
		base16_variant |= LIBUNA_BASE16_VARIANT_ENCODING_UTF16_BIG_ENDIAN;
	}
	else
	{
		base16_variant |= LIBUNA_BASE16_VARIANT_ENCODING_UTF16_LITTLE_ENDIAN;
	}
#endif
	if( string_length != 32 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported string length.",
		 function );

		goto on_error;
	}
	if( libuna_base16_stream_copy_to_byte_stream(
	     (uint8_t *) string,
	     string_length,
	     check_handle->key_data,
	     16,
	     base16_variant,
	     0,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_COPY_FAILED,
		 "%s: unable to copy key data.",
		 function );

		goto on_error;
	}
	check_handle->key_data_size = 16;

	return( 1 );

on_error:
	memory_set(
	 check_handle->key_data,
	 0,
	 16 );

	check_handle->key_data_size = 0;

	return( -1 );
}

/* Sets the password
 * Returns 1 if successful or -1 on error
 */
int check_handle_set_password(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error )
{
	static char *function = "check_handle_set_password";
	size_t string_length  = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( string == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid string.",
		 function );

		return( -1 );
	}
	string_length = system_string_length(
	                 string );

	check_handle->user_password        = string;
	check_handle->user_password_length = string_length;

	return( 1 );
}

/* Sets the recovery password
 * Returns 1 if successful or -1 on error
 */
int check_handle_set_recovery_password(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error )
{
	static char *function = "check_handle_set_recovery_password";
	size_t string_length  = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( string == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid string.",
		 function );

		return( -1 );
	}
	string_length = system_string_length(
	                 string );

	check_handle->recovery_password        = string;
	check_handle->recovery_password_length = string_length;

	return( 1 );
}

/* Sets the path of the EncryptedRoot.plist.wipekey file
 * Returns 1 if successful or -1 on error
 */
int check_handle_set_encrypted_root_plist(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error )
{
	static char *function = "check_handle_set_encrypted_root_plist";

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( string == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid string.",
		 function );

		return( -1 );
	}
	check_handle->encrypted_root_plist_path = string;

	return( 1 );
}

/* Sets the volume offset
 * Returns 1 if successful or -1 on error
 */
int check_handle_set_volume_offset(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error )
{
	static char *function = "check_handle_set_volume_offset";
	size_t string_length  = 0;
	uint64_t value_64bit  = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	string_length = system_string_length(
	                 string );

	if( fvdetools_system_string_copy_from_64_bit_in_decimal(
	     string,
	     string_length + 1,
	     &value_64bit,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_COPY_FAILED,
		 "%s: unable to copy string to 64-bit decimal.",
		 function );

		return( -1 );
	}
	check_handle->volume_offset = (off64_t) value_64bit;

	return( 1 );
}

/* Sets the processing order
 * Returns 1 if successful or -1 on error
 */
int check_handle_set_order(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error )
{
	static char *function = "check_handle_set_order";

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( string == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid string.",
		 function );

		return( -1 );
	}
	if( system_string_compare(
	     string,
	     _SYSTEM_STRING( "ascending" ),
	     9 ) == 0 )
	{
		check_handle->processing_order = CHECK_HANDLE_ORDER_ASCENDING;
	}
	else if( system_string_compare(
	          string,
	          _SYSTEM_STRING( "descending" ),
	          10 ) == 0 )
	{
		check_handle->processing_order = CHECK_HANDLE_ORDER_DESCENDING;
	}
	else if( system_string_compare(
	          string,
	          _SYSTEM_STRING( "physical" ),
	          8 ) == 0 )
	{
		check_handle->processing_order = CHECK_HANDLE_ORDER_PHYSICAL;
	}
	else
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported order value.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Sets stop at block
 * Returns 1 if successful or -1 on error
 */
int check_handle_set_stop_at_block(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error )
{
	static char *function = "check_handle_set_stop_at_block";
	size_t string_length  = 0;
	uint64_t value_64bit  = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	string_length = system_string_length(
	                 string );

	if( fvdetools_system_string_copy_from_64_bit_in_decimal(
	     string,
	     string_length + 1,
	     &value_64bit,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_COPY_FAILED,
		 "%s: unable to copy string to 64-bit decimal.",
		 function );

		return( -1 );
	}
	check_handle->stop_at_block = (uint32_t) value_64bit;

	return( 1 );
}

/* Sets stop at transaction
 * Returns 1 if successful or -1 on error
 */
int check_handle_set_stop_at_transaction(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error )
{
	static char *function = "check_handle_set_stop_at_transaction";
	size_t string_length  = 0;
	uint64_t value_64bit  = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	string_length = system_string_length(
	                 string );

	if( fvdetools_system_string_copy_from_64_bit_in_decimal(
	     string,
	     string_length + 1,
	     &value_64bit,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_COPY_FAILED,
		 "%s: unable to copy string to 64-bit decimal.",
		 function );

		return( -1 );
	}
	check_handle->stop_at_transaction = value_64bit;

	return( 1 );
}

/* Sets lookup linux sector
 * Returns 1 if successful or -1 on error
 */
int check_handle_set_lookup_linux_sector(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error )
{
	static char *function = "check_handle_set_lookup_linux_sector";
	size_t string_length  = 0;
	uint64_t value_64bit  = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	string_length = system_string_length(
	                 string );

	if( fvdetools_system_string_copy_from_64_bit_in_decimal(
	     string,
	     string_length + 1,
	     &value_64bit,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_COPY_FAILED,
		 "%s: unable to copy string to 64-bit decimal.",
		 function );

		return( -1 );
	}
	check_handle->lookup_linux_sector_set = 1;
	check_handle->lookup_linux_sector = value_64bit;

	return( 1 );
}

/* Opens the check handle
 * Returns 1 if successful or -1 on error
 */
int check_handle_open(
     check_handle_t *check_handle,
     system_character_t * const * filenames,
     int number_of_filenames,
     libcerror_error_t **error )
{
	libbfio_handle_t *file_io_handle         = NULL;
	libfvde_logical_volume_t *logical_volume = NULL;
	libfvde_physical_volume_t *physical_volume = NULL;
	static char *function                    = "check_handle_open";
	size_t filename_length                   = 0;
	size64_t volume_size                     = 0;
	uint8_t uuid_data[ 16 ];
	int filename_index                       = 0;
	int logical_volume_index                 = 0;
	int number_of_logical_volumes            = 0;
	int number_of_physical_volumes           = 0;
	int physical_volume_index                = 0;
	uint32_t pv_index                        = 0;
	uint32_t lv_index                        = 0;
	int result                               = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( check_handle->physical_volume_file_io_pool != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid check handle - physical volume file IO pool value already set.",
		 function );

		return( -1 );
	}
	if( check_handle->volume != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid check handle - volume value already set.",
		 function );

		return( -1 );
	}
	if( number_of_filenames <= 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_ZERO_OR_LESS,
		 "%s: invalid number of filenames.",
		 function );

		return( -1 );
	}
	if( libbfio_file_range_initialize(
	     &file_io_handle,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to initialize file IO handle: 0.",
		 function );

		goto on_error;
	}
	filename_length = system_string_length(
	                   filenames[ 0 ] );

#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
	if( libbfio_file_range_set_name_wide(
	     file_io_handle,
	     filenames[ 0 ],
	     filename_length,
	     error ) != 1 )
#else
	if( libbfio_file_range_set_name(
	     file_io_handle,
	     filenames[ 0 ],
	     filename_length,
	     error ) != 1 )
#endif
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_OPEN_FAILED,
		 "%s: unable to set name of file IO handle: 0.",
		 function );

		goto on_error;
	}
	if( libbfio_file_range_set(
	     file_io_handle,
	     check_handle->volume_offset,
	     0,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_OPEN_FAILED,
		 "%s: unable to set volume offset of file IO handle: 0.",
		 function );

		goto on_error;
	}
	if( libfvde_volume_initialize(
	     &( check_handle->volume ),
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to initialize volume.",
		 function );

		goto on_error;
	}
	if( check_handle->encrypted_root_plist_path != NULL )
	{
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
		if( libfvde_volume_read_encrypted_root_plist_wide(
		     check_handle->volume,
		     check_handle->encrypted_root_plist_path,
		     error ) != 1 )
#else
		if( libfvde_volume_read_encrypted_root_plist(
		     check_handle->volume,
		     check_handle->encrypted_root_plist_path,
		     error ) != 1 )
#endif
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read EncryptedRoot.plist.wipekey file.",
			 function );

			goto on_error;
		}
	}
	if( libfvde_volume_open_file_io_handle(
	     check_handle->volume,
	     file_io_handle,
	     LIBFVDE_OPEN_READ,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_OPEN_FAILED,
		 "%s: unable to open volume.",
		 function );

		goto on_error;
	}
	if( libbfio_pool_initialize(
	     &( check_handle->physical_volume_file_io_pool ),
	     number_of_filenames,
	     0,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to initialize physical volume file IO pool.",
		 function );

		goto on_error;
	}
	if( libbfio_pool_set_handle(
	     check_handle->physical_volume_file_io_pool,
	     0,
	     file_io_handle,
	     LIBBFIO_OPEN_READ,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to set file IO handle: 0 in pool.",
		 function );

		goto on_error;
	}
	file_io_handle = NULL;

	for( filename_index = 1;
	     filename_index < number_of_filenames;
	     filename_index++ )
	{
		if( libbfio_file_range_initialize(
		     &file_io_handle,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to initialize file IO handle: %d.",
			 function,
			 filename_index );

			goto on_error;
		}
		filename_length = system_string_length(
		                   filenames[ filename_index ] );

#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
		if( libbfio_file_range_set_name_wide(
		     file_io_handle,
		     filenames[ filename_index ],
		     filename_length,
		     error ) != 1 )
#else
		if( libbfio_file_range_set_name(
		     file_io_handle,
		     filenames[ filename_index ],
		     filename_length,
		     error ) != 1 )
#endif
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_OPEN_FAILED,
			 "%s: unable to set name of file IO handle: %d.",
			 function,
			 filename_index );

			goto on_error;
		}
		if( libbfio_file_range_set(
		     file_io_handle,
		     check_handle->volume_offset,
		     0,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_OPEN_FAILED,
			 "%s: unable to set volume offset of file IO handle: %d.",
			 function,
			 filename_index );

			goto on_error;
		}
		if( libbfio_pool_set_handle(
		     check_handle->physical_volume_file_io_pool,
		     filename_index,
		     file_io_handle,
		     LIBBFIO_OPEN_READ,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to set file IO handle: %d in pool.",
			 function,
			 filename_index );

			goto on_error;
		}
		file_io_handle = NULL;
	}
	if( libfvde_volume_open_physical_volume_files_file_io_pool(
	     check_handle->volume,
	     check_handle->physical_volume_file_io_pool,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_OPEN_FAILED,
		 "%s: unable to open physical volume files.",
		 function );

		goto on_error;
	}
	if( libfvde_volume_get_volume_group(
	     check_handle->volume,
	     &( check_handle->volume_group ),
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve volume group.",
		 function );

		goto on_error;
	}
	/* Get physical volumes and add to state */
	if( libfvde_volume_group_get_number_of_physical_volumes(
	     check_handle->volume_group,
	     &number_of_physical_volumes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of physical volumes.",
		 function );

		goto on_error;
	}
	for( physical_volume_index = 0;
	     physical_volume_index < number_of_physical_volumes;
	     physical_volume_index++ )
	{
		if( libfvde_volume_group_get_physical_volume_by_index(
		     check_handle->volume_group,
		     physical_volume_index,
		     &physical_volume,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve physical volume: %d.",
			 function,
			 physical_volume_index );

			goto on_error;
		}
		if( libfvde_physical_volume_get_identifier(
		     physical_volume,
		     uuid_data,
		     16,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve physical volume identifier.",
			 function );

			goto on_error;
		}
		if( libfvde_physical_volume_get_size(
		     physical_volume,
		     &volume_size,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve physical volume size.",
			 function );

			goto on_error;
		}
		if( fvdecheck_volume_state_add_physical_volume(
		     check_handle->volume_state,
		     uuid_data,
		     volume_size / check_handle->volume_state->block_size,
		     &pv_index,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
			 "%s: unable to add physical volume to state.",
			 function );

			goto on_error;
		}
		/* Mark volume header as reserved (block 0) */
		if( fvdecheck_volume_state_mark_reserved(
		     check_handle->volume_state,
		     pv_index,
		     0,
		     1,
		     "Volume header",
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to mark volume header as reserved.",
			 function );

			goto on_error;
		}
		if( libfvde_physical_volume_free(
		     &physical_volume,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free physical volume: %d.",
			 function,
			 physical_volume_index );

			goto on_error;
		}
	}
	/* Get logical volumes and add to state */
	if( libfvde_volume_group_get_number_of_logical_volumes(
	     check_handle->volume_group,
	     &number_of_logical_volumes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of logical volumes.",
		 function );

		goto on_error;
	}
	for( logical_volume_index = 0;
	     logical_volume_index < number_of_logical_volumes;
	     logical_volume_index++ )
	{
		if( libfvde_volume_group_get_logical_volume_by_index(
		     check_handle->volume_group,
		     logical_volume_index,
		     &logical_volume,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve logical volume: %d.",
			 function,
			 logical_volume_index );

			goto on_error;
		}
		if( libfvde_logical_volume_get_identifier(
		     logical_volume,
		     uuid_data,
		     16,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve logical volume identifier.",
			 function );

			goto on_error;
		}
		if( libfvde_logical_volume_get_size(
		     logical_volume,
		     &volume_size,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve logical volume size.",
			 function );

			goto on_error;
		}
		if( fvdecheck_volume_state_add_logical_volume(
		     check_handle->volume_state,
		     uuid_data,
		     volume_size / check_handle->volume_state->block_size,
		     &lv_index,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
			 "%s: unable to add logical volume to state.",
			 function );

			goto on_error;
		}
		if( check_handle->key_data_size != 0 )
		{
			if( libfvde_logical_volume_set_key(
			     logical_volume,
			     check_handle->key_data,
			     16,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
				 "%s: unable to set key.",
				 function );

				goto on_error;
			}
		}
		if( check_handle->user_password != NULL )
		{
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
			if( libfvde_logical_volume_set_utf16_password(
			     logical_volume,
			     (uint16_t *) check_handle->user_password,
			     check_handle->user_password_length,
			     error ) != 1 )
#else
			if( libfvde_logical_volume_set_utf8_password(
			     logical_volume,
			     (uint8_t *) check_handle->user_password,
			     check_handle->user_password_length,
			     error ) != 1 )
#endif
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
				 "%s: unable to set password.",
				 function );

				goto on_error;
			}
		}
		if( check_handle->recovery_password != NULL )
		{
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
			if( libfvde_logical_volume_set_utf16_recovery_password(
			     logical_volume,
			     (uint16_t *) check_handle->recovery_password,
			     check_handle->recovery_password_length,
			     error ) != 1 )
#else
			if( libfvde_logical_volume_set_utf8_recovery_password(
			     logical_volume,
			     (uint8_t *) check_handle->recovery_password,
			     check_handle->recovery_password_length,
			     error ) != 1 )
#endif
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
				 "%s: unable to set recovery password.",
				 function );

				goto on_error;
			}
		}
		result = libfvde_logical_volume_unlock(
		          logical_volume,
		          error );

		if( result == -1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to unlock logical volume.",
			 function );

			goto on_error;
		}
		if( libfvde_logical_volume_free(
		     &logical_volume,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free logical volume: %d.",
			 function,
			 logical_volume_index );

			goto on_error;
		}
	}
	return( 1 );

on_error:
	if( logical_volume != NULL )
	{
		libfvde_logical_volume_free(
		 &logical_volume,
		 NULL );
	}
	if( physical_volume != NULL )
	{
		libfvde_physical_volume_free(
		 &physical_volume,
		 NULL );
	}
	if( check_handle->volume_group != NULL )
	{
		libfvde_volume_group_free(
		 &( check_handle->volume_group ),
		 NULL );
	}
	if( check_handle->volume != NULL )
	{
		libfvde_volume_free(
		 &( check_handle->volume ),
		 NULL );
	}
	if( check_handle->physical_volume_file_io_pool != NULL )
	{
		libbfio_pool_free(
		 &( check_handle->physical_volume_file_io_pool ),
		 NULL );
	}
	if( file_io_handle != NULL )
	{
		libbfio_handle_free(
		 &file_io_handle,
		 NULL );
	}
	return( -1 );
}

/* Closes the check handle
 * Returns the 0 if succesful or -1 on error
 */
int check_handle_close(
     check_handle_t *check_handle,
     libcerror_error_t **error )
{
	static char *function = "check_handle_close";
	int result            = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( check_handle->volume_group != NULL )
	{
		if( libfvde_volume_group_free(
		     &( check_handle->volume_group ),
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free volume group.",
			 function );

			result = -1;
		}
	}
	if( check_handle->volume != NULL )
	{
		if( libfvde_volume_close(
		     check_handle->volume,
		     error ) != 0 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_CLOSE_FAILED,
			 "%s: unable to close volume.",
			 function );

			result = -1;
		}
		if( libfvde_volume_free(
		     &( check_handle->volume ),
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free volume.",
			 function );

			result = -1;
		}
	}
	if( check_handle->physical_volume_file_io_pool != NULL )
	{
		if( libbfio_pool_close_all(
		     check_handle->physical_volume_file_io_pool,
		     error ) != 0 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_CLOSE_FAILED,
			 "%s: unable to close physical volume file IO pool.",
			 function );

			result = -1;
		}
		if( libbfio_pool_free(
		     &( check_handle->physical_volume_file_io_pool ),
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free physical volume file IO pool.",
			 function );

			result = -1;
		}
	}
	return( result );
}

/* Mark metadata regions as reserved
 * Returns 1 if successful or -1 on error
 */
int check_handle_mark_metadata_reserved(
     check_handle_t *check_handle,
     libcerror_error_t **error )
{
	static const char *metadata_descriptions[ 4 ] = {
		"Metadata block 1",
		"Metadata block 2",
		"Metadata block 3",
		"Metadata block 4"
	};
	libfvde_internal_volume_t *internal_volume = NULL;
	static char *function                      = "check_handle_mark_metadata_reserved";
	uint64_t metadata_size                     = 0;
	uint32_t pv_index                          = 0;
	int metadata_index                         = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( check_handle->volume == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid check handle - missing volume.",
		 function );

		return( -1 );
	}
	/* Cast to internal volume to access metadata offsets */
	internal_volume = (libfvde_internal_volume_t *) check_handle->volume;

	if( internal_volume->volume_header == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid volume - missing volume header.",
		 function );

		return( -1 );
	}
	/* Get metadata size from volume header */
	metadata_size = internal_volume->volume_header->metadata_size;

	/* Mark the 4 metadata block regions as reserved */
	for( metadata_index = 0;
	     metadata_index < 4;
	     metadata_index++ )
	{
		uint64_t metadata_offset = internal_volume->volume_header->metadata_offsets[ metadata_index ];
		uint64_t metadata_start_block = metadata_offset / check_handle->volume_state->block_size;
		uint64_t metadata_block_count = metadata_size / check_handle->volume_state->block_size;

		if( fvdecheck_volume_state_mark_reserved(
		     check_handle->volume_state,
		     pv_index,
		     metadata_start_block,
		     metadata_block_count,
		     metadata_descriptions[ metadata_index ],
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to mark metadata block %d as reserved.",
			 function,
			 metadata_index + 1 );

			return( -1 );
		}
	}
	/* Mark encrypted metadata regions if available */
	if( internal_volume->metadata != NULL )
	{
		uint64_t encrypted_metadata_size = internal_volume->metadata->encrypted_metadata_size;
		uint64_t encrypted_metadata1_offset = internal_volume->metadata->encrypted_metadata1_offset;
		uint64_t encrypted_metadata2_offset = internal_volume->metadata->encrypted_metadata2_offset;

		if( encrypted_metadata1_offset > 0 && encrypted_metadata_size > 0 )
		{
			uint64_t start_block = encrypted_metadata1_offset / check_handle->volume_state->block_size;
			uint64_t block_count = encrypted_metadata_size / check_handle->volume_state->block_size;

			if( fvdecheck_volume_state_mark_reserved(
			     check_handle->volume_state,
			     pv_index,
			     start_block,
			     block_count,
			     "Encrypted metadata 1",
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
				 "%s: unable to mark encrypted metadata 1 as reserved.",
				 function );

				return( -1 );
			}
		}
		if( encrypted_metadata2_offset > 0 && encrypted_metadata_size > 0 )
		{
			uint64_t start_block = encrypted_metadata2_offset / check_handle->volume_state->block_size;
			uint64_t block_count = encrypted_metadata_size / check_handle->volume_state->block_size;

			if( fvdecheck_volume_state_mark_reserved(
			     check_handle->volume_state,
			     pv_index,
			     start_block,
			     block_count,
			     "Encrypted metadata 2",
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
				 "%s: unable to mark encrypted metadata 2 as reserved.",
				 function );

				return( -1 );
			}
		}
	}
	return( 1 );
}

/* Process volume and build extent state
 * Returns 1 if successful or -1 on error
 */
int check_handle_process_volume(
     check_handle_t *check_handle,
     libcerror_error_t **error )
{
	libfvde_logical_volume_t *logical_volume = NULL;
	libfvde_logical_volume_descriptor_t *logical_volume_descriptor = NULL;
	libfvde_segment_descriptor_t *segment_descriptor = NULL;
	static char *function              = "check_handle_process_volume";
	uint32_t lv_index                  = 0;
	uint32_t pv_index                  = 0;
	int logical_volume_index           = 0;
	int number_of_logical_volumes      = 0;
	int number_of_segment_descriptors  = 0;
	int segment_descriptor_index       = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( check_handle->volume_group == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid check handle - missing volume group.",
		 function );

		return( -1 );
	}
	/* Mark metadata regions as reserved */
	if( check_handle_mark_metadata_reserved(
	     check_handle,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to mark metadata regions as reserved.",
		 function );

		return( -1 );
	}
	/* Get number of logical volumes */
	if( libfvde_volume_group_get_number_of_logical_volumes(
	     check_handle->volume_group,
	     &number_of_logical_volumes,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of logical volumes.",
		 function );

		return( -1 );
	}
	/* Process each logical volume */
	for( logical_volume_index = 0;
	     logical_volume_index < number_of_logical_volumes;
	     logical_volume_index++ )
	{
		if( libfvde_volume_group_get_logical_volume_by_index(
		     check_handle->volume_group,
		     logical_volume_index,
		     &logical_volume,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve logical volume: %d.",
			 function,
			 logical_volume_index );

			goto on_error;
		}
		/* Get the logical volume descriptor which contains segment descriptors */
		if( libfvde_logical_volume_get_logical_volume_descriptor(
		     logical_volume,
		     &logical_volume_descriptor,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve logical volume descriptor for volume: %d.",
			 function,
			 logical_volume_index );

			goto on_error;
		}
		if( logical_volume_descriptor == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
			 "%s: missing logical volume descriptor for volume: %d.",
			 function,
			 logical_volume_index );

			goto on_error;
		}
		/* Get number of segment descriptors (extent mappings) */
		if( libfvde_logical_volume_descriptor_get_number_of_segment_descriptors(
		     logical_volume_descriptor,
		     &number_of_segment_descriptors,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve number of segment descriptors.",
			 function );

			goto on_error;
		}
		/* Process each segment descriptor (extent mapping) */
		for( segment_descriptor_index = 0;
		     segment_descriptor_index < number_of_segment_descriptors;
		     segment_descriptor_index++ )
		{
			if( libfvde_logical_volume_descriptor_get_segment_descriptor_by_index(
			     logical_volume_descriptor,
			     segment_descriptor_index,
			     &segment_descriptor,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve segment descriptor: %d.",
				 function,
				 segment_descriptor_index );

				goto on_error;
			}
			if( segment_descriptor == NULL )
			{
				continue;
			}
			/* Map the segment descriptor to our volume state indices */
			pv_index = (uint32_t) segment_descriptor->physical_volume_index;
			lv_index = (uint32_t) logical_volume_index;

			/* Mark the extent as allocated */
			if( fvdecheck_volume_state_mark_allocated(
			     check_handle->volume_state,
			     pv_index,
			     segment_descriptor->physical_block_number,
			     segment_descriptor->number_of_blocks,
			     lv_index,
			     segment_descriptor->logical_block_number,
			     0,  /* transaction_id - not tracked for now */
			     0,  /* metadata_block_index - not tracked for now */
			     0x0305,  /* block_type - using 0x0305 as generic allocation */
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
				 "%s: unable to mark extent as allocated.",
				 function );

				goto on_error;
			}
		}
		if( libfvde_logical_volume_free(
		     &logical_volume,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free logical volume: %d.",
			 function,
			 logical_volume_index );

			goto on_error;
		}
	}
	return( 1 );

on_error:
	if( logical_volume != NULL )
	{
		libfvde_logical_volume_free(
		 &logical_volume,
		 NULL );
	}
	return( -1 );
}

/* Prints an UUID value
 * Returns 1 if successful or -1 on error
 */
int check_handle_uuid_value_fprint(
     check_handle_t *check_handle,
     const char *value_name,
     const uint8_t *uuid_data,
     libcerror_error_t **error )
{
	system_character_t uuid_string[ 48 ];

	libfguid_identifier_t *uuid = NULL;
	static char *function       = "check_handle_uuid_value_fprint";
	int result                  = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( libfguid_identifier_initialize(
	     &uuid,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create UUID.",
		 function );

		goto on_error;
	}
	if( libfguid_identifier_copy_from_byte_stream(
	     uuid,
	     uuid_data,
	     16,
	     LIBFGUID_ENDIAN_BIG,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_COPY_FAILED,
		 "%s: unable to copy byte stream to UUID.",
		 function );

		goto on_error;
	}
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
	result = libfguid_identifier_copy_to_utf16_string(
		  uuid,
		  (uint16_t *) uuid_string,
		  48,
		  LIBFGUID_STRING_FORMAT_FLAG_USE_LOWER_CASE,
		  error );
#else
	result = libfguid_identifier_copy_to_utf8_string(
		  uuid,
		  (uint8_t *) uuid_string,
		  48,
		  LIBFGUID_STRING_FORMAT_FLAG_USE_LOWER_CASE,
		  error );
#endif
	if( result != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_COPY_FAILED,
		 "%s: unable to copy UUID to string.",
		 function );

		goto on_error;
	}
	fprintf(
	 check_handle->notify_stream,
	 "%s: %" PRIs_SYSTEM "\n",
	 value_name,
	 uuid_string );

	if( libfguid_identifier_free(
	     &uuid,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
		 "%s: unable to free UUID.",
		 function );

		goto on_error;
	}
	return( 1 );

on_error:
	if( uuid != NULL )
	{
		libfguid_identifier_free(
		 &uuid,
		 NULL );
	}
	return( -1 );
}

/* Perform block lookup
 * Returns 1 if successful or -1 on error
 */
int check_handle_lookup_block(
     check_handle_t *check_handle,
     libcerror_error_t **error )
{
	fvdecheck_extent_t *extent = NULL;
	static char *function      = "check_handle_lookup_block";
	uint64_t block_number      = 0;
	uint64_t byte_offset       = 0;
	uint32_t pv_index          = 0;
	uint32_t lv_index          = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	fprintf( check_handle->notify_stream, "\nBlock Information:\n" );

	if( check_handle->lookup_linux_sector_set )
	{
		byte_offset = check_handle->lookup_linux_sector * 512;
		block_number = fvdecheck_linux_sector_to_fvde_block(
		                check_handle->lookup_linux_sector,
		                check_handle->volume_state->block_size );
		pv_index = 0;

		fprintf(
		 check_handle->notify_stream,
		 "  Linux sector:       %" PRIu64 " (512-byte sectors)\n",
		 check_handle->lookup_linux_sector );

		fprintf(
		 check_handle->notify_stream,
		 "  Linux byte offset:  %" PRIu64 " (0x%" PRIx64 ")\n",
		 byte_offset,
		 byte_offset );

		fprintf(
		 check_handle->notify_stream,
		 "\n  FVDE physical:\n" );

		fprintf(
		 check_handle->notify_stream,
		 "    Volume index:     %" PRIu32 "\n",
		 pv_index );

		fprintf(
		 check_handle->notify_stream,
		 "    Block number:     %" PRIu64 "\n",
		 block_number );

		extent = fvdecheck_volume_state_find_physical_extent(
		          check_handle->volume_state,
		          pv_index,
		          block_number );

		if( extent != NULL )
		{
			fprintf(
			 check_handle->notify_stream,
			 "\n  State:              %s\n",
			 fvdecheck_extent_state_to_string( extent->state ) );

			if( extent->state == FVDECHECK_EXTENT_STATE_RESERVED )
			{
				fprintf(
				 check_handle->notify_stream,
				 "  Reserved for:       %s\n",
				 extent->reserved_description != NULL ? extent->reserved_description : "Unknown" );
			}
			else if( extent->state == FVDECHECK_EXTENT_STATE_ALLOCATED )
			{
				fprintf(
				 check_handle->notify_stream,
				 "  Allocated by:       Transaction %" PRIu64 ", 0x%04" PRIx16 "\n",
				 extent->transaction_id,
				 extent->block_type );

				fprintf(
				 check_handle->notify_stream,
				 "\n  FVDE logical:\n" );

				fprintf(
				 check_handle->notify_stream,
				 "    Volume index:     %" PRIu32 "\n",
				 extent->logical_volume_index );

				fprintf(
				 check_handle->notify_stream,
				 "    Block number:     %" PRIu64 "\n",
				 extent->logical_block_start + ( block_number - extent->physical_block_start ) );
			}
			else if( extent->state == FVDECHECK_EXTENT_STATE_FREE )
			{
				fprintf(
				 check_handle->notify_stream,
				 "  Freed by:           Transaction %" PRIu64 ", 0x%04" PRIx16 "\n",
				 extent->transaction_id,
				 extent->block_type );
			}
			fprintf(
			 check_handle->notify_stream,
			 "\n  Extent context:\n" );

			fprintf(
			 check_handle->notify_stream,
			 "    Physical extent:  PV%" PRIu32 " blocks %" PRIu64 "-%" PRIu64 " (%" PRIu64 " blocks)\n",
			 extent->physical_volume_index,
			 extent->physical_block_start,
			 extent->physical_block_start + extent->physical_block_count - 1,
			 extent->physical_block_count );

			if( extent->state == FVDECHECK_EXTENT_STATE_ALLOCATED )
			{
				fprintf(
				 check_handle->notify_stream,
				 "    Logical extent:   LV%" PRIu32 " blocks %" PRIu64 "-%" PRIu64 " (%" PRIu64 " blocks)\n",
				 extent->logical_volume_index,
				 extent->logical_block_start,
				 extent->logical_block_start + extent->physical_block_count - 1,
				 extent->physical_block_count );
			}
		}
		else
		{
			fprintf(
			 check_handle->notify_stream,
			 "\n  State:              UNKNOWN (not in any tracked extent)\n" );
		}
	}
	fprintf( check_handle->notify_stream, "\n" );

	return( 1 );
}

/* Print allocation summary
 * Returns 1 if successful or -1 on error
 */
int check_handle_print_allocation_summary(
     check_handle_t *check_handle,
     libcerror_error_t **error )
{
	static char *function = "check_handle_print_allocation_summary";
	uint32_t pv_index     = 0;
	uint32_t lv_index     = 0;
	uint64_t total        = 0;
	double percent        = 0.0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( fvdecheck_volume_state_calculate_statistics(
	     check_handle->volume_state,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GENERIC,
		 "%s: unable to calculate statistics.",
		 function );

		return( -1 );
	}
	fprintf( check_handle->notify_stream, "\nAllocation Summary:\n" );

	for( pv_index = 0;
	     pv_index < check_handle->volume_state->num_physical_volumes;
	     pv_index++ )
	{
		total = check_handle->volume_state->physical_volumes[ pv_index ].size_in_blocks;

		fprintf(
		 check_handle->notify_stream,
		 "\nPhysical Volume %" PRIu32 ":\n",
		 pv_index );

		check_handle_uuid_value_fprint(
		 check_handle,
		 "  Identifier",
		 check_handle->volume_state->physical_volumes[ pv_index ].uuid,
		 error );

		fprintf(
		 check_handle->notify_stream,
		 "  Total blocks:     %" PRIu64 "\n",
		 total );

		if( total > 0 )
		{
			percent = ( (double) check_handle->volume_state->physical_volumes[ pv_index ].reserved_blocks / (double) total ) * 100.0;
		}
		fprintf(
		 check_handle->notify_stream,
		 "  Reserved:         %" PRIu64 " (%.2f%%)\n",
		 check_handle->volume_state->physical_volumes[ pv_index ].reserved_blocks,
		 percent );

		if( total > 0 )
		{
			percent = ( (double) check_handle->volume_state->physical_volumes[ pv_index ].allocated_blocks / (double) total ) * 100.0;
		}
		fprintf(
		 check_handle->notify_stream,
		 "  Allocated:        %" PRIu64 " (%.2f%%)\n",
		 check_handle->volume_state->physical_volumes[ pv_index ].allocated_blocks,
		 percent );

		if( total > 0 )
		{
			percent = ( (double) check_handle->volume_state->physical_volumes[ pv_index ].free_blocks / (double) total ) * 100.0;
		}
		fprintf(
		 check_handle->notify_stream,
		 "  Free:             %" PRIu64 " (%.2f%%)\n",
		 check_handle->volume_state->physical_volumes[ pv_index ].free_blocks,
		 percent );
	}
	for( lv_index = 0;
	     lv_index < check_handle->volume_state->num_logical_volumes;
	     lv_index++ )
	{
		total = check_handle->volume_state->logical_volumes[ lv_index ].size_in_blocks;

		fprintf(
		 check_handle->notify_stream,
		 "\nLogical Volume %" PRIu32 ":\n",
		 lv_index );

		check_handle_uuid_value_fprint(
		 check_handle,
		 "  Identifier",
		 check_handle->volume_state->logical_volumes[ lv_index ].uuid,
		 error );

		fprintf(
		 check_handle->notify_stream,
		 "  Total blocks:     %" PRIu64 "\n",
		 total );

		if( total > 0 )
		{
			percent = ( (double) check_handle->volume_state->logical_volumes[ lv_index ].mapped_blocks / (double) total ) * 100.0;
		}
		fprintf(
		 check_handle->notify_stream,
		 "  Mapped:           %" PRIu64 " (%.2f%%)\n",
		 check_handle->volume_state->logical_volumes[ lv_index ].mapped_blocks,
		 percent );

		fprintf(
		 check_handle->notify_stream,
		 "  Unmapped:         %" PRIu64 "\n",
		 check_handle->volume_state->logical_volumes[ lv_index ].unmapped_blocks );
	}
	fprintf( check_handle->notify_stream, "\n" );
	fprintf(
	 check_handle->notify_stream,
	 "Total extents tracked: %" PRIu64 "\n",
	 check_handle->volume_state->total_extents );

	fprintf(
	 check_handle->notify_stream,
	 "Errors: %" PRIu32 "\n",
	 check_handle->volume_state->error_count );

	fprintf(
	 check_handle->notify_stream,
	 "Warnings: %" PRIu32 "\n",
	 check_handle->volume_state->warning_count );

	return( 1 );
}

/* Print allocation map
 * Returns 1 if successful or -1 on error
 */
int check_handle_print_allocation_map(
     check_handle_t *check_handle,
     libcerror_error_t **error )
{
	fvdecheck_extent_t *extent = NULL;
	static char *function      = "check_handle_print_allocation_map";
	uint32_t pv_index          = 0;
	int extent_count           = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	fprintf( check_handle->notify_stream, "\nAllocation Map:\n" );

	for( pv_index = 0;
	     pv_index < check_handle->volume_state->num_physical_volumes;
	     pv_index++ )
	{
		fprintf(
		 check_handle->notify_stream,
		 "\nPhysical Volume %" PRIu32 " Extents:\n",
		 pv_index );

		extent = check_handle->volume_state->physical_volumes[ pv_index ].extent_list_head;
		extent_count = 0;

		while( extent != NULL )
		{
			fprintf(
			 check_handle->notify_stream,
			 "  [%-9s] blocks %" PRIu64 "-%" PRIu64 " (%" PRIu64 " blocks)",
			 fvdecheck_extent_state_to_string( extent->state ),
			 extent->physical_block_start,
			 extent->physical_block_start + extent->physical_block_count - 1,
			 extent->physical_block_count );

			if( extent->state == FVDECHECK_EXTENT_STATE_ALLOCATED )
			{
				fprintf(
				 check_handle->notify_stream,
				 " -> LV%" PRIu32 ":%" PRIu64 "-%" PRIu64,
				 extent->logical_volume_index,
				 extent->logical_block_start,
				 extent->logical_block_start + extent->physical_block_count - 1 );
			}
			else if( extent->state == FVDECHECK_EXTENT_STATE_RESERVED )
			{
				fprintf(
				 check_handle->notify_stream,
				 " - %s",
				 extent->reserved_description != NULL ? extent->reserved_description : "Reserved" );
			}
			fprintf( check_handle->notify_stream, "\n" );

			extent = extent->phys_next;
			extent_count++;

			/* Limit output for very large maps */
			if( extent_count >= 1000 && !check_handle->verbose_mode )
			{
				fprintf(
				 check_handle->notify_stream,
				 "  ... (%" PRIu64 " more extents, use -v for full list)\n",
				 check_handle->volume_state->total_extents - extent_count );

				break;
			}
		}
	}
	return( 1 );
}

/* Print results in JSON format
 * Returns 1 if successful or -1 on error
 */
int check_handle_print_json(
     check_handle_t *check_handle,
     libcerror_error_t **error )
{
	system_character_t uuid_string[ 48 ];
	libfguid_identifier_t *uuid = NULL;
	static char *function       = "check_handle_print_json";
	uint32_t pv_index           = 0;
	uint32_t lv_index           = 0;
	int result                  = 0;

	if( check_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid check handle.",
		 function );

		return( -1 );
	}
	if( fvdecheck_volume_state_calculate_statistics(
	     check_handle->volume_state,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GENERIC,
		 "%s: unable to calculate statistics.",
		 function );

		return( -1 );
	}
	if( libfguid_identifier_initialize(
	     &uuid,
	     error ) != 1 )
	{
		return( -1 );
	}
	fprintf( check_handle->notify_stream, "{\n" );
	fprintf( check_handle->notify_stream, "  \"volume\": {\n" );
	fprintf( check_handle->notify_stream, "    \"physical_volumes\": [\n" );

	for( pv_index = 0;
	     pv_index < check_handle->volume_state->num_physical_volumes;
	     pv_index++ )
	{
		if( libfguid_identifier_copy_from_byte_stream(
		     uuid,
		     check_handle->volume_state->physical_volumes[ pv_index ].uuid,
		     16,
		     LIBFGUID_ENDIAN_BIG,
		     error ) != 1 )
		{
			goto on_error;
		}
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
		result = libfguid_identifier_copy_to_utf16_string(
			  uuid,
			  (uint16_t *) uuid_string,
			  48,
			  LIBFGUID_STRING_FORMAT_FLAG_USE_LOWER_CASE,
			  error );
#else
		result = libfguid_identifier_copy_to_utf8_string(
			  uuid,
			  (uint8_t *) uuid_string,
			  48,
			  LIBFGUID_STRING_FORMAT_FLAG_USE_LOWER_CASE,
			  error );
#endif
		if( result != 1 )
		{
			goto on_error;
		}
		fprintf( check_handle->notify_stream, "      {\n" );
		fprintf( check_handle->notify_stream, "        \"index\": %" PRIu32 ",\n", pv_index );
		fprintf( check_handle->notify_stream, "        \"uuid\": \"%" PRIs_SYSTEM "\",\n", uuid_string );
		fprintf( check_handle->notify_stream, "        \"size_blocks\": %" PRIu64 ",\n",
		         check_handle->volume_state->physical_volumes[ pv_index ].size_in_blocks );
		fprintf( check_handle->notify_stream, "        \"block_size\": %" PRIu32 "\n",
		         check_handle->volume_state->block_size );
		fprintf( check_handle->notify_stream, "      }%s\n",
		         ( pv_index < check_handle->volume_state->num_physical_volumes - 1 ) ? "," : "" );
	}
	fprintf( check_handle->notify_stream, "    ],\n" );
	fprintf( check_handle->notify_stream, "    \"logical_volumes\": [\n" );

	for( lv_index = 0;
	     lv_index < check_handle->volume_state->num_logical_volumes;
	     lv_index++ )
	{
		if( libfguid_identifier_copy_from_byte_stream(
		     uuid,
		     check_handle->volume_state->logical_volumes[ lv_index ].uuid,
		     16,
		     LIBFGUID_ENDIAN_BIG,
		     error ) != 1 )
		{
			goto on_error;
		}
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
		result = libfguid_identifier_copy_to_utf16_string(
			  uuid,
			  (uint16_t *) uuid_string,
			  48,
			  LIBFGUID_STRING_FORMAT_FLAG_USE_LOWER_CASE,
			  error );
#else
		result = libfguid_identifier_copy_to_utf8_string(
			  uuid,
			  (uint8_t *) uuid_string,
			  48,
			  LIBFGUID_STRING_FORMAT_FLAG_USE_LOWER_CASE,
			  error );
#endif
		if( result != 1 )
		{
			goto on_error;
		}
		fprintf( check_handle->notify_stream, "      {\n" );
		fprintf( check_handle->notify_stream, "        \"index\": %" PRIu32 ",\n", lv_index );
		fprintf( check_handle->notify_stream, "        \"uuid\": \"%" PRIs_SYSTEM "\",\n", uuid_string );
		fprintf( check_handle->notify_stream, "        \"size_blocks\": %" PRIu64 "\n",
		         check_handle->volume_state->logical_volumes[ lv_index ].size_in_blocks );
		fprintf( check_handle->notify_stream, "      }%s\n",
		         ( lv_index < check_handle->volume_state->num_logical_volumes - 1 ) ? "," : "" );
	}
	fprintf( check_handle->notify_stream, "    ]\n" );
	fprintf( check_handle->notify_stream, "  },\n" );

	fprintf( check_handle->notify_stream, "  \"processing\": {\n" );
	fprintf( check_handle->notify_stream, "    \"order\": \"%s\",\n",
	         check_handle->processing_order == CHECK_HANDLE_ORDER_ASCENDING ? "ascending" :
	         check_handle->processing_order == CHECK_HANDLE_ORDER_DESCENDING ? "descending" : "physical" );
	fprintf( check_handle->notify_stream, "    \"transactions_processed\": %" PRIu32 ",\n",
	         check_handle->transactions_processed );
	fprintf( check_handle->notify_stream, "    \"metadata_blocks_processed\": %" PRIu32 "\n",
	         check_handle->metadata_blocks_processed );
	fprintf( check_handle->notify_stream, "  },\n" );

	fprintf( check_handle->notify_stream, "  \"allocation\": {\n" );
	fprintf( check_handle->notify_stream, "    \"physical\": {\n" );

	for( pv_index = 0;
	     pv_index < check_handle->volume_state->num_physical_volumes;
	     pv_index++ )
	{
		fprintf( check_handle->notify_stream, "      \"%" PRIu32 "\": {\n", pv_index );
		fprintf( check_handle->notify_stream, "        \"reserved_blocks\": %" PRIu64 ",\n",
		         check_handle->volume_state->physical_volumes[ pv_index ].reserved_blocks );
		fprintf( check_handle->notify_stream, "        \"allocated_blocks\": %" PRIu64 ",\n",
		         check_handle->volume_state->physical_volumes[ pv_index ].allocated_blocks );
		fprintf( check_handle->notify_stream, "        \"free_blocks\": %" PRIu64 "\n",
		         check_handle->volume_state->physical_volumes[ pv_index ].free_blocks );
		fprintf( check_handle->notify_stream, "      }%s\n",
		         ( pv_index < check_handle->volume_state->num_physical_volumes - 1 ) ? "," : "" );
	}
	fprintf( check_handle->notify_stream, "    },\n" );
	fprintf( check_handle->notify_stream, "    \"logical\": {\n" );

	for( lv_index = 0;
	     lv_index < check_handle->volume_state->num_logical_volumes;
	     lv_index++ )
	{
		fprintf( check_handle->notify_stream, "      \"%" PRIu32 "\": {\n", lv_index );
		fprintf( check_handle->notify_stream, "        \"mapped_blocks\": %" PRIu64 ",\n",
		         check_handle->volume_state->logical_volumes[ lv_index ].mapped_blocks );
		fprintf( check_handle->notify_stream, "        \"unmapped_blocks\": %" PRIu64 "\n",
		         check_handle->volume_state->logical_volumes[ lv_index ].unmapped_blocks );
		fprintf( check_handle->notify_stream, "      }%s\n",
		         ( lv_index < check_handle->volume_state->num_logical_volumes - 1 ) ? "," : "" );
	}
	fprintf( check_handle->notify_stream, "    }\n" );
	fprintf( check_handle->notify_stream, "  },\n" );

	fprintf( check_handle->notify_stream, "  \"errors\": [],\n" );
	fprintf( check_handle->notify_stream, "  \"warnings\": []\n" );
	fprintf( check_handle->notify_stream, "}\n" );

	libfguid_identifier_free(
	 &uuid,
	 NULL );

	return( 1 );

on_error:
	if( uuid != NULL )
	{
		libfguid_identifier_free(
		 &uuid,
		 NULL );
	}
	return( -1 );
}
