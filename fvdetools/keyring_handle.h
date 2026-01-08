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

#if !defined( _KEYRING_HANDLE_H )
#define _KEYRING_HANDLE_H

#include <common.h>
#include <types.h>

#include "fvdetools_libcerror.h"

#if defined( __cplusplus )
extern "C" {
#endif

int keyring_handle_add_key(
     const uint8_t *volume_master_key,
     size_t volume_master_key_size,
     const uint8_t *volume_tweak_key,
     size_t volume_tweak_key_size,
     const uint8_t *volume_uuid,
     size_t volume_uuid_size,
     const char *keyring_id,
     int verbose,
     libcerror_error_t **error );

int keyring_handle_format_uuid_string(
     const uint8_t *uuid_data,
     size_t uuid_data_size,
     char *uuid_string,
     size_t uuid_string_size,
     libcerror_error_t **error );

#if defined( __cplusplus )
}
#endif

#endif /* !defined( _KEYRING_HANDLE_H ) */
