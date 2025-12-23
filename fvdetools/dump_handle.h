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

#if !defined( _DUMP_HANDLE_H )
#define _DUMP_HANDLE_H

#include <common.h>
#include <file_stream.h>
#include <system_string.h>
#include <types.h>

#include "fvdetools_libcerror.h"

#if defined( __cplusplus )
extern "C" {
#endif

typedef struct dump_handle dump_handle_t;

struct dump_handle
{
	/* The source file descriptor
	 */
	int source_fd;

	/* The destination file descriptor
	 */
	int destination_fd;

	/* The physical volume size
	 */
	uint64_t physical_volume_size;

	/* The block size
	 */
	uint32_t block_size;

	/* The metadata size
	 */
	uint32_t metadata_size;

	/* The metadata offsets (4 copies)
	 */
	uint64_t metadata_offsets[ 4 ];

	/* The encrypted metadata 1 offset
	 */
	uint64_t encrypted_metadata1_offset;

	/* The encrypted metadata 2 offset
	 */
	uint64_t encrypted_metadata2_offset;

	/* The encrypted metadata size
	 */
	uint64_t encrypted_metadata_size;

	/* Verbose mode flag
	 */
	int verbose;

	/* Force overwrite flag
	 */
	int force;

	/* Best metadata only flag
	 */
	int best_metadata_only;

	/* Compact mode flag (non-sparse with adjusted offsets)
	 */
	int compact;

	/* Number of sample blocks to include
	 */
	int sample_blocks;

	/* Total bytes copied
	 */
	uint64_t bytes_copied;

	/* Value to indicate if abort was signalled
	 */
	int abort;
};

int dump_handle_initialize(
     dump_handle_t **dump_handle,
     libcerror_error_t **error );

int dump_handle_free(
     dump_handle_t **dump_handle,
     libcerror_error_t **error );

int dump_handle_signal_abort(
     dump_handle_t *dump_handle,
     libcerror_error_t **error );

int dump_handle_open_source(
     dump_handle_t *dump_handle,
     const system_character_t *filename,
     libcerror_error_t **error );

int dump_handle_open_destination(
     dump_handle_t *dump_handle,
     const system_character_t *filename,
     libcerror_error_t **error );

int dump_handle_close(
     dump_handle_t *dump_handle,
     libcerror_error_t **error );

int dump_handle_read_volume_header(
     dump_handle_t *dump_handle,
     libcerror_error_t **error );

int dump_handle_read_metadata(
     dump_handle_t *dump_handle,
     libcerror_error_t **error );

int dump_handle_dump(
     dump_handle_t *dump_handle,
     libcerror_error_t **error );

#if defined( __cplusplus )
}
#endif

#endif /* !defined( _DUMP_HANDLE_H ) */
