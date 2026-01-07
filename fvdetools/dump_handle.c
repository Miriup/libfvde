/*
 * Dump handle for FVDE metadata extraction
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

#include <stdio.h>

#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif

#if defined( HAVE_FCNTL_H ) || defined( WINAPI )
#include <fcntl.h>
#endif

#if defined( HAVE_SYS_STAT_H )
#include <sys/stat.h>
#endif

#include <errno.h>

#include "dump_handle.h"
#include "fvdetools_libcerror.h"
#include "fvdetools_libcnotify.h"

/* Volume header size is 512 bytes */
#define FVDE_VOLUME_HEADER_SIZE 512

/* CRC32 table and calculation for volume header checksum */
static uint32_t crc32_table[ 256 ];
static int crc32_table_initialized = 0;

/* Initializes the CRC-32 table for volume header checksum calculation
 */
void dump_handle_initialize_crc32_table(
     uint32_t polynomial )
{
	uint32_t checksum    = 0;
	uint32_t table_index = 0;
	uint8_t bit_iterator = 0;

	for( table_index = 0;
	     table_index < 256;
	     table_index++ )
	{
		checksum = (uint32_t) table_index;

		for( bit_iterator = 0;
		     bit_iterator < 8;
		     bit_iterator++ )
		{
			if( checksum & 1 )
			{
				checksum = polynomial ^ ( checksum >> 1 );
			}
			else
			{
				checksum = checksum >> 1;
			}
		}
		crc32_table[ table_index ] = checksum;
	}
	crc32_table_initialized = 1;
}

/* Calculates weak CRC-32 checksum used for volume header
 */
uint32_t dump_handle_calculate_weak_crc32(
     const uint8_t *buffer,
     size_t size,
     uint32_t initial_value )
{
	size_t buffer_offset  = 0;
	uint32_t checksum     = initial_value;
	uint32_t table_index  = 0;

	if( crc32_table_initialized == 0 )
	{
		dump_handle_initialize_crc32_table( 0x82f63b78UL );
	}
	for( buffer_offset = 0;
	     buffer_offset < size;
	     buffer_offset++ )
	{
		table_index = ( checksum ^ buffer[ buffer_offset ] ) & 0x000000ffUL;
		checksum = crc32_table[ table_index ] ^ ( checksum >> 8 );
	}
	return( checksum );
}

/* Creates a dump handle
 * Make sure the value dump_handle is referencing, is set to NULL
 * Returns 1 if successful or -1 on error
 */
int dump_handle_initialize(
     dump_handle_t **dump_handle,
     libcerror_error_t **error )
{
	static char *function = "dump_handle_initialize";

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	if( *dump_handle != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid dump handle value already set.",
		 function );

		return( -1 );
	}
	*dump_handle = memory_allocate_structure(
	                dump_handle_t );

	if( *dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create dump handle.",
		 function );

		goto on_error;
	}
	if( memory_set(
	     *dump_handle,
	     0,
	     sizeof( dump_handle_t ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear dump handle.",
		 function );

		goto on_error;
	}
	( *dump_handle )->source_fd      = -1;
	( *dump_handle )->destination_fd = -1;

	return( 1 );

on_error:
	if( *dump_handle != NULL )
	{
		memory_free(
		 *dump_handle );

		*dump_handle = NULL;
	}
	return( -1 );
}

/* Frees a dump handle
 * Returns 1 if successful or -1 on error
 */
int dump_handle_free(
     dump_handle_t **dump_handle,
     libcerror_error_t **error )
{
	static char *function = "dump_handle_free";
	int result            = 1;

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	if( *dump_handle != NULL )
	{
		if( ( *dump_handle )->source_fd != -1 )
		{
			if( close( ( *dump_handle )->source_fd ) != 0 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_CLOSE_FAILED,
				 "%s: unable to close source file.",
				 function );

				result = -1;
			}
		}
		if( ( *dump_handle )->destination_fd != -1 )
		{
			if( close( ( *dump_handle )->destination_fd ) != 0 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_CLOSE_FAILED,
				 "%s: unable to close destination file.",
				 function );

				result = -1;
			}
		}
		memory_free(
		 *dump_handle );

		*dump_handle = NULL;
	}
	return( result );
}

/* Signals the dump handle to abort
 * Returns 1 if successful or -1 on error
 */
