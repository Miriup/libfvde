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

#include <common.h>
#include <memory.h>
#include <types.h>

#include "fvdecheck_extent.h"
#include "fvdetools_libcerror.h"

/* Initialize volume state structure
 * Returns 1 if successful or -1 on error
 */
int fvdecheck_volume_state_initialize(
     fvdecheck_volume_state_t **volume_state,
     libcerror_error_t **error )
{
	static char *function = "fvdecheck_volume_state_initialize";

	if( volume_state == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume state.",
		 function );

		return( -1 );
	}
	if( *volume_state != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid volume state value already set.",
		 function );

		return( -1 );
	}
	*volume_state = memory_allocate_structure(
	                 fvdecheck_volume_state_t );

	if( *volume_state == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create volume state.",
		 function );

		return( -1 );
	}
	if( memory_set(
	     *volume_state,
	     0,
	     sizeof( fvdecheck_volume_state_t ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear volume state.",
		 function );

		memory_free(
		 *volume_state );

		*volume_state = NULL;

		return( -1 );
	}
	/* Default block size */
	( *volume_state )->block_size = 4096;

	return( 1 );
}

/* Free volume state structure
 * Returns 1 if successful or -1 on error
 */
int fvdecheck_volume_state_free(
     fvdecheck_volume_state_t **volume_state,
     libcerror_error_t **error )
{
	fvdecheck_extent_t *current_extent = NULL;
	fvdecheck_extent_t *next_extent    = NULL;
	static char *function              = "fvdecheck_volume_state_free";
	uint32_t pv_index                  = 0;

	if( volume_state == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume state.",
		 function );

		return( -1 );
	}
	if( *volume_state != NULL )
	{
		/* Free all extents from physical volume lists */
		for( pv_index = 0;
		     pv_index < ( *volume_state )->num_physical_volumes;
		     pv_index++ )
		{
			current_extent = ( *volume_state )->physical_volumes[ pv_index ].extent_list_head;

			while( current_extent != NULL )
			{
				next_extent = current_extent->phys_next;

				memory_free(
				 current_extent );

				current_extent = next_extent;
			}
		}
		memory_free(
		 *volume_state );

		*volume_state = NULL;
	}
	return( 1 );
}

/* Initialize an extent
 * Returns 1 if successful or -1 on error
 */
int fvdecheck_extent_initialize(
     fvdecheck_extent_t **extent,
     libcerror_error_t **error )
{
	static char *function = "fvdecheck_extent_initialize";

	if( extent == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid extent.",
		 function );

		return( -1 );
	}
	if( *extent != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid extent value already set.",
		 function );

		return( -1 );
	}
	*extent = memory_allocate_structure(
	           fvdecheck_extent_t );

	if( *extent == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create extent.",
		 function );

		return( -1 );
	}
	if( memory_set(
	     *extent,
	     0,
	     sizeof( fvdecheck_extent_t ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear extent.",
		 function );

		memory_free(
		 *extent );

		*extent = NULL;

		return( -1 );
	}
	return( 1 );
}

/* Free an extent
 * Returns 1 if successful or -1 on error
 */
int fvdecheck_extent_free(
     fvdecheck_extent_t **extent,
     libcerror_error_t **error )
{
	static char *function = "fvdecheck_extent_free";

	if( extent == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid extent.",
		 function );

		return( -1 );
	}
	if( *extent != NULL )
	{
		memory_free(
		 *extent );

		*extent = NULL;
	}
	return( 1 );
}

/* Add a physical volume to the state
 * Returns 1 if successful or -1 on error
 */
