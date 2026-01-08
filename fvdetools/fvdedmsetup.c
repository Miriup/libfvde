/*
 * Device mapper setup tool for FileVault Drive Encrypted (FVDE) volumes
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
#include <system_string.h>
#include <types.h>

#include <stdio.h>

#if defined( HAVE_IO_H ) || defined( WINAPI )
#include <io.h>
#endif

#if defined( HAVE_STDLIB_H ) || defined( WINAPI )
#include <stdlib.h>
#endif

#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif

#include "dmsetup_handle.h"
#include "fvdetools_getopt.h"
#include "fvdetools_i18n.h"
#include "fvdetools_input.h"
#include "fvdetools_libcerror.h"
#include "fvdetools_libclocale.h"
#include "fvdetools_libcnotify.h"
#include "fvdetools_libfvde.h"
#include "fvdetools_output.h"
#include "fvdetools_signal.h"
#include "fvdetools_unused.h"
#include "keyring_handle.h"
#include "mount_handle.h"

/* Need to access internal structures to get keyring */
#include "../libfvde/libfvde_logical_volume.h"
#include "../libfvde/libfvde_keyring.h"

mount_handle_t *fvdedmsetup_mount_handle = NULL;
int fvdedmsetup_abort                    = 0;

/* Prints usage information
 */
void usage_fprint(
      FILE *stream )
{
	if( stream == NULL )
	{
		return;
	}
	fprintf( stream, "Use fvdedmsetup to setup device-mapper for FileVault Drive\n"
	                 " Encrypted (FVDE) volumes via Linux kernel keyring and dmsetup.\n\n" );

	fprintf( stream, "Usage: fvdedmsetup [ -e plist_path ] [ -k key ] [ -o offset ]\n"
	                 "                   [ -p password ] [ -r password ] [ -K keyring_id ]\n"
	                 "                   [ -m mapper_name ] [ -snhuvV ] sources\n\n" );

	fprintf( stream, "\tsources: one or more source files or devices\n\n" );

	fprintf( stream, "\t-e:      specify the path of the EncryptedRoot.plist.wipekey file\n" );
	fprintf( stream, "\t-h:      shows this help\n" );
	fprintf( stream, "\t-k:      specify the volume master key formatted in base16\n" );
	fprintf( stream, "\t-K:      specify the target kernel keyring ID (default: @s)\n" );
	fprintf( stream, "\t-m:      specify base name for device mapper devices (default: logical volume name)\n" );
	fprintf( stream, "\t-n:      dry-run mode (show what would be done without modifying keyring)\n" );
	fprintf( stream, "\t-o:      specify the volume offset in bytes\n" );
	fprintf( stream, "\t-p:      specify the password\n" );
	fprintf( stream, "\t-r:      specify the recovery password\n" );
	fprintf( stream, "\t-s:      output complete shell commands instead of raw dmsetup table format\n" );
	fprintf( stream, "\t-u:      unattended mode (disables user interaction)\n" );
	fprintf( stream, "\t-v:      verbose output to stderr\n" );
	fprintf( stream, "\t-V:      print version\n" );
}

/* Signal handler for fvdedmsetup
 */
void fvdedmsetup_signal_handler(
      fvdetools_signal_t signal FVDETOOLS_ATTRIBUTE_UNUSED )
{
	libcerror_error_t *error = NULL;
	static char *function    = "fvdedmsetup_signal_handler";

	FVDETOOLS_UNREFERENCED_PARAMETER( signal )

	fvdedmsetup_abort = 1;

	if( fvdedmsetup_mount_handle != NULL )
	{
		if( mount_handle_signal_abort(
		     fvdedmsetup_mount_handle,
		     &error ) != 1 )
		{
			libcnotify_printf(
			 "%s: unable to signal mount handle to abort.\n",
			 function );

			libcnotify_print_error_backtrace(
			 error );
			libcerror_error_free(
			 &error );
		}
	}
	/* Force stdin to close otherwise any function reading it will remain blocked
	 */
#if defined( WINAPI ) && !defined( __CYGWIN__ )
	if( _close(
	     0 ) != 0 )
#else
	if( close(
	     0 ) != 0 )
#endif
	{
		libcnotify_printf(
		 "%s: unable to close stdin.\n",
		 function );
	}
}