int dump_handle_signal_abort(
     dump_handle_t *dump_handle,
     libcerror_error_t **error )
{
	static char *function = "dump_handle_signal_abort";

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	dump_handle->abort = 1;

	return( 1 );
}

/* Opens the source file
 * Returns 1 if successful or -1 on error
 */
int dump_handle_open_source(
     dump_handle_t *dump_handle,
     const system_character_t *filename,
     libcerror_error_t **error )
{
	static char *function = "dump_handle_open_source";

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	if( filename == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid filename.",
		 function );

		return( -1 );
	}
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
	dump_handle->source_fd = _wopen(
	                          filename,
	                          O_RDONLY );
#else
	dump_handle->source_fd = open(
	                          filename,
	                          O_RDONLY );
#endif

	if( dump_handle->source_fd == -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_OPEN_FAILED,
		 "%s: unable to open source file.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Opens the destination file
 * Returns 1 if successful or -1 on error
 */
int dump_handle_open_destination(
     dump_handle_t *dump_handle,
     const system_character_t *filename,
     libcerror_error_t **error )
{
	struct stat file_stat;

	static char *function = "dump_handle_open_destination";
	int open_flags        = 0;

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	if( filename == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid filename.",
		 function );

		return( -1 );
	}
	/* Check if file exists */
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
	if( _wstat( filename, &file_stat ) == 0 )
#else
	if( stat( filename, &file_stat ) == 0 )
#endif
	{
		if( dump_handle->force == 0 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_GENERIC,
			 "%s: destination file already exists. Use --force to overwrite.",
			 function );

			return( -1 );
		}
	}
	open_flags = O_WRONLY | O_CREAT | O_TRUNC;

#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
	dump_handle->destination_fd = _wopen(
	                               filename,
	                               open_flags,
	                               0644 );
#else
	dump_handle->destination_fd = open(
	                               filename,
	                               open_flags,
	                               0644 );
#endif

	if( dump_handle->destination_fd == -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_OPEN_FAILED,
		 "%s: unable to open destination file.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Closes the dump handle
 * Returns 0 if successful or -1 on error
 */
int dump_handle_close(
     dump_handle_t *dump_handle,
     libcerror_error_t **error )
{
	static char *function = "dump_handle_close";
	int result            = 0;

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	if( dump_handle->source_fd != -1 )
	{
		if( close( dump_handle->source_fd ) != 0 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_CLOSE_FAILED,
			 "%s: unable to close source file.",
			 function );

			result = -1;
		}
		dump_handle->source_fd = -1;
	}
	if( dump_handle->destination_fd != -1 )
	{
		if( close( dump_handle->destination_fd ) != 0 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_CLOSE_FAILED,
			 "%s: unable to close destination file.",
			 function );

			result = -1;
		}
		dump_handle->destination_fd = -1;
	}
	return( result );
}

/* Reads and parses the volume header
 * Returns 1 if successful or -1 on error
 */