int fvdecheck_volume_state_add_physical_volume(
     fvdecheck_volume_state_t *volume_state,
     const uint8_t *uuid,
     uint64_t size_in_blocks,
     uint32_t *pv_index,
     libcerror_error_t **error )
{
	static char *function = "fvdecheck_volume_state_add_physical_volume";
	uint32_t new_index    = 0;

	if( volume_state == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume state.",
		 function );

		return( -1 );
	}
	if( uuid == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid UUID.",
		 function );

		return( -1 );
	}
	if( pv_index == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid physical volume index.",
		 function );

		return( -1 );
	}
	if( volume_state->num_physical_volumes >= FVDECHECK_MAX_PHYSICAL_VOLUMES )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: maximum number of physical volumes reached.",
		 function );

		return( -1 );
	}
	new_index = volume_state->num_physical_volumes;

	if( memory_copy(
	     volume_state->physical_volumes[ new_index ].uuid,
	     uuid,
	     16 ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy UUID.",
		 function );

		return( -1 );
	}
	volume_state->physical_volumes[ new_index ].size_in_blocks = size_in_blocks;
	volume_state->physical_volumes[ new_index ].extent_list_head = NULL;
	volume_state->physical_volumes[ new_index ].extent_list_tail = NULL;
	volume_state->physical_volumes[ new_index ].reserved_blocks = 0;
	volume_state->physical_volumes[ new_index ].allocated_blocks = 0;
	volume_state->physical_volumes[ new_index ].free_blocks = 0;

	volume_state->num_physical_volumes++;
	*pv_index = new_index;

	return( 1 );
}

/* Add a logical volume to the state
 * Returns 1 if successful or -1 on error
 */
int fvdecheck_volume_state_add_logical_volume(
     fvdecheck_volume_state_t *volume_state,
     const uint8_t *uuid,
     uint64_t size_in_blocks,
     uint32_t *lv_index,
     libcerror_error_t **error )
{
	static char *function = "fvdecheck_volume_state_add_logical_volume";
	uint32_t new_index    = 0;

	if( volume_state == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume state.",
		 function );

		return( -1 );
	}
	if( uuid == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid UUID.",
		 function );

		return( -1 );
	}
	if( lv_index == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid logical volume index.",
		 function );

		return( -1 );
	}
	if( volume_state->num_logical_volumes >= FVDECHECK_MAX_LOGICAL_VOLUMES )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: maximum number of logical volumes reached.",
		 function );

		return( -1 );
	}
	new_index = volume_state->num_logical_volumes;

	if( memory_copy(
	     volume_state->logical_volumes[ new_index ].uuid,
	     uuid,
	     16 ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy UUID.",
		 function );

		return( -1 );
	}
	volume_state->logical_volumes[ new_index ].size_in_blocks = size_in_blocks;
	volume_state->logical_volumes[ new_index ].extent_list_head = NULL;
	volume_state->logical_volumes[ new_index ].extent_list_tail = NULL;
	volume_state->logical_volumes[ new_index ].mapped_blocks = 0;
	volume_state->logical_volumes[ new_index ].unmapped_blocks = 0;

	volume_state->num_logical_volumes++;
	*lv_index = new_index;

	return( 1 );
}

/* Insert extent into physical volume list maintaining sorted order by block_start
 */
static void fvdecheck_insert_extent_physical(
              fvdecheck_volume_state_t *volume_state,
              uint32_t pv_index,
              fvdecheck_extent_t *extent )
{
	fvdecheck_extent_t *current = NULL;
	fvdecheck_physical_volume_info_t *pv_info = NULL;

	if( volume_state == NULL || extent == NULL )
	{
		return;
	}
	if( pv_index >= volume_state->num_physical_volumes )
	{
		return;
	}
	pv_info = &( volume_state->physical_volumes[ pv_index ] );

	/* Empty list case */
	if( pv_info->extent_list_head == NULL )
	{
		pv_info->extent_list_head = extent;
		pv_info->extent_list_tail = extent;
		extent->phys_prev = NULL;
		extent->phys_next = NULL;
		return;
	}
	/* Find insertion point */
	current = pv_info->extent_list_head;

	while( current != NULL )
	{
		if( extent->physical_block_start < current->physical_block_start )
		{
			/* Insert before current */
			extent->phys_prev = current->phys_prev;
			extent->phys_next = current;

			if( current->phys_prev != NULL )
			{
				current->phys_prev->phys_next = extent;
			}
			else
			{
				pv_info->extent_list_head = extent;
			}
			current->phys_prev = extent;
			return;
		}
		current = current->phys_next;
	}
	/* Insert at end */
	extent->phys_prev = pv_info->extent_list_tail;
	extent->phys_next = NULL;
	pv_info->extent_list_tail->phys_next = extent;
	pv_info->extent_list_tail = extent;
}

/* Insert extent into logical volume list maintaining sorted order by logical_block_start
 */
