/*
 * Validates FVDE metadata structures by tracking extent allocations
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

#if defined( HAVE_GETOPT_H )
#include <getopt.h>
#endif

#include "check_handle.h"
#include "fvdetools_getopt.h"
#include "fvdetools_i18n.h"
#include "fvdetools_libcerror.h"
#include "fvdetools_libclocale.h"
#include "fvdetools_libcnotify.h"
#include "fvdetools_libfvde.h"
#include "fvdetools_output.h"
#include "fvdetools_signal.h"
#include "fvdetools_unused.h"

check_handle_t *fvdecheck_check_handle = NULL;
int fvdecheck_abort                    = 0;

/* Prints usage information
 */
void usage_fprint(
      FILE *stream )
{
	if( stream == NULL )
	{
		return;
	}
	fprintf( stream, "Use fvdecheck to validate FVDE metadata structures by tracking\n"
	                 " extent allocations and detecting inconsistencies.\n\n" );

	fprintf( stream, "Usage: fvdecheck [ -e plist_path ] [ -k key ] [ -o offset ]\n"
	                 "                 [ -p password ] [ -r password ]\n"
	                 "                 [ --order=ORDER ] [ --stop-at-block=N ]\n"
	                 "                 [ --stop-at-transaction=ID ]\n"
	                 "                 [ --lookup-linux-sector=N ]\n"
	                 "                 [ --dump-allocation-map ] [ --json ]\n"
	                 "                 [ -hquvV ] sources\n\n" );

	fprintf( stream, "\tsources: one or more source files or devices\n\n" );

	fprintf( stream, "BASIC OPTIONS:\n" );
	fprintf( stream, "\t-e:      specify the path of the EncryptedRoot.plist.wipekey file\n" );
	fprintf( stream, "\t-h:      shows this help\n" );
	fprintf( stream, "\t-k:      specify the volume master key formatted in base16\n" );
	fprintf( stream, "\t-o:      specify the volume offset\n" );
	fprintf( stream, "\t-p:      specify the password\n" );
	fprintf( stream, "\t-q:      quiet mode, only show errors\n" );
	fprintf( stream, "\t-r:      specify the recovery password\n" );
	fprintf( stream, "\t-u:      unattended mode (disables user interaction)\n" );
	fprintf( stream, "\t-v:      verbose output to stderr\n" );
	fprintf( stream, "\t-V:      print version\n" );

	fprintf( stream, "\nPROCESSING ORDER:\n" );
	fprintf( stream, "\t--order=ORDER       Process metadata blocks in order:\n" );
	fprintf( stream, "\t                    ascending (oldest first, default)\n" );
	fprintf( stream, "\t                    descending (newest first)\n" );
	fprintf( stream, "\t                    physical (physical block order)\n" );

	fprintf( stream, "\nSTOP CONDITIONS:\n" );
	fprintf( stream, "\t--stop-at-block=N          Stop after processing metadata block N\n" );
	fprintf( stream, "\t--stop-at-transaction=ID   Stop after processing transaction ID\n" );

	fprintf( stream, "\nBLOCK LOOKUP:\n" );
	fprintf( stream, "\t--lookup-linux-sector=N    Look up Linux 512-byte sector N\n" );

	fprintf( stream, "\nOUTPUT OPTIONS:\n" );
	fprintf( stream, "\t--dump-allocation-map      Dump full allocation map after processing\n" );
	fprintf( stream, "\t--json                     Output in JSON format\n" );
}

/* Signal handler for fvdecheck
 */
