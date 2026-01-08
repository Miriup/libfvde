/*
 * Device mapper setup handle
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
#include <file_stream.h>
#include <memory.h>
#include <narrow_string.h>
#include <system_string.h>
#include <types.h>

#include <stdio.h>

#include "dmsetup_handle.h"
#include "fvdetools_libcerror.h"
#include "keyring_handle.h"

/* Prints a device-mapper table entry to stdout
 * Returns 1 if successful or -1 on error
 */
int dmsetup_handle_print_table_entry(
     FILE *stream,
     const uint8_t *volume_uuid,
     size_t volume_uuid_size,
     uint64_t volume_size_in_bytes,
     const system_character_t *source_path,
     off64_t volume_offset_in_bytes,
     int shell_mode,
     const char *mapper_name,
     int volume_index,
     libcerror_error_t **error )
{
	char uuid_string[ 37 ];
	static char *function      = "dmsetup_handle_print_table_entry";
	uint64_t size_in_sectors   = 0;
	uint64_t offset_in_sectors = 0;

	if( stream == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid stream.",
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
	if( source_path == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid source path.",
		 function );

		return( -1 );
	}
	/* Format UUID string */
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
	/* Convert bytes to 512-byte sectors for dmsetup */
	size_in_sectors = volume_size_in_bytes / 512;
	offset_in_sectors = volume_offset_in_bytes / 512;

	if( shell_mode != 0 )
	{
		/* Output shell command format */
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
		fprintf(
		 stream,
		 "echo \"0 %" PRIu64 " crypt aes-xts-plain64 :48:logon:fvde:%s 0 %ls %" PRIu64 "\" | dmsetup create %s%d\n",
		 size_in_sectors,
		 uuid_string,
		 source_path,
		 offset_in_sectors,
		 mapper_name,
		 volume_index );
#else
		fprintf(
		 stream,
		 "echo \"0 %" PRIu64 " crypt aes-xts-plain64 :48:logon:fvde:%s 0 %s %" PRIu64 "\" | dmsetup create %s%d\n",
		 size_in_sectors,
		 uuid_string,
		 source_path,
		 offset_in_sectors,
		 mapper_name,
		 volume_index );
#endif
	}
	else
	{
		/* Output raw dmsetup table format */
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
		fprintf(
		 stream,
		 "0 %" PRIu64 " crypt aes-xts-plain64 :48:logon:fvde:%s 0 %ls %" PRIu64 "\n",
		 size_in_sectors,
		 uuid_string,
		 source_path,
		 offset_in_sectors );
#else
		fprintf(
		 stream,
		 "0 %" PRIu64 " crypt aes-xts-plain64 :48:logon:fvde:%s 0 %s %" PRIu64 "\n",
		 size_in_sectors,
		 uuid_string,
		 source_path,
		 offset_in_sectors );
#endif
	}
	return( 1 );
}