static void fvdecheck_insert_extent_logical(
              fvdecheck_volume_state_t *volume_state,
              uint32_t lv_index,
              fvdecheck_extent_t *extent )
{
	fvdecheck_extent_t *current = NULL;
	fvdecheck_logical_volume_info_t *lv_info = NULL;

	if( volume_state == NULL || extent == NULL )
	{
		return;
	}
	if( lv_index >= volume_state->num_logical_volumes )
	{
		return;
	}
	lv_info = &( volume_state->logical_volumes[ lv_index ] );

	/* Empty list case */
	if( lv_info->extent_list_head == NULL )
	{
		lv_info->extent_list_head = extent;
		lv_info->extent_list_tail = extent;
		extent->logical_prev = NULL;
		extent->logical_next = NULL;
		return;
	}
	/* Find insertion point */
	current = lv_info->extent_list_head;

	while( current != NULL )
	{
		if( extent->logical_block_start < current->logical_block_start )
		{
			/* Insert before current */
			extent->logical_prev = current->logical_prev;
			extent->logical_next = current;

			if( current->logical_prev != NULL )
			{
				current->logical_prev->logical_next = extent;
			}
			else
			{
				lv_info->extent_list_head = extent;
			}
			current->logical_prev = extent;
			return;
		}
		current = current->logical_next;
	}
	/* Insert at end */
	extent->logical_prev = lv_info->extent_list_tail;
	extent->logical_next = NULL;
	lv_info->extent_list_tail->logical_next = extent;
	lv_info->extent_list_tail = extent;
}

/* Mark a physical extent as reserved
 * Returns 1 if successful or -1 on error
 */
int fvdecheck_volume_state_mark_reserved(
     fvdecheck_volume_state_t *volume_state,
     uint32_t pv_index,
     uint64_t block_start,
     uint64_t block_count,
     const char *description,
     libcerror_error_t **error )
{
	fvdecheck_extent_t *extent = NULL;
	static char *function      = "fvdecheck_volume_state_mark_reserved";

	if( volume_state == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume state.",
		 function );

		return( -1 );
	}
	if( pv_index >= volume_state->num_physical_volumes )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: physical volume index out of bounds.",
		 function );

		return( -1 );
	}
	if( fvdecheck_extent_initialize(
	     &extent,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create extent.",
		 function );

		return( -1 );
	}
	extent->physical_volume_index = pv_index;
	extent->physical_block_start = block_start;
	extent->physical_block_count = block_count;
	extent->state = FVDECHECK_EXTENT_STATE_RESERVED;
	extent->reserved_description = description;

	fvdecheck_insert_extent_physical(
	 volume_state,
	 pv_index,
	 extent );

	volume_state->total_extents++;

	return( 1 );
}

/* Mark a physical extent as free
 * Returns 1 if successful or -1 on error
 */
int fvdecheck_volume_state_mark_free(
     fvdecheck_volume_state_t *volume_state,
     uint32_t pv_index,
     uint64_t block_start,
     uint64_t block_count,
     uint64_t transaction_id,
     uint32_t metadata_block_index,
     uint16_t block_type,
     libcerror_error_t **error )
{
	fvdecheck_extent_t *extent = NULL;
	static char *function      = "fvdecheck_volume_state_mark_free";

	if( volume_state == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume state.",
		 function );

		return( -1 );
	}
	if( pv_index >= volume_state->num_physical_volumes )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: physical volume index out of bounds.",
		 function );

		return( -1 );
	}
	if( fvdecheck_extent_initialize(
	     &extent,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create extent.",
		 function );

		return( -1 );
	}
	extent->physical_volume_index = pv_index;
	extent->physical_block_start = block_start;
	extent->physical_block_count = block_count;
	extent->state = FVDECHECK_EXTENT_STATE_FREE;
	extent->transaction_id = transaction_id;
	extent->metadata_block_index = metadata_block_index;
	extent->block_type = block_type;

	fvdecheck_insert_extent_physical(
	 volume_state,
	 pv_index,
	 extent );

	volume_state->total_extents++;

	return( 1 );
}