int dump_handle_read_volume_header(
     dump_handle_t *dump_handle,
     libcerror_error_t **error )
{
	uint8_t volume_header_data[ FVDE_VOLUME_HEADER_SIZE ];

	static char *function     = "dump_handle_read_volume_header";
	ssize_t read_count        = 0;
	int metadata_block_index  = 0;

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	if( dump_handle->source_fd == -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid dump handle - source file not open.",
		 function );

		return( -1 );
	}
	/* Seek to beginning */
	if( lseek( dump_handle->source_fd, 0, SEEK_SET ) == (off_t) -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek to volume header.",
		 function );

		return( -1 );
	}
	read_count = read(
	              dump_handle->source_fd,
	              volume_header_data,
	              FVDE_VOLUME_HEADER_SIZE );

	if( read_count != FVDE_VOLUME_HEADER_SIZE )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read volume header.",
		 function );

		return( -1 );
	}
	/* Check CoreStorage signature "CS" at offset 88 */
	if( ( volume_header_data[ 88 ] != 'C' )
	 || ( volume_header_data[ 89 ] != 'S' ) )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported core storage signature.",
		 function );

		return( -1 );
	}
	/* Extract physical volume size at offset 72 */
	byte_stream_copy_to_uint64_little_endian(
	 &( volume_header_data[ 72 ] ),
	 dump_handle->physical_volume_size );

	/* Extract block size at offset 96 */
	byte_stream_copy_to_uint32_little_endian(
	 &( volume_header_data[ 96 ] ),
	 dump_handle->block_size );

	/* Extract metadata size at offset 100 */
	byte_stream_copy_to_uint32_little_endian(
	 &( volume_header_data[ 100 ] ),
	 dump_handle->metadata_size );

	/* Extract metadata block numbers at offset 104 (4 x 8 bytes) */
	for( metadata_block_index = 0;
	     metadata_block_index < 4;
	     metadata_block_index++ )
	{
		byte_stream_copy_to_uint64_little_endian(
		 &( volume_header_data[ 104 + ( metadata_block_index * 8 ) ] ),
		 dump_handle->metadata_offsets[ metadata_block_index ] );

		/* Convert block number to byte offset */
		dump_handle->metadata_offsets[ metadata_block_index ] *= dump_handle->block_size;
	}
	if( dump_handle->verbose != 0 )
	{
		fprintf(
		 stdout,
		 "Volume header:\n" );
		fprintf(
		 stdout,
		 "\tPhysical volume size: %" PRIu64 " bytes\n",
		 dump_handle->physical_volume_size );
		fprintf(
		 stdout,
		 "\tBlock size: %" PRIu32 " bytes\n",
		 dump_handle->block_size );
		fprintf(
		 stdout,
		 "\tMetadata size: %" PRIu32 " bytes\n",
		 dump_handle->metadata_size );

		for( metadata_block_index = 0;
		     metadata_block_index < 4;
		     metadata_block_index++ )
		{
			fprintf(
			 stdout,
			 "\tMetadata %d offset: 0x%08" PRIx64 "\n",
			 metadata_block_index + 1,
			 dump_handle->metadata_offsets[ metadata_block_index ] );
		}
		fprintf(
		 stdout,
		 "\n" );
	}
	return( 1 );
}

/* Reads metadata to find encrypted metadata offsets
 * Returns 1 if successful or -1 on error
 */