/* Processes a logical volume for dmsetup
 * Returns 1 if successful or -1 on error
 */
int fvdedmsetup_process_logical_volume(
     libfvde_logical_volume_t *logical_volume,
     const system_character_t *source_path,
     off64_t volume_offset,
     const char *keyring_id,
     const char *mapper_name,
     int volume_index,
     int shell_mode,
     int dry_run,
     int verbose,
     libcerror_error_t **error )
{
	libfvde_internal_logical_volume_t *internal_logical_volume = NULL;
	uint8_t uuid_data[ 16 ];
	uint8_t name_data[ 256 ];
	static char *function      = "fvdedmsetup_process_logical_volume";
	size64_t volume_size       = 0;
	size_t name_size           = 0;
	char *effective_mapper_name = NULL;
	char default_mapper_name[ 256 ];
	int is_locked              = 0;
	int result                 = 0;

	if( logical_volume == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid logical volume.",
		 function );

		return( -1 );
	}
	/* Check if volume is locked */
	if( libfvde_logical_volume_is_locked(
	     logical_volume,
	     &is_locked,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to determine if logical volume is locked.",
		 function );

		return( -1 );
	}
	if( is_locked != 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: logical volume is locked.",
		 function );

		return( -1 );
	}
	/* Get logical volume UUID */
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

		return( -1 );
	}
	/* Get logical volume size */
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

		return( -1 );
	}
	/* Determine mapper name */
	if( mapper_name == NULL )
	{
		/* Try to get logical volume name */
		result = libfvde_logical_volume_get_utf8_name_size(
		          logical_volume,
		          &name_size,
		          error );

		if( result == -1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve logical volume name size.",
			 function );

			return( -1 );
		}
		else if( ( result == 1 ) && ( name_size > 0 ) && ( name_size <= 256 ) )
		{
			if( libfvde_logical_volume_get_utf8_name(
			     logical_volume,
			     name_data,
			     name_size,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve logical volume name.",
				 function );

				return( -1 );
			}
			/* Copy name to default mapper name buffer */
			if( memory_copy(
			     default_mapper_name,
			     name_data,
			     name_size ) == NULL )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_MEMORY,
				 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
				 "%s: unable to copy volume name.",
				 function );

				return( -1 );
			}
			default_mapper_name[ name_size - 1 ] = '\0';
			effective_mapper_name = default_mapper_name;
		}
		else
		{
			/* Fallback to generic name */
			effective_mapper_name = "fvde";
		}
	}
	else
	{
		effective_mapper_name = (char *) mapper_name;
	}
	if( verbose != 0 )
	{
		char uuid_string[ 37 ];

		keyring_handle_format_uuid_string(
		 uuid_data,
		 16,
		 uuid_string,
		 37,
		 NULL );

		fprintf(
		 stderr,
		 "Logical volume %d:\n",
		 volume_index + 1 );
		fprintf(
		 stderr,
		 "  UUID: %s\n",
		 uuid_string );
		fprintf(
		 stderr,
		 "  Name: %s\n",
		 effective_mapper_name );
		fprintf(
		 stderr,
		 "  Size: %" PRIu64 " bytes (%" PRIu64 " sectors)\n",
		 volume_size,
		 volume_size / 512 );
	}
	/* Access internal structure to get keyring */
	internal_logical_volume = (libfvde_internal_logical_volume_t *) logical_volume;

	if( internal_logical_volume->keyring == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid logical volume - missing keyring.",
		 function );

		return( -1 );
	}
	/* Add key to kernel keyring (unless dry-run) */
	if( dry_run == 0 )
	{
		if( keyring_handle_add_key(
		     internal_logical_volume->keyring->volume_master_key,
		     16,
		     internal_logical_volume->keyring->volume_tweak_key,
		     32,
		     uuid_data,
		     16,
		     keyring_id,
		     verbose,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to add key to kernel keyring.",
			 function );

			return( -1 );
		}
	}
	else if( verbose != 0 )
	{
		fprintf(
		 stderr,
		 "  Dry-run: skipping kernel keyring storage\n" );
	}
	/* Output dmsetup table entry */
	if( dmsetup_handle_print_table_entry(
	     stdout,
	     uuid_data,
	     16,
	     volume_size,
	     source_path,
	     volume_offset,
	     shell_mode,
	     effective_mapper_name,
	     volume_index + 1,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_PRINT_FAILED,
		 "%s: unable to print dmsetup table entry.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* The main program
 */
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
int wmain( int argc, wchar_t * const argv[] )
#else
int main( int argc, char * const argv[] )
#endif
{
	libfvde_logical_volume_t *logical_volume = NULL;
	libfvde_volume_group_t *volume_group     = NULL;
	libcerror_error_t *error                 = NULL;
	system_character_t *option_encrypted_root_plist_path = NULL;
	system_character_t *option_key_data      = NULL;
	system_character_t *option_offset        = NULL;
	system_character_t *option_password      = NULL;
	system_character_t *option_recovery_password = NULL;
	system_character_t *source               = NULL;
	char *keyring_id                         = NULL;
	char *mapper_name                        = NULL;
	char *program                            = "fvdedmsetup";
	system_integer_t option                  = 0;
	int number_of_logical_volumes            = 0;
	int logical_volume_index                 = 0;
	int result                               = 0;
	int shell_mode                           = 0;
	int dry_run                              = 0;
	int unattended_mode                      = 0;
	int verbose                              = 0;

	libcnotify_stream_set(
	 stderr,
	 NULL );
	libcnotify_verbose_set(
	 1 );

	if( libclocale_initialize(
	     "fvdetools",
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to initialize locale values.\n" );

		goto on_error;
	}
	if( fvdetools_output_initialize(
	     _IONBF,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to initialize output settings.\n" );

		goto on_error;
	}
	fvdetools_output_version_fprint(
	 stderr,
	 program );

	while( ( option = fvdetools_getopt(
	                   argc,
	                   argv,
	                   _SYSTEM_STRING( "e:hk:K:m:no:p:r:suvV" ) ) ) != (system_integer_t) -1 )
	{
		switch( option )
		{
			case (system_integer_t) '?':
			default:
				fprintf(
				 stderr,
				 "Invalid argument: %" PRIs_SYSTEM ".\n",
				 argv[ optind - 1 ] );

				usage_fprint(
				 stderr );

				return( EXIT_FAILURE );

			case (system_integer_t) 'e':
				option_encrypted_root_plist_path = optarg;

				break;

			case (system_integer_t) 'h':
				usage_fprint(
				 stderr );

				return( EXIT_SUCCESS );

			case (system_integer_t) 'k':
				option_key_data = optarg;

				break;

			case (system_integer_t) 'K':
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
				keyring_id = narrow_string_allocate_from_wide(
				              optarg );
#else
				keyring_id = optarg;
#endif
				break;

			case (system_integer_t) 'm':
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
				mapper_name = narrow_string_allocate_from_wide(
				               optarg );
#else
				mapper_name = optarg;
#endif
				break;

			case (system_integer_t) 'n':
				dry_run = 1;

				break;

			case (system_integer_t) 'o':
				option_offset = optarg;

				break;

			case (system_integer_t) 'p':
				option_password = optarg;

				break;

			case (system_integer_t) 'r':
				option_recovery_password = optarg;

				break;

			case (system_integer_t) 's':
				shell_mode = 1;

				break;

			case (system_integer_t) 'u':
				unattended_mode = 1;

				break;

			case (system_integer_t) 'v':
				verbose = 1;

				break;

			case (system_integer_t) 'V':
				fvdetools_output_copyright_fprint(
				 stderr );

				return( EXIT_SUCCESS );
		}
	}
	if( optind == argc )
	{
		fprintf(
		 stderr,
		 "Missing source file or device.\n" );

		usage_fprint(
		 stderr );

		return( EXIT_FAILURE );
	}
	source = argv[ optind ];

	libcnotify_verbose_set(
	 verbose );
	libfvde_notify_set_stream(
	 stderr,
	 NULL );
	libfvde_notify_set_verbose(
	 verbose );

	if( mount_handle_initialize(
	     &fvdedmsetup_mount_handle,
	     unattended_mode,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to initialize mount handle.\n" );

		goto on_error;
	}
	if( option_encrypted_root_plist_path != NULL )
	{
		if( mount_handle_set_encrypted_root_plist(
		     fvdedmsetup_mount_handle,
		     option_encrypted_root_plist_path,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set encrypted root plist path.\n" );

			goto on_error;
		}
	}
	if( option_key_data != NULL )
	{
		if( mount_handle_set_key(
		     fvdedmsetup_mount_handle,
		     option_key_data,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set key.\n" );

			goto on_error;
		}
	}
	if( option_offset != NULL )
	{
		if( mount_handle_set_offset(
		     fvdedmsetup_mount_handle,
		     option_offset,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set volume offset.\n" );

			goto on_error;
		}
	}
	if( option_password != NULL )
	{
		if( mount_handle_set_password(
		     fvdedmsetup_mount_handle,
		     option_password,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set password.\n" );

			goto on_error;
		}
	}
	if( option_recovery_password != NULL )
	{
		if( mount_handle_set_recovery_password(
		     fvdedmsetup_mount_handle,
		     option_recovery_password,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set recovery password.\n" );

			goto on_error;
		}
	}
	if( fvdetools_signal_attach(
	     fvdedmsetup_signal_handler,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to attach signal handler.\n" );

		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	result = mount_handle_open(
	          fvdedmsetup_mount_handle,
	          &source,
	          1,
	          &error );

	if( result == -1 )
	{
		fprintf(
		 stderr,
		 "Unable to open FVDE volume.\n" );

		goto on_error;
	}
	else if( result == 0 )
	{
		fprintf(
		 stderr,
		 "Unable to unlock FVDE volume.\n" );

		goto on_error;
	}
	/* Get volume group */
	if( libfvde_volume_get_volume_group(
	     fvdedmsetup_mount_handle->volume,
	     &volume_group,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to retrieve volume group.\n" );

		goto on_error;
	}
	/* Get number of logical volumes */
	if( libfvde_volume_group_get_number_of_logical_volumes(
	     volume_group,
	     &number_of_logical_volumes,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to retrieve number of logical volumes.\n" );

		goto on_error;
	}
	if( verbose != 0 )
	{
		fprintf(
		 stderr,
		 "Found %d logical volume(s)\n",
		 number_of_logical_volumes );
	}
	/* Process each logical volume */
	for( logical_volume_index = 0;
	     logical_volume_index < number_of_logical_volumes;
	     logical_volume_index++ )
	{
		if( libfvde_volume_group_get_logical_volume_by_index(
		     volume_group,
		     logical_volume_index,
		     &logical_volume,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to retrieve logical volume: %d.\n",
			 logical_volume_index );

			goto on_error;
		}
		if( fvdedmsetup_process_logical_volume(
		     logical_volume,
		     source,
		     fvdedmsetup_mount_handle->volume_offset,
		     keyring_id,
		     mapper_name,
		     logical_volume_index,
		     shell_mode,
		     dry_run,
		     verbose,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to process logical volume: %d.\n",
			 logical_volume_index );

			goto on_error;
		}
		if( libfvde_logical_volume_free(
		     &logical_volume,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to free logical volume: %d.\n",
			 logical_volume_index );

			goto on_error;
		}
	}
	if( libfvde_volume_group_free(
	     &volume_group,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to free volume group.\n" );

		goto on_error;
	}
	if( fvdetools_signal_detach(
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to detach signal handler.\n" );

		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	if( mount_handle_close(
	     fvdedmsetup_mount_handle,
	     &error ) != 0 )
	{
		fprintf(
		 stderr,
		 "Unable to close mount handle.\n" );

		goto on_error;
	}
	if( mount_handle_free(
	     &fvdedmsetup_mount_handle,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to free mount handle.\n" );

		goto on_error;
	}
	return( EXIT_SUCCESS );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	if( logical_volume != NULL )
	{
		libfvde_logical_volume_free(
		 &logical_volume,
		 NULL );
	}
	if( volume_group != NULL )
	{
		libfvde_volume_group_free(
		 &volume_group,
		 NULL );
	}
	if( fvdedmsetup_mount_handle != NULL )
	{
		mount_handle_close(
		 fvdedmsetup_mount_handle,
		 NULL );
		mount_handle_free(
		 &fvdedmsetup_mount_handle,
		 NULL );
	}
	return( EXIT_FAILURE );
}