/* Mark an extent as allocated (maps physical to logical)
 * Returns 1 if successful or -1 on error
 */
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
     libcerror_error_t **error )
{
	fvdecheck_extent_t *extent = NULL;
	static char *function      = "fvdecheck_volume_state_mark_allocated";

	if( volume_state == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume state.",
		 function );

		return( -1 );
	}
	if( pv_index >= volume_state->num_physical_volumes )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: physical volume index out of bounds.",
		 function );

		return( -1 );
	}
	if( lv_index >= volume_state->num_logical_volumes )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: logical volume index out of bounds.",
		 function );

		return( -1 );
	}
	if( fvdecheck_extent_initialize(
	     &extent,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create extent.",
		 function );

		return( -1 );
	}
	extent->physical_volume_index = pv_index;
	extent->physical_block_start = phys_block_start;
	extent->physical_block_count = block_count;
	extent->logical_volume_index = lv_index;
	extent->logical_block_start = logical_block_start;
	extent->state = FVDECHECK_EXTENT_STATE_ALLOCATED;
	extent->transaction_id = transaction_id;
	extent->metadata_block_index = metadata_block_index;
	extent->block_type = block_type;

	fvdecheck_insert_extent_physical(
	 volume_state,
	 pv_index,
	 extent );

	fvdecheck_insert_extent_logical(
	 volume_state,
	 lv_index,
	 extent );

	volume_state->total_extents++;

	return( 1 );
}

/* Find extent containing a physical block
 * Returns pointer to extent or NULL if not found
 */
fvdecheck_extent_t *fvdecheck_volume_state_find_physical_extent(
                     fvdecheck_volume_state_t *volume_state,
                     uint32_t pv_index,
                     uint64_t block_number )
{
	fvdecheck_extent_t *current = NULL;

	if( volume_state == NULL )
	{
		return( NULL );
	}
	if( pv_index >= volume_state->num_physical_volumes )
	{
		return( NULL );
	}
	current = volume_state->physical_volumes[ pv_index ].extent_list_head;

	while( current != NULL )
	{
		if( ( block_number >= current->physical_block_start )
		 && ( block_number < ( current->physical_block_start + current->physical_block_count ) ) )
		{
			return( current );
		}
		/* Since list is sorted, we can stop early */
		if( current->physical_block_start > block_number )
		{
			break;
		}
		current = current->phys_next;
	}
	return( NULL );
}

/* Find extent containing a logical block
 * Returns pointer to extent or NULL if not found
 */
fvdecheck_extent_t *fvdecheck_volume_state_find_logical_extent(
                     fvdecheck_volume_state_t *volume_state,
                     uint32_t lv_index,
                     uint64_t block_number )
{
	fvdecheck_extent_t *current = NULL;

	if( volume_state == NULL )
	{
		return( NULL );
	}
	if( lv_index >= volume_state->num_logical_volumes )
	{
		return( NULL );
	}
	current = volume_state->logical_volumes[ lv_index ].extent_list_head;

	while( current != NULL )
	{
		if( ( block_number >= current->logical_block_start )
		 && ( block_number < ( current->logical_block_start + current->physical_block_count ) ) )
		{
			return( current );
		}
		/* Since list is sorted, we can stop early */
		if( current->logical_block_start > block_number )
		{
			break;
		}
		current = current->logical_next;
	}
	return( NULL );
}

/* Check for overlap with existing extents in physical space
 * Returns pointer to overlapping extent or NULL if no overlap
 */
fvdecheck_extent_t *fvdecheck_volume_state_check_overlap(
                     fvdecheck_volume_state_t *volume_state,
                     uint32_t pv_index,
                     uint64_t block_start,
                     uint64_t block_count )
{
	fvdecheck_extent_t *current = NULL;
	uint64_t block_end          = 0;
	uint64_t current_end        = 0;

	if( volume_state == NULL )
	{
		return( NULL );
	}
	if( pv_index >= volume_state->num_physical_volumes )
	{
		return( NULL );
	}
	block_end = block_start + block_count;
	current = volume_state->physical_volumes[ pv_index ].extent_list_head;

	while( current != NULL )
	{
		current_end = current->physical_block_start + current->physical_block_count;

		/* Check for overlap: ranges overlap if start1 < end2 && start2 < end1 */
		if( ( block_start < current_end )
		 && ( current->physical_block_start < block_end ) )
		{
			return( current );
		}
		/* Since list is sorted, we can stop if current starts after our range */
		if( current->physical_block_start >= block_end )
		{
			break;
		}
		current = current->phys_next;
	}
	return( NULL );
}

/* Calculate and update allocation statistics
 * Returns 1 if successful or -1 on error
 */