int dump_handle_read_metadata(
     dump_handle_t *dump_handle,
     libcerror_error_t **error )
{
	uint8_t *metadata_data                     = NULL;
	static char *function                      = "dump_handle_read_metadata";
	uint64_t encrypted_metadata_size           = 0;
	uint64_t encrypted_metadata1_offset        = 0;
	uint64_t encrypted_metadata2_offset        = 0;
	uint64_t highest_transaction_id            = 0;
	uint64_t transaction_identifier            = 0;
	uint32_t volume_groups_descriptor_offset   = 0;
	ssize_t read_count                         = 0;
	int best_metadata_index                    = 0;
	int metadata_index                         = 0;

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	if( dump_handle->metadata_size == 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid dump handle - metadata size not set.",
		 function );

		return( -1 );
	}
	metadata_data = (uint8_t *) memory_allocate(
	                             dump_handle->metadata_size );

	if( metadata_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create metadata buffer.",
		 function );

		goto on_error;
	}
	/* Find metadata with highest transaction ID and extract encrypted metadata offsets */
	for( metadata_index = 0;
	     metadata_index < 4;
	     metadata_index++ )
	{
		if( lseek( dump_handle->source_fd, (off_t) dump_handle->metadata_offsets[ metadata_index ], SEEK_SET ) == (off_t) -1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_SEEK_FAILED,
			 "%s: unable to seek to metadata %d.",
			 function,
			 metadata_index + 1 );

			goto on_error;
		}
		read_count = read(
		              dump_handle->source_fd,
		              metadata_data,
		              dump_handle->metadata_size );

		if( read_count != (ssize_t) dump_handle->metadata_size )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read metadata %d.",
			 function,
			 metadata_index + 1 );

			goto on_error;
		}
		/* Check for metadata block header - skip first 64 bytes (block header)
		 * and read transaction identifier at offset 16 within the header */
		byte_stream_copy_to_uint64_little_endian(
		 &( metadata_data[ 16 ] ),
		 transaction_identifier );

		if( dump_handle->verbose != 0 )
		{
			fprintf(
			 stdout,
			 "Metadata %d: transaction ID = %" PRIu64 "\n",
			 metadata_index + 1,
			 transaction_identifier );
		}
		if( transaction_identifier > highest_transaction_id )
		{
			highest_transaction_id = transaction_identifier;
			best_metadata_index = metadata_index;

			/* Read volume groups descriptor offset at 64 + 156 = 220 */
			byte_stream_copy_to_uint32_little_endian(
			 &( metadata_data[ 64 + 156 ] ),
			 volume_groups_descriptor_offset );

			/* The offset is relative to start of metadata block (includes 64 byte header) */
			/* Read encrypted metadata info from volume groups descriptor
			 * The descriptor is at volume_groups_descriptor_offset from start of block data
			 * which is at metadata_data[ volume_groups_descriptor_offset ] since block starts at 0 */
			if( volume_groups_descriptor_offset > 64 )
			{
				uint32_t actual_offset = volume_groups_descriptor_offset - 64;

				/* Encrypted metadata size (in blocks) at offset +8 */
				byte_stream_copy_to_uint64_little_endian(
				 &( metadata_data[ 64 + actual_offset + 8 ] ),
				 encrypted_metadata_size );

				/* Encrypted metadata 1 block number at offset +32 */
				byte_stream_copy_to_uint64_little_endian(
				 &( metadata_data[ 64 + actual_offset + 32 ] ),
				 encrypted_metadata1_offset );

				/* Encrypted metadata 2 block number at offset +40 */
				byte_stream_copy_to_uint64_little_endian(
				 &( metadata_data[ 64 + actual_offset + 40 ] ),
				 encrypted_metadata2_offset );
			}
		}
	}
	/* Convert block numbers to byte offsets and store in handle */
	/* Clear upper 16 bits which contain volume index */
	dump_handle->encrypted_metadata1_offset = ( encrypted_metadata1_offset & 0x0000ffffffffffffUL ) * dump_handle->block_size;
	dump_handle->encrypted_metadata2_offset = ( encrypted_metadata2_offset & 0x0000ffffffffffffUL ) * dump_handle->block_size;
	dump_handle->encrypted_metadata_size = encrypted_metadata_size * dump_handle->block_size;

	if( dump_handle->verbose != 0 )
	{
		fprintf(
		 stdout,
		 "\nBest metadata: %d (transaction ID: %" PRIu64 ")\n",
		 best_metadata_index + 1,
		 highest_transaction_id );
		fprintf(
		 stdout,
		 "Encrypted metadata 1 offset: 0x%08" PRIx64 "\n",
		 dump_handle->encrypted_metadata1_offset );
		fprintf(
		 stdout,
		 "Encrypted metadata 2 offset: 0x%08" PRIx64 "\n",
		 dump_handle->encrypted_metadata2_offset );
		fprintf(
		 stdout,
		 "Encrypted metadata size: %" PRIu64 " bytes\n",
		 dump_handle->encrypted_metadata_size );
		fprintf(
		 stdout,
		 "\n" );
	}
	memory_free(
	 metadata_data );

	return( 1 );

on_error:
	if( metadata_data != NULL )
	{
		memory_free(
		 metadata_data );
	}
	return( -1 );
}

/* Copies a region from source to destination
 * Returns 1 if successful or -1 on error
 */
