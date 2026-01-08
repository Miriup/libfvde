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

#if !defined( _DMSETUP_HANDLE_H )
#define _DMSETUP_HANDLE_H

#include <common.h>
#include <types.h>

#include "fvdetools_libcerror.h"

#if defined( __cplusplus )
extern "C" {
#endif

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
     libcerror_error_t **error );

#if defined( __cplusplus )
}
#endif

#endif /* !defined( _DMSETUP_HANDLE_H ) */
