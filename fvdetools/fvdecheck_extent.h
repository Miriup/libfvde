/*
 * Extent tracking for fvdecheck
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

#if !defined( _FVDECHECK_EXTENT_H )
#define _FVDECHECK_EXTENT_H

#include <common.h>
#include <types.h>

#include "fvdetools_libcerror.h"

#if defined( __cplusplus )
extern "C" {
#endif

/* Maximum supported physical and logical volumes */
#define FVDECHECK_MAX_PHYSICAL_VOLUMES 16
#define FVDECHECK_MAX_LOGICAL_VOLUMES  16

/* Extent state enumeration */
enum fvdecheck_extent_state
{
	FVDECHECK_EXTENT_STATE_UNKNOWN   = 0,
	FVDECHECK_EXTENT_STATE_FREE      = 1,
	FVDECHECK_EXTENT_STATE_ALLOCATED = 2,
	FVDECHECK_EXTENT_STATE_RESERVED  = 3
};

typedef struct fvdecheck_extent fvdecheck_extent_t;

struct fvdecheck_extent
{
	/* Physical volume location */
	uint32_t physical_volume_index;
	uint64_t physical_block_start;
	uint64_t physical_block_count;

	/* Logical volume location (if allocated to logical volume) */
	uint32_t logical_volume_index;
	uint64_t logical_block_start;
	/* logical_block_count == physical_block_count */

	/* Doubly-linked list for physical volume ordering */
	fvdecheck_extent_t *phys_prev;
	fvdecheck_extent_t *phys_next;

	/* Doubly-linked list for logical volume ordering */
	fvdecheck_extent_t *logical_prev;
	fvdecheck_extent_t *logical_next;

	/* Allocation state */
	int state;

	/* Provenance tracking */
	uint64_t transaction_id;
	uint32_t metadata_block_index;
	uint16_t block_type;

	/* Reserved area description (if RESERVED state) */
	const char *reserved_description;
};

typedef struct fvdecheck_physical_volume_info fvdecheck_physical_volume_info_t;

struct fvdecheck_physical_volume_info
{
	/* UUID of the physical volume */
	uint8_t uuid[ 16 ];

	/* Size in blocks */
	uint64_t size_in_blocks;

	/* Head of physical extent list */
	fvdecheck_extent_t *extent_list_head;

	/* Tail of physical extent list */
	fvdecheck_extent_t *extent_list_tail;

	/* Allocation statistics */
	uint64_t reserved_blocks;
	uint64_t allocated_blocks;
	uint64_t free_blocks;
};

typedef struct fvdecheck_logical_volume_info fvdecheck_logical_volume_info_t;

struct fvdecheck_logical_volume_info
{
	/* UUID of the logical volume */
	uint8_t uuid[ 16 ];

	/* Size in blocks */
	uint64_t size_in_blocks;

	/* Head of logical extent list */
	fvdecheck_extent_t *extent_list_head;

	/* Tail of logical extent list */
	fvdecheck_extent_t *extent_list_tail;

	/* Allocation statistics */
	uint64_t mapped_blocks;
	uint64_t unmapped_blocks;
};

typedef struct fvdecheck_volume_state fvdecheck_volume_state_t;

struct fvdecheck_volume_state
{
	/* Physical volumes */
	uint32_t num_physical_volumes;
	fvdecheck_physical_volume_info_t physical_volumes[ FVDECHECK_MAX_PHYSICAL_VOLUMES ];

	/* Logical volumes */
	uint32_t num_logical_volumes;
	fvdecheck_logical_volume_info_t logical_volumes[ FVDECHECK_MAX_LOGICAL_VOLUMES ];

	/* Block size (typically 4096) */
	uint32_t block_size;

	/* Processing state */
	uint64_t current_transaction_id;
	uint32_t current_metadata_block_index;

	/* Error tracking */
	uint32_t error_count;
	uint32_t warning_count;

	/* Total extents allocated */
	uint64_t total_extents;
};

typedef struct fvdecheck_error_info fvdecheck_error_info_t;

struct fvdecheck_error_info
{
	/* Error type (overlap, double-alloc, etc.) */
	int error_type;

	/* Physical volume and block range involved */
	uint32_t pv_index;
	uint64_t block_start;
	uint64_t block_count;

	/* First allocation info */
	uint64_t first_transaction_id;
	uint16_t first_block_type;
	uint32_t first_metadata_block_index;

