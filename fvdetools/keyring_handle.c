/*
 * Kernel keyring handle
 *
 * Copyright (C) 2025, Dirk Tilger <dirk@systemication.com>
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
#include <memory.h>
#include <narrow_string.h>
#include <types.h>

#include <stdio.h>

#if defined( HAVE_STDLIB_H ) || defined( WINAPI )
#include <stdlib.h>
#endif

#if defined( HAVE_STRING_H )
#include <string.h>
#endif

#if defined( HAVE_ERRNO_H ) || defined( WINAPI )
#include <errno.h>
#endif

#if defined( HAVE_KEYUTILS_H )
#include <keyutils.h>
#endif

#include "fvdetools_libcerror.h"
#include "fvdetools_unused.h"
#include "keyring_handle.h"

/* Formats a UUID byte array into a string representation
 * Returns 1 if successful or -1 on error
 */
int keyring_handle_format_uuid_string(
     const uint8_t *uuid_data,
     size_t uuid_data_size,
     char *uuid_string,
     size_t uuid_string_size,
     libcerror_error_t **error )
{
	static char *function = "keyring_handle_format_uuid_string";

	if( uuid_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid UUID data.",
		 function );

		return( -1 );
	}
	if( uuid_data_size != 16 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid UUID data size value out of bounds.",
		 function );

		return( -1 );
	}
	if( uuid_string == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid UUID string.",
		 function );

		return( -1 );
	}
	if( uuid_string_size < 37 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_TOO_SMALL,
		 "%s: UUID string too small.",
		 function );

		return( -1 );
	}
	/* Format UUID as: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
	if( narrow_string_snprintf(
	     uuid_string,
	     uuid_string_size,
	     "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	     uuid_data[ 0 ],
	     uuid_data[ 1 ],
	     uuid_data[ 2 ],
	     uuid_data[ 3 ],
	     uuid_data[ 4 ],
	     uuid_data[ 5 ],
	     uuid_data[ 6 ],
	     uuid_data[ 7 ],
	     uuid_data[ 8 ],
	     uuid_data[ 9 ],
	     uuid_data[ 10 ],
	     uuid_data[ 11 ],
	     uuid_data[ 12 ],
	     uuid_data[ 13 ],
	     uuid_data[ 14 ],
	     uuid_data[ 15 ] ) < 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to format UUID string.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Adds encryption keys to the kernel keyring
 * Returns 1 if successful or -1 on error
 */
int keyring_handle_add_key(
     const uint8_t *volume_master_key,
     size_t volume_master_key_size,
     const uint8_t *volume_tweak_key,
     size_t volume_tweak_key_size,
     const uint8_t *volume_uuid,
     size_t volume_uuid_size,
     const char *keyring_id,
     int verbose,
     libcerror_error_t **error )
{
#if defined( HAVE_KEYUTILS_H )
	uint8_t combined_key[ 48 ];
	char key_description[ 64 ];
	char uuid_string[ 37 ];
	key_serial_t key_id     = 0;
	static char *function   = "keyring_handle_add_key";
	size_t combined_key_size = 0;
	int keyring_id_value    = 0;

	if( volume_master_key == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume master key.",
		 function );

		return( -1 );
	}
	if( volume_master_key_size != 16 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid volume master key size value out of bounds.",
		 function );

		return( -1 );
	}
	if( volume_tweak_key == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume tweak key.",
		 function );

		return( -1 );
	}
	if( volume_tweak_key_size != 32 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid volume tweak key size value out of bounds.",
		 function );

		return( -1 );
	}
	if( volume_uuid == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume UUID.",
		 function );

		return( -1 );
	}
	if( volume_uuid_size != 16 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid volume UUID size value out of bounds.",
		 function );

		return( -1 );
	}
	/* Format UUID for key description */
	if( keyring_handle_format_uuid_string(
	     volume_uuid,
	     volume_uuid_size,
	     uuid_string,
	     37,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to format UUID string.",
		 function );

		return( -1 );
	}
	/* Create key description: fvde:<uuid> */
	if( narrow_string_snprintf(
	     key_description,
	     64,
	     "fvde:%s",
	     uuid_string ) < 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to format key description.",
		 function );

		return( -1 );
	}
	/* Combine volume master key (16 bytes) and tweak key (32 bytes)
	 * into a single 48-byte key for dm-crypt AES-XTS
	 */
	if( memory_copy(
	     combined_key,
	     volume_master_key,
	     16 ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy volume master key.",
		 function );

		return( -1 );
	}
	if( memory_copy(
	     &( combined_key[ 16 ] ),
	     volume_tweak_key,
	     32 ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy volume tweak key.",
		 function );

		return( -1 );
	}
	combined_key_size = 48;

	/* Parse keyring ID */
	if( keyring_id == NULL )
	{
		/* Default to session keyring */
		keyring_id_value = KEY_SPEC_SESSION_KEYRING;
	}
	else if( narrow_string_compare(
	          keyring_id,
	          "@s",
	          3 ) == 0 )
	{
		keyring_id_value = KEY_SPEC_SESSION_KEYRING;
	}
	else if( narrow_string_compare(
	          keyring_id,
	          "@u",
	          3 ) == 0 )
	{
		keyring_id_value = KEY_SPEC_USER_KEYRING;
	}
	else if( narrow_string_compare(
	          keyring_id,
	          "@us",
	          4 ) == 0 )
	{
		keyring_id_value = KEY_SPEC_USER_SESSION_KEYRING;
	}
	else
	{
		/* Try to parse as numeric key ID */
		keyring_id_value = atoi( keyring_id );

		if( keyring_id_value == 0 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
			 "%s: unsupported keyring ID.",
			 function );

			return( -1 );
		}
	}
	if( verbose != 0 )
	{
		fprintf(
		 stderr,
		 "Storing key in kernel keyring:\n" );
		fprintf(
		 stderr,
		 "  Key descriptor: %s\n",
		 key_description );
		fprintf(
		 stderr,
		 "  Key size: %zd bytes\n",
		 combined_key_size );
	}
	/* Add key to kernel keyring using "logon" type */
	key_id = add_key(
	          "logon",
	          key_description,
	          combined_key,
	          combined_key_size,
	          keyring_id_value );

	if( key_id < 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to add key to kernel keyring (errno: %d).",
		 function,
		 errno );

		return( -1 );
	}
	if( verbose != 0 )
	{
		fprintf(
		 stderr,
		 "  Key ID: %d\n",
		 key_id );
	}
	/* Zero out the combined key from memory */
	if( memory_set(
	     combined_key,
	     0,
	     48 ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear combined key.",
		 function );

		return( -1 );
	}
	return( 1 );

#else
	static char *function = "keyring_handle_add_key";

	FVDETOOLS_UNREFERENCED_PARAMETER( volume_master_key )
	FVDETOOLS_UNREFERENCED_PARAMETER( volume_master_key_size )
	FVDETOOLS_UNREFERENCED_PARAMETER( volume_tweak_key )
	FVDETOOLS_UNREFERENCED_PARAMETER( volume_tweak_key_size )
	FVDETOOLS_UNREFERENCED_PARAMETER( volume_uuid )
	FVDETOOLS_UNREFERENCED_PARAMETER( volume_uuid_size )
	FVDETOOLS_UNREFERENCED_PARAMETER( keyring_id )
	FVDETOOLS_UNREFERENCED_PARAMETER( verbose )

	libcerror_error_set(
	 error,
	 LIBCERROR_ERROR_DOMAIN_RUNTIME,
	 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
	 "%s: keyutils support not available.",
	 function );

	return( -1 );
#endif /* defined( HAVE_KEYUTILS_H ) */
}