int dump_handle_copy_region(
     dump_handle_t *dump_handle,
     off64_t source_offset,
     off64_t destination_offset,
     size64_t size,
     const char *region_name,
     libcerror_error_t **error )
{
	uint8_t *buffer       = NULL;
	static char *function = "dump_handle_copy_region";
	size_t buffer_size    = 0;
	size_t bytes_to_copy  = 0;
	ssize_t read_count    = 0;
	ssize_t write_count   = 0;
	size64_t remaining    = 0;

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	if( size == 0 )
	{
		return( 1 );
	}
	/* Use 64KB buffer for copying */
	buffer_size = 64 * 1024;

	buffer = (uint8_t *) memory_allocate( buffer_size );

	if( buffer == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create copy buffer.",
		 function );

		goto on_error;
	}
	if( dump_handle->verbose != 0 )
	{
		fprintf(
		 stdout,
		 "Copying %s: source offset 0x%08" PRIx64 ", size %" PRIu64 " bytes",
		 region_name,
		 source_offset,
		 size );

		if( source_offset != destination_offset )
		{
			fprintf(
			 stdout,
			 ", dest offset 0x%08" PRIx64,
			 destination_offset );
		}
		fprintf(
		 stdout,
		 "\n" );
	}
	/* Seek source */
	if( lseek( dump_handle->source_fd, source_offset, SEEK_SET ) == (off_t) -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek source to offset: 0x%08" PRIx64 ".",
		 function,
		 source_offset );

		goto on_error;
	}
	/* Seek destination */
	if( lseek( dump_handle->destination_fd, destination_offset, SEEK_SET ) == (off_t) -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek destination to offset: 0x%08" PRIx64 ".",
		 function,
		 destination_offset );

		goto on_error;
	}
	remaining = size;

	while( remaining > 0 )
	{
		if( dump_handle->abort != 0 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_ABORT_REQUESTED,
			 "%s: abort requested.",
			 function );

			goto on_error;
		}
		bytes_to_copy = ( remaining > buffer_size ) ? buffer_size : (size_t) remaining;

		read_count = read(
		              dump_handle->source_fd,
		              buffer,
		              bytes_to_copy );

		if( read_count != (ssize_t) bytes_to_copy )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read from source.",
			 function );

			goto on_error;
		}
		write_count = write(
		               dump_handle->destination_fd,
		               buffer,
		               bytes_to_copy );

		if( write_count != (ssize_t) bytes_to_copy )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_WRITE_FAILED,
			 "%s: unable to write to destination.",
			 function );

			goto on_error;
		}
		remaining -= bytes_to_copy;
		dump_handle->bytes_copied += bytes_to_copy;
	}
	memory_free( buffer );

	return( 1 );

on_error:
	if( buffer != NULL )
	{
		memory_free( buffer );
	}
	return( -1 );
}

/* Writes metadata block with corrected encrypted metadata offsets for compact dumps
 * Returns 1 if successful or -1 on error
 */