	/* Second allocation info (for overlaps) */
	uint64_t second_transaction_id;
	uint16_t second_block_type;
	uint32_t second_metadata_block_index;

	/* Description */
	char description[ 256 ];
};

/* Error types */
#define FVDECHECK_ERROR_PHYSICAL_OVERLAP      1
#define FVDECHECK_ERROR_LOGICAL_OVERLAP       2
#define FVDECHECK_ERROR_ALLOCATE_AFTER_ALLOC  3
#define FVDECHECK_ERROR_RESERVED_VIOLATION    4
#define FVDECHECK_ERROR_FREE_AFTER_FREE       5

/* Initialize volume state structure */
int fvdecheck_volume_state_initialize(
     fvdecheck_volume_state_t **volume_state,
     libcerror_error_t **error );

/* Free volume state structure */
int fvdecheck_volume_state_free(
     fvdecheck_volume_state_t **volume_state,
     libcerror_error_t **error );

/* Initialize an extent */
int fvdecheck_extent_initialize(
     fvdecheck_extent_t **extent,
     libcerror_error_t **error );

/* Free an extent */
int fvdecheck_extent_free(
     fvdecheck_extent_t **extent,
     libcerror_error_t **error );

/* Add a physical volume to the state */
int fvdecheck_volume_state_add_physical_volume(
     fvdecheck_volume_state_t *volume_state,
     const uint8_t *uuid,
     uint64_t size_in_blocks,
     uint32_t *pv_index,
     libcerror_error_t **error );

/* Add a logical volume to the state */
int fvdecheck_volume_state_add_logical_volume(
     fvdecheck_volume_state_t *volume_state,
     const uint8_t *uuid,
     uint64_t size_in_blocks,
     uint32_t *lv_index,
     libcerror_error_t **error );

/* Mark a physical extent as reserved */
int fvdecheck_volume_state_mark_reserved(
     fvdecheck_volume_state_t *volume_state,
     uint32_t pv_index,
     uint64_t block_start,
     uint64_t block_count,
     const char *description,
     libcerror_error_t **error );

/* Mark a physical extent as free */
int fvdecheck_volume_state_mark_free(
     fvdecheck_volume_state_t *volume_state,
     uint32_t pv_index,
     uint64_t block_start,
     uint64_t block_count,
     uint64_t transaction_id,
     uint32_t metadata_block_index,
     uint16_t block_type,
     libcerror_error_t **error );

/* Mark an extent as allocated (maps physical to logical) */
int fvdecheck_volume_state_mark_allocated(
     fvdecheck_volume_state_t *volume_state,
     uint32_t pv_index,
     uint64_t phys_block_start,
     uint64_t block_count,
     uint32_t lv_index,
     uint64_t logical_block_start,
     uint64_t transaction_id,
     uint32_t metadata_block_index,
     uint16_t block_type,
     libcerror_error_t **error );

/* Find extent containing a physical block */
fvdecheck_extent_t *fvdecheck_volume_state_find_physical_extent(
     fvdecheck_volume_state_t *volume_state,
     uint32_t pv_index,
     uint64_t block_number );

/* Find extent containing a logical block */
fvdecheck_extent_t *fvdecheck_volume_state_find_logical_extent(
     fvdecheck_volume_state_t *volume_state,
     uint32_t lv_index,
     uint64_t block_number );

/* Check for overlap with existing extents in physical space */
fvdecheck_extent_t *fvdecheck_volume_state_check_overlap(
     fvdecheck_volume_state_t *volume_state,
     uint32_t pv_index,
     uint64_t block_start,
     uint64_t block_count );

/* Calculate and update allocation statistics */
int fvdecheck_volume_state_calculate_statistics(
     fvdecheck_volume_state_t *volume_state,
     libcerror_error_t **error );

/* Block number conversions */
uint64_t fvdecheck_linux_sector_to_fvde_block(
     uint64_t sector,
     uint32_t block_size );

uint64_t fvdecheck_fvde_block_to_linux_sector(
     uint64_t block,
     uint32_t block_size );

/* Get state name string */
const char *fvdecheck_extent_state_to_string(
     int state );

/* Get error type name string */
const char *fvdecheck_error_type_to_string(
     int error_type );

#if defined( __cplusplus )
}
#endif

#endif /* !defined( _FVDECHECK_EXTENT_H ) */