int fvdecheck_volume_state_calculate_statistics(
     fvdecheck_volume_state_t *volume_state,
     libcerror_error_t **error )
{
	fvdecheck_extent_t *current = NULL;
	static char *function       = "fvdecheck_volume_state_calculate_statistics";
	uint32_t lv_index           = 0;
	uint32_t pv_index           = 0;

	if( volume_state == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid volume state.",
		 function );

		return( -1 );
	}
	/* Calculate physical volume statistics */
	for( pv_index = 0;
	     pv_index < volume_state->num_physical_volumes;
	     pv_index++ )
	{
		volume_state->physical_volumes[ pv_index ].reserved_blocks = 0;
		volume_state->physical_volumes[ pv_index ].allocated_blocks = 0;
		volume_state->physical_volumes[ pv_index ].free_blocks = 0;

		current = volume_state->physical_volumes[ pv_index ].extent_list_head;

		while( current != NULL )
		{
			switch( current->state )
			{
				case FVDECHECK_EXTENT_STATE_RESERVED:
					volume_state->physical_volumes[ pv_index ].reserved_blocks += current->physical_block_count;
					break;

				case FVDECHECK_EXTENT_STATE_ALLOCATED:
					volume_state->physical_volumes[ pv_index ].allocated_blocks += current->physical_block_count;
					break;

				case FVDECHECK_EXTENT_STATE_FREE:
					volume_state->physical_volumes[ pv_index ].free_blocks += current->physical_block_count;
					break;

				default:
					break;
			}
			current = current->phys_next;
		}
	}
	/* Calculate logical volume statistics */
	for( lv_index = 0;
	     lv_index < volume_state->num_logical_volumes;
	     lv_index++ )
	{
		volume_state->logical_volumes[ lv_index ].mapped_blocks = 0;

		current = volume_state->logical_volumes[ lv_index ].extent_list_head;

		while( current != NULL )
		{
			volume_state->logical_volumes[ lv_index ].mapped_blocks += current->physical_block_count;
			current = current->logical_next;
		}
		if( volume_state->logical_volumes[ lv_index ].size_in_blocks > volume_state->logical_volumes[ lv_index ].mapped_blocks )
		{
			volume_state->logical_volumes[ lv_index ].unmapped_blocks =
			    volume_state->logical_volumes[ lv_index ].size_in_blocks -
			    volume_state->logical_volumes[ lv_index ].mapped_blocks;
		}
		else
		{
			volume_state->logical_volumes[ lv_index ].unmapped_blocks = 0;
		}
	}
	return( 1 );
}

/* Convert Linux sector to FVDE block */
uint64_t fvdecheck_linux_sector_to_fvde_block(
          uint64_t sector,
          uint32_t block_size )
{
	if( block_size == 0 )
	{
		return( 0 );
	}
	return( ( sector * 512 ) / block_size );
}

/* Convert FVDE block to Linux sector */
uint64_t fvdecheck_fvde_block_to_linux_sector(
          uint64_t block,
          uint32_t block_size )
{
	return( ( block * block_size ) / 512 );
}

/* Get state name string */
const char *fvdecheck_extent_state_to_string(
             int state )
{
	switch( state )
	{
		case FVDECHECK_EXTENT_STATE_UNKNOWN:
			return( "UNKNOWN" );

		case FVDECHECK_EXTENT_STATE_FREE:
			return( "FREE" );

		case FVDECHECK_EXTENT_STATE_ALLOCATED:
			return( "ALLOCATED" );

		case FVDECHECK_EXTENT_STATE_RESERVED:
			return( "RESERVED" );

		default:
			return( "INVALID" );
	}
}

/* Get error type name string */
const char *fvdecheck_error_type_to_string(
             int error_type )
{
	switch( error_type )
	{
		case FVDECHECK_ERROR_PHYSICAL_OVERLAP:
			return( "Physical overlap" );

		case FVDECHECK_ERROR_LOGICAL_OVERLAP:
			return( "Logical overlap" );

		case FVDECHECK_ERROR_ALLOCATE_AFTER_ALLOC:
			return( "Block already allocated" );

		case FVDECHECK_ERROR_RESERVED_VIOLATION:
			return( "Allocation overlaps reserved area" );

		case FVDECHECK_ERROR_FREE_AFTER_FREE:
			return( "Block freed when already free" );

		default:
			return( "Unknown error" );
	}
}