int dump_handle_write_corrected_metadata(
     dump_handle_t *dump_handle,
     off64_t source_offset,
     off64_t destination_offset,
     uint64_t compact_encrypted_metadata1_offset,
     uint64_t compact_encrypted_metadata2_offset,
     const char *region_name,
     libcerror_error_t **error )
{
	uint8_t *metadata_data                   = NULL;
	static char *function                    = "dump_handle_write_corrected_metadata";
	uint32_t volume_groups_descriptor_offset = 0;
	ssize_t read_count                       = 0;
	ssize_t write_count                      = 0;

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	metadata_data = (uint8_t *) memory_allocate(
	                             dump_handle->metadata_size );

	if( metadata_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create metadata buffer.",
		 function );

		return( -1 );
	}
	/* Read metadata from source */
	if( lseek( dump_handle->source_fd, source_offset, SEEK_SET ) == (off_t) -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek to source offset: 0x%08" PRIx64 ".",
		 function,
		 source_offset );

		goto on_error;
	}
	read_count = read(
	              dump_handle->source_fd,
	              metadata_data,
	              dump_handle->metadata_size );

	if( read_count != (ssize_t) dump_handle->metadata_size )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read metadata.",
		 function );

		goto on_error;
	}
	/* Read volume groups descriptor offset at 64 + 156 = 220
	 * Note: This offset is relative to the start of the metadata block (including 64-byte header)
	 * but our metadata_data buffer doesn't include the header, so we need to adjust
	 */
	byte_stream_copy_to_uint32_little_endian(
	 &( metadata_data[ 64 + 156 ] ),
	 volume_groups_descriptor_offset );

	/* Check if we have a valid volume groups descriptor offset
	 * The offset should be > 64 (past the block header) and within bounds
	 */
	if( volume_groups_descriptor_offset > 64 )
	{
		uint32_t actual_offset = volume_groups_descriptor_offset - 64;
		uint64_t encrypted_metadata1_block_number = compact_encrypted_metadata1_offset / dump_handle->block_size;
		uint64_t encrypted_metadata2_block_number = compact_encrypted_metadata2_offset / dump_handle->block_size;

		/* Verify the actual offset + field offsets are within our buffer */
		if( ( 64 + actual_offset + 48 <= dump_handle->metadata_size ) )
		{
			if( dump_handle->verbose != 0 )
			{
				fprintf(
				 stdout,
				 "Correcting %s encrypted metadata offsets:\n",
				 region_name );
				fprintf(
				 stdout,
				 "  Volume groups descriptor at offset: %" PRIu32 " (buffer offset: %" PRIu32 ")\n",
				 volume_groups_descriptor_offset,
				 64 + actual_offset );
				fprintf(
				 stdout,
				 "  Encrypted metadata 1: block %" PRIu64 " (offset 0x%08" PRIx64 ")\n",
				 encrypted_metadata1_block_number,
				 compact_encrypted_metadata1_offset );
				fprintf(
				 stdout,
				 "  Encrypted metadata 2: block %" PRIu64 " (offset 0x%08" PRIx64 ")\n",
				 encrypted_metadata2_block_number,
				 compact_encrypted_metadata2_offset );
			}
			/* Write corrected encrypted metadata 1 block number at offset +32 */
			byte_stream_copy_from_uint64_little_endian(
			 &( metadata_data[ 64 + actual_offset + 32 ] ),
			 encrypted_metadata1_block_number );

			/* Write corrected encrypted metadata 2 block number at offset +40 */
			byte_stream_copy_from_uint64_little_endian(
			 &( metadata_data[ 64 + actual_offset + 40 ] ),
			 encrypted_metadata2_block_number );

			/* Recalculate metadata block checksum after modifications
			 * Metadata block structure:
			 * - Offset 0-3: checksum (4 bytes)
			 * - Offset 4-7: initial value (4 bytes, 0xffffffff)
			 * - Offset 8-8191: data (8184 bytes)
			 */
			uint32_t initial_value = 0xffffffffUL;
			uint32_t calculated_checksum = dump_handle_calculate_weak_crc32(
			                                &( metadata_data[ 8 ] ),
			                                8184,
			                                initial_value );

			/* Write the recalculated checksum */
			byte_stream_copy_from_uint32_little_endian(
			 &( metadata_data[ 0 ] ),
			 calculated_checksum );

			if( dump_handle->verbose != 0 )
			{
				fprintf(
				 stdout,
				 "  Recalculated metadata block checksum: 0x%08" PRIx32 "\n",
				 calculated_checksum );
			}
		}
	}
	/* Write corrected metadata to destination */
	if( lseek( dump_handle->destination_fd, destination_offset, SEEK_SET ) == (off_t) -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek to destination offset: 0x%08" PRIx64 ".",
		 function,
		 destination_offset );

		goto on_error;
	}
	write_count = write(
	               dump_handle->destination_fd,
	               metadata_data,
	               dump_handle->metadata_size );

	if( write_count != (ssize_t) dump_handle->metadata_size )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_WRITE_FAILED,
		 "%s: unable to write metadata.",
		 function );

		goto on_error;
	}
	dump_handle->bytes_copied += dump_handle->metadata_size;

	memory_free( metadata_data );

	return( 1 );

on_error:
	if( metadata_data != NULL )
	{
		memory_free( metadata_data );
	}
	return( -1 );
}

/* Writes volume header with corrected metadata offsets for compact dumps
 * Returns 1 if successful or -1 on error
 */
