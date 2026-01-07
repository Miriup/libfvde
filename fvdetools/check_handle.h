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

#if !defined( _CHECK_HANDLE_H )
#define _CHECK_HANDLE_H

#include <common.h>
#include <file_stream.h>
#include <types.h>

#include "fvdecheck_extent.h"
#include "fvdetools_libbfio.h"
#include "fvdetools_libcerror.h"
#include "fvdetools_libfvde.h"

#if defined( __cplusplus )
extern "C" {
#endif

/* Processing order options */
enum check_handle_order
{
	CHECK_HANDLE_ORDER_ASCENDING  = 0,
	CHECK_HANDLE_ORDER_DESCENDING = 1,
	CHECK_HANDLE_ORDER_PHYSICAL   = 2
};

typedef struct check_handle check_handle_t;

struct check_handle
{
	/* The encrypted root plist path
	 */
	const system_character_t *encrypted_root_plist_path;

	/* The key data
	 */
	uint8_t key_data[ 16 ];

	/* The key data size
	 */
	size_t key_data_size;

	/* The volume offset
	 */
	off64_t volume_offset;

	/* The recovery password
	 */
	const system_character_t *recovery_password;

	/* The recovery password length
	 */
	size_t recovery_password_length;

	/* The user password
	 */
	const system_character_t *user_password;

	/* The user password length
	 */
	size_t user_password_length;

	/* The libbfio physical volume file IO pool
	 */
	libbfio_pool_t *physical_volume_file_io_pool;

	/* The libfvde volume
	 */
	libfvde_volume_t *volume;

	/* The libfvde volume group
	 */
	libfvde_volume_group_t *volume_group;

	/* The volume state (extent tracking)
	 */
	fvdecheck_volume_state_t *volume_state;

	/* Processing order
	 */
	int processing_order;

	/* Stop at block index (0 = no stop)
	 */
	uint32_t stop_at_block;

	/* Stop at transaction ID (0 = no stop)
	 */
	uint64_t stop_at_transaction;

	/* Output mode flags */
	int verbose_mode;
	int quiet_mode;
	int json_mode;
	int dump_allocation_map;

	/* Block lookup options (0 = not set, non-zero = lookup requested) */
	int lookup_linux_sector_set;
	uint64_t lookup_linux_sector;

	int lookup_physical_set;
	uint32_t lookup_physical_pv;
	uint64_t lookup_physical_block;

	int lookup_logical_set;
	uint32_t lookup_logical_lv;
	uint64_t lookup_logical_block;

	/* The notification output stream
	 */
	FILE *notify_stream;

	/* Value to indicate if user interaction is disabled
	 */
	int unattended_mode;

	/* Value to indicate if abort was signalled
	 */
	int abort;

	/* Statistics for processing */
	uint32_t transactions_processed;
	uint32_t metadata_blocks_processed;
};

/* Copies a string of a decimal value to a 64-bit value */
int fvdetools_system_string_copy_from_64_bit_in_decimal(
     const system_character_t *string,
     size_t string_size,
     uint64_t *value_64bit,
     libcerror_error_t **error );

/* Initialize check handle */
int check_handle_initialize(
     check_handle_t **check_handle,
     int unattended_mode,
     libcerror_error_t **error );

/* Free check handle */
int check_handle_free(
     check_handle_t **check_handle,
     libcerror_error_t **error );

/* Signal abort */
int check_handle_signal_abort(
     check_handle_t *check_handle,
     libcerror_error_t **error );

/* Set encrypted root plist path */
int check_handle_set_encrypted_root_plist(
     check_handle_t *check_handle,
     const system_character_t *filename,
     libcerror_error_t **error );

/* Set key */
int check_handle_set_key(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Set password */
int check_handle_set_password(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Set recovery password */
int check_handle_set_recovery_password(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Set volume offset */
int check_handle_set_volume_offset(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Set processing order */
int check_handle_set_order(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Set stop at block */
int check_handle_set_stop_at_block(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Set stop at transaction */
int check_handle_set_stop_at_transaction(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Set lookup linux sector */
int check_handle_set_lookup_linux_sector(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Set lookup physical block */
int check_handle_set_lookup_physical(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Set lookup logical block */
int check_handle_set_lookup_logical(
     check_handle_t *check_handle,
     const system_character_t *string,
     libcerror_error_t **error );

/* Open the check handle */
int check_handle_open(
     check_handle_t *check_handle,
     system_character_t * const * filenames,
     int number_of_filenames,
     libcerror_error_t **error );

/* Close the check handle */
int check_handle_close(
     check_handle_t *check_handle,
     libcerror_error_t **error );

/* Process volume and build extent state */
int check_handle_process_volume(
     check_handle_t *check_handle,
     libcerror_error_t **error );

/* Perform block lookup */
int check_handle_lookup_block(
     check_handle_t *check_handle,
     libcerror_error_t **error );

/* Print allocation summary */
int check_handle_print_allocation_summary(
     check_handle_t *check_handle,
     libcerror_error_t **error );

/* Print allocation map */
int check_handle_print_allocation_map(
     check_handle_t *check_handle,
     libcerror_error_t **error );

/* Print results in JSON format */
int check_handle_print_json(
     check_handle_t *check_handle,
     libcerror_error_t **error );

/* Print UUID value */
int check_handle_uuid_value_fprint(
     check_handle_t *check_handle,
     const char *value_name,
     const uint8_t *uuid_data,
     libcerror_error_t **error );

#if defined( __cplusplus )
}
#endif

#endif /* !defined( _CHECK_HANDLE_H ) */