void fvdecheck_signal_handler(
      fvdetools_signal_t signal FVDETOOLS_ATTRIBUTE_UNUSED )
{
	libcerror_error_t *error = NULL;
	static char *function    = "fvdecheck_signal_handler";

	FVDETOOLS_UNREFERENCED_PARAMETER( signal )

	fvdecheck_abort = 1;

	if( fvdecheck_check_handle != NULL )
	{
		if( check_handle_signal_abort(
		     fvdecheck_check_handle,
		     &error ) != 1 )
		{
			libcnotify_printf(
			 "%s: unable to signal check handle to abort.\n",
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

/* The main program
 */
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
int wmain( int argc, wchar_t * const argv[] )
#else
int main( int argc, char * const argv[] )
#endif
{
	system_character_t * const *sources                  = NULL;
	libfvde_error_t *error                               = NULL;
	system_character_t *option_encrypted_root_plist_path = NULL;
	system_character_t *option_key                       = NULL;
	system_character_t *option_password                  = NULL;
	system_character_t *option_recovery_password         = NULL;
	system_character_t *option_volume_offset             = NULL;
	system_character_t *option_order                     = NULL;
	system_character_t *option_stop_at_block             = NULL;
	system_character_t *option_stop_at_transaction       = NULL;
	system_character_t *option_lookup_linux_sector       = NULL;
	char *program                                        = "fvdecheck";
	system_integer_t option                              = 0;
	int number_of_sources                                = 0;
	int unattended_mode                                  = 0;
	int verbose                                          = 0;
	int quiet_mode                                       = 0;
	int json_mode                                        = 0;
	int dump_allocation_map                              = 0;
	int option_index                                     = 0;

#if defined( HAVE_GETOPT_LONG )
	static struct option long_options[] = {
		{ "order",                 required_argument, NULL, 'O' },
		{ "stop-at-block",         required_argument, NULL, 'B' },
		{ "stop-at-transaction",   required_argument, NULL, 'T' },
		{ "lookup-linux-sector",   required_argument, NULL, 'L' },
		{ "dump-allocation-map",   no_argument,       NULL, 'D' },
		{ "json",                  no_argument,       NULL, 'J' },
		{ "help",                  no_argument,       NULL, 'h' },
		{ "verbose",               no_argument,       NULL, 'v' },
		{ "version",               no_argument,       NULL, 'V' },
		{ NULL,                    0,                 NULL, 0   }
	};
#endif

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
	 stdout,
	 program );

#if defined( HAVE_GETOPT_LONG )
	while( ( option = getopt_long(
	                   argc,
	                   argv,
	                   _SYSTEM_STRING( "e:hk:o:p:qr:uvV" ),
	                   long_options,
	                   &option_index ) ) != (system_integer_t) -1 )
#else
	while( ( option = fvdetools_getopt(
	                   argc,
	                   argv,
	                   _SYSTEM_STRING( "e:hk:o:p:qr:uvV" ) ) ) != (system_integer_t) -1 )
#endif
	{
		switch( option )
		{
			case (system_integer_t) '?':
			default:
				fprintf(
				 stderr,
				 "Invalid argument: %" PRIs_SYSTEM "\n",
				 argv[ optind - 1 ] );

				usage_fprint(
				 stdout );

				return( EXIT_FAILURE );

			case (system_integer_t) 'e':
				option_encrypted_root_plist_path = optarg;

				break;

			case (system_integer_t) 'h':
				usage_fprint(
				 stdout );

				return( EXIT_SUCCESS );

			case (system_integer_t) 'k':
				option_key = optarg;

				break;

			case (system_integer_t) 'o':
				option_volume_offset = optarg;

				break;

			case (system_integer_t) 'p':
				option_password = optarg;

				break;

			case (system_integer_t) 'q':
				quiet_mode = 1;

				break;

			case (system_integer_t) 'r':
				option_recovery_password = optarg;

				break;

			case (system_integer_t) 'u':
				unattended_mode = 1;

				break;

			case (system_integer_t) 'v':
				verbose = 1;

				break;

			case (system_integer_t) 'V':
				fvdetools_output_copyright_fprint(
				 stdout );

				return( EXIT_SUCCESS );

			/* Long options */
			case (system_integer_t) 'O':
				option_order = optarg;

				break;

			case (system_integer_t) 'B':
				option_stop_at_block = optarg;

				break;

			case (system_integer_t) 'T':
				option_stop_at_transaction = optarg;

				break;

			case (system_integer_t) 'L':
				option_lookup_linux_sector = optarg;

				break;

			case (system_integer_t) 'D':
				dump_allocation_map = 1;

				break;

			case (system_integer_t) 'J':
				json_mode = 1;

				break;
		}
	}
	if( optind == argc )
	{
		fprintf(
		 stderr,
		 "Missing source file or device.\n" );

		usage_fprint(
		 stdout );

		return( EXIT_FAILURE );
	}
	sources           = &( argv[ optind ] );
	number_of_sources = argc - optind;

	libcnotify_verbose_set(
	 verbose );
	libfvde_notify_set_stream(
	 stderr,
	 NULL );
	libfvde_notify_set_verbose(
	 verbose );

	if( check_handle_initialize(
	     &fvdecheck_check_handle,
	     unattended_mode,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to initialize check handle.\n" );

		goto on_error;
	}
	fvdecheck_check_handle->verbose_mode = verbose;
	fvdecheck_check_handle->quiet_mode = quiet_mode;
	fvdecheck_check_handle->json_mode = json_mode;
	fvdecheck_check_handle->dump_allocation_map = dump_allocation_map;

	if( option_encrypted_root_plist_path != NULL )
	{
		if( check_handle_set_encrypted_root_plist(
		     fvdecheck_check_handle,
		     option_encrypted_root_plist_path,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set path of EncryptedRoot.plist.wipekey file.\n" );

			goto on_error;
		}
	}
	if( option_key != NULL )
	{
		if( check_handle_set_key(
		     fvdecheck_check_handle,
		     option_key,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set key.\n" );

			goto on_error;
		}
	}
	if( option_password != NULL )
	{
		if( check_handle_set_password(
		     fvdecheck_check_handle,
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
		if( check_handle_set_recovery_password(
		     fvdecheck_check_handle,
		     option_recovery_password,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set recovery password.\n" );

			goto on_error;
		}
	}
	if( option_volume_offset != NULL )
	{
		if( check_handle_set_volume_offset(
		     fvdecheck_check_handle,
		     option_volume_offset,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set volume offset.\n" );

			goto on_error;
		}
	}
	if( option_order != NULL )
	{
		if( check_handle_set_order(
		     fvdecheck_check_handle,
		     option_order,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set processing order.\n" );

			goto on_error;
		}
	}
	if( option_stop_at_block != NULL )
	{
		if( check_handle_set_stop_at_block(
		     fvdecheck_check_handle,
		     option_stop_at_block,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set stop-at-block.\n" );

			goto on_error;
		}
	}
	if( option_stop_at_transaction != NULL )
	{
		if( check_handle_set_stop_at_transaction(
		     fvdecheck_check_handle,
		     option_stop_at_transaction,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set stop-at-transaction.\n" );

			goto on_error;
		}
	}
	if( option_lookup_linux_sector != NULL )
	{
		if( check_handle_set_lookup_linux_sector(
		     fvdecheck_check_handle,
		     option_lookup_linux_sector,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to set lookup-linux-sector.\n" );

			goto on_error;
		}
	}
	if( !quiet_mode && !json_mode )
	{
		fprintf(
		 stdout,
		 "Opening volume...\n" );
	}
	if( check_handle_open(
	     fvdecheck_check_handle,
	     sources,
	     number_of_sources,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to open: %" PRIs_SYSTEM ".\n",
		 sources[ 0 ] );

		goto on_error;
	}
	if( !quiet_mode && !json_mode )
	{
		fprintf(
		 stdout,
		 "Volume opened successfully.\n" );

		fprintf(
		 stdout,
		 "Physical volumes: %" PRIu32 "\n",
		 fvdecheck_check_handle->volume_state->num_physical_volumes );

		fprintf(
		 stdout,
		 "Logical volumes: %" PRIu32 "\n",
		 fvdecheck_check_handle->volume_state->num_logical_volumes );

		fprintf(
		 stdout,
		 "Block size: %" PRIu32 " bytes\n",
		 fvdecheck_check_handle->volume_state->block_size );
	}
	/* Perform block lookup if requested */
	if( fvdecheck_check_handle->lookup_linux_sector_set )
	{
		if( check_handle_lookup_block(
		     fvdecheck_check_handle,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to perform block lookup.\n" );

			goto on_error;
		}
	}
	/* Print output based on mode */
	if( json_mode )
	{
		if( check_handle_print_json(
		     fvdecheck_check_handle,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to print JSON output.\n" );

			goto on_error;
		}
	}
	else
	{
		if( check_handle_print_allocation_summary(
		     fvdecheck_check_handle,
		     &error ) != 1 )
		{
			fprintf(
			 stderr,
			 "Unable to print allocation summary.\n" );

			goto on_error;
		}
		if( dump_allocation_map )
		{
			if( check_handle_print_allocation_map(
			     fvdecheck_check_handle,
			     &error ) != 1 )
			{
				fprintf(
				 stderr,
				 "Unable to print allocation map.\n" );

				goto on_error;
			}
		}
	}
	if( check_handle_close(
	     fvdecheck_check_handle,
	     &error ) != 0 )
	{
		fprintf(
		 stderr,
		 "Unable to close check handle.\n" );

		goto on_error;
	}
	if( check_handle_free(
	     &fvdecheck_check_handle,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to free check handle.\n" );

		goto on_error;
	}
	if( !quiet_mode && !json_mode )
	{
		fprintf(
		 stdout,
		 "\nfvdecheck completed.\n" );
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
	if( fvdecheck_check_handle != NULL )
	{
		check_handle_free(
		 &fvdecheck_check_handle,
		 NULL );
	}
	return( EXIT_FAILURE );
}