int dump_handle_write_corrected_volume_header(
     dump_handle_t *dump_handle,
     libcerror_error_t **error )
{
	uint8_t volume_header_data[ FVDE_VOLUME_HEADER_SIZE ];
	static char *function     = "dump_handle_write_corrected_volume_header";
	uint64_t compact_block    = 1;  /* Start at block 1 (block 0 contains volume header) */
	ssize_t read_count        = 0;
	ssize_t write_count       = 0;
	int metadata_index        = 0;

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	/* Read volume header from source */
	if( lseek( dump_handle->source_fd, 0, SEEK_SET ) == (off_t) -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek to volume header.",
		 function );

		return( -1 );
	}
	read_count = read(
	              dump_handle->source_fd,
	              volume_header_data,
	              FVDE_VOLUME_HEADER_SIZE );

	if( read_count != FVDE_VOLUME_HEADER_SIZE )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read volume header.",
		 function );

		return( -1 );
	}
	/* Update metadata block offsets for compact layout
	 * Metadata blocks must be aligned to block boundaries (4096-byte blocks)
	 * Block 0 contains the volume header, so start metadata at block 1
	 */
	for( metadata_index = 0;
	     metadata_index < 4;
	     metadata_index++ )
	{
		uint64_t metadata_block_number = compact_block;

		/* Write block number at offset 104 + (metadata_index * 8) */
		byte_stream_copy_from_uint64_little_endian(
		 &( volume_header_data[ 104 + ( metadata_index * 8 ) ] ),
		 metadata_block_number );

		if( dump_handle->verbose != 0 )
		{
			fprintf(
			 stdout,
			 "Correcting metadata %d block number: %" PRIu64 " (offset 0x%08" PRIx64 ")\n",
			 metadata_index + 1,
			 metadata_block_number,
			 metadata_block_number * dump_handle->block_size );
		}
		/* Calculate how many blocks this metadata occupies */
		compact_block += ( dump_handle->metadata_size + dump_handle->block_size - 1 ) / dump_handle->block_size;
	}
	/* Recalculate volume header checksum after modifications */
	{
		uint32_t initial_value      = 0;
		uint32_t calculated_checksum = 0;

		/* Read initial value from offset 4 */
		byte_stream_copy_to_uint32_little_endian(
		 &( volume_header_data[ 4 ] ),
		 initial_value );

		/* Calculate checksum on bytes 8-511 (504 bytes) */
		calculated_checksum = dump_handle_calculate_weak_crc32(
		                       &( volume_header_data[ 8 ] ),
		                       504,
		                       initial_value );

		/* Write corrected checksum at offset 0 */
		byte_stream_copy_from_uint32_little_endian(
		 &( volume_header_data[ 0 ] ),
		 calculated_checksum );

		if( dump_handle->verbose != 0 )
		{
			fprintf(
			 stdout,
			 "Recalculated volume header checksum: 0x%08" PRIx32 "\n",
			 calculated_checksum );
		}
	}
	/* Write corrected volume header to destination */
	if( lseek( dump_handle->destination_fd, 0, SEEK_SET ) == (off_t) -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek to destination.",
		 function );

		return( -1 );
	}
	write_count = write(
	               dump_handle->destination_fd,
	               volume_header_data,
	               FVDE_VOLUME_HEADER_SIZE );

	if( write_count != FVDE_VOLUME_HEADER_SIZE )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_WRITE_FAILED,
		 "%s: unable to write volume header.",
		 function );

		return( -1 );
	}
	dump_handle->bytes_copied += FVDE_VOLUME_HEADER_SIZE;

	return( 1 );
}

/* Performs the dump operation
 * Returns 1 if successful or -1 on error
 */
int dump_handle_dump(
     dump_handle_t *dump_handle,
     libcerror_error_t **error )
{
	static char *function      = "dump_handle_dump";
	off64_t current_offset     = 0;
	int metadata_index         = 0;
	char region_name[ 64 ];

	if( dump_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid dump handle.",
		 function );

		return( -1 );
	}
	dump_handle->bytes_copied = 0;

	if( dump_handle->compact == 0 )
	{
		/* Create sparse file with full size */
		if( ftruncate( dump_handle->destination_fd, (off_t) dump_handle->physical_volume_size ) != 0 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_GENERIC,
			 "%s: unable to set destination file size.",
			 function );

			return( -1 );
		}
		if( dump_handle->verbose != 0 )
		{
			fprintf(
			 stdout,
			 "Created sparse file with size %" PRIu64 " bytes\n\n",
			 dump_handle->physical_volume_size );
		}
	}
	current_offset = 0;

	/* Copy volume header (512 bytes at offset 0) */
	if( dump_handle->compact != 0 )
	{
		/* In compact mode, write corrected volume header with adjusted metadata offsets */
		if( dump_handle_write_corrected_volume_header(
		     dump_handle,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_WRITE_FAILED,
			 "%s: unable to write corrected volume header.",
			 function );

			return( -1 );
		}
		/* In compact mode, metadata blocks must be block-aligned
		 * Start at block 1 (offset 4096) since block 0 contains volume header
		 */
		current_offset = dump_handle->block_size;
	}
	else
	{
		/* In normal mode, just copy the volume header as-is */
		if( dump_handle_copy_region(
		     dump_handle,
		     0,
		     current_offset,
		     FVDE_VOLUME_HEADER_SIZE,
		     "volume header",
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_IO,
			 LIBCERROR_IO_ERROR_WRITE_FAILED,
			 "%s: unable to copy volume header.",
			 function );

			return( -1 );
		}
	}
	/* Copy metadata regions */
	for( metadata_index = 0;
	     metadata_index < 4;
	     metadata_index++ )
	{
		snprintf(
		 region_name,
		 64,
		 "metadata %d",
		 metadata_index + 1 );

		if( dump_handle->compact != 0 )
		{
			/* In compact mode, write corrected metadata with adjusted encrypted metadata offsets
			 * Metadata starts at block 1 (offset = block_size), so encrypted metadata
			 * starts after 4 metadata blocks
			 */
			uint64_t compact_encrypted_metadata1_offset = dump_handle->block_size + ( 4 * dump_handle->metadata_size );
			uint64_t compact_encrypted_metadata2_offset = compact_encrypted_metadata1_offset + dump_handle->encrypted_metadata_size;

			if( dump_handle_write_corrected_metadata(
			     dump_handle,
			     (off64_t) dump_handle->metadata_offsets[ metadata_index ],
			     current_offset,
			     compact_encrypted_metadata1_offset,
			     compact_encrypted_metadata2_offset,
			     region_name,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_WRITE_FAILED,
				 "%s: unable to write corrected %s.",
				 function,
				 region_name );

				return( -1 );
			}
			current_offset += dump_handle->metadata_size;
		}
		else
		{
			if( dump_handle_copy_region(
			     dump_handle,
			     (off64_t) dump_handle->metadata_offsets[ metadata_index ],
			     (off64_t) dump_handle->metadata_offsets[ metadata_index ],
			     dump_handle->metadata_size,
			     region_name,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_WRITE_FAILED,
				 "%s: unable to copy %s.",
				 function,
				 region_name );

				return( -1 );
			}
		}
	}
	/* Copy encrypted metadata 1 */
	if( dump_handle->encrypted_metadata1_offset != 0 )
	{
		if( dump_handle->compact != 0 )
		{
			if( dump_handle_copy_region(
			     dump_handle,
			     (off64_t) dump_handle->encrypted_metadata1_offset,
			     current_offset,
			     dump_handle->encrypted_metadata_size,
			     "encrypted metadata 1",
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_WRITE_FAILED,
				 "%s: unable to copy encrypted metadata 1.",
				 function );

				return( -1 );
			}
			current_offset += dump_handle->encrypted_metadata_size;
		}
		else
		{
			if( dump_handle_copy_region(
			     dump_handle,
			     (off64_t) dump_handle->encrypted_metadata1_offset,
			     (off64_t) dump_handle->encrypted_metadata1_offset,
			     dump_handle->encrypted_metadata_size,
			     "encrypted metadata 1",
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_WRITE_FAILED,
				 "%s: unable to copy encrypted metadata 1.",
				 function );

				return( -1 );
			}
		}
	}
	/* Copy encrypted metadata 2 */
	if( dump_handle->encrypted_metadata2_offset != 0 )
	{
		if( dump_handle->compact != 0 )
		{
			if( dump_handle_copy_region(
			     dump_handle,
			     (off64_t) dump_handle->encrypted_metadata2_offset,
			     current_offset,
			     dump_handle->encrypted_metadata_size,
			     "encrypted metadata 2",
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_WRITE_FAILED,
				 "%s: unable to copy encrypted metadata 2.",
				 function );

				return( -1 );
			}
			current_offset += dump_handle->encrypted_metadata_size;
		}
		else
		{
			if( dump_handle_copy_region(
			     dump_handle,
			     (off64_t) dump_handle->encrypted_metadata2_offset,
			     (off64_t) dump_handle->encrypted_metadata2_offset,
			     dump_handle->encrypted_metadata_size,
			     "encrypted metadata 2",
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_WRITE_FAILED,
				 "%s: unable to copy encrypted metadata 2.",
				 function );

				return( -1 );
			}
		}
	}
	fprintf(
	 stdout,
	 "\nDump complete.\n" );
	fprintf(
	 stdout,
	 "Total bytes copied: %" PRIu64 " bytes\n",
	 dump_handle->bytes_copied );

	if( dump_handle->compact == 0 )
	{
		fprintf(
		 stdout,
		 "Sparse file size: %" PRIu64 " bytes\n",
		 dump_handle->physical_volume_size );
	}
	return( 1 );
}
