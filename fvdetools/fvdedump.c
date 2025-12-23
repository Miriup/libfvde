/*
 * Extracts FVDE (FileVault Drive Encryption) metadata from a block device
 * or image file into a sparse file for debugging decryption issues.
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

#include "dump_handle.h"
#include "fvdetools_getopt.h"
#include "fvdetools_i18n.h"
#include "fvdetools_libcerror.h"
#include "fvdetools_libclocale.h"
#include "fvdetools_libcnotify.h"
#include "fvdetools_libfvde.h"
#include "fvdetools_output.h"
#include "fvdetools_signal.h"
#include "fvdetools_unused.h"

dump_handle_t *fvdedump_dump_handle = NULL;
int fvdedump_abort                  = 0;

/* Prints usage information
 */
void usage_fprint(
      FILE *stream )
{
	if( stream == NULL )
	{
		return;
	}
	fprintf( stream, "Use fvdedump to extract FVDE metadata from a FileVault encrypted\n"
	                 "volume into a sparse file for debugging.\n\n" );

	fprintf( stream, "Usage: fvdedump [ -bcfhvV ] [ -s sample_blocks ] source destination\n\n" );

	fprintf( stream, "\tsource:      the source file or block device\n" );
	fprintf( stream, "\tdestination: the destination file for the metadata dump\n\n" );

	fprintf( stream, "\t-b:          copy only best metadata (highest transaction ID)\n" );
	fprintf( stream, "\t-c:          compact mode (non-sparse file with adjusted offsets)\n" );
	fprintf( stream, "\t-f:          force overwrite of destination if it exists\n" );
	fprintf( stream, "\t-h:          shows this help\n" );
	fprintf( stream, "\t-s:          include first N encrypted filesystem blocks (default: 0)\n" );
	fprintf( stream, "\t-v:          verbose output to stderr\n" );
	fprintf( stream, "\t-V:          print version\n" );
}

/* Signal handler for fvdedump
 */
void fvdedump_signal_handler(
      fvdetools_signal_t signal FVDETOOLS_ATTRIBUTE_UNUSED )
{
	libcerror_error_t *error = NULL;
	static char *function    = "fvdedump_signal_handler";

	FVDETOOLS_UNREFERENCED_PARAMETER( signal )

	fvdedump_abort = 1;

	if( fvdedump_dump_handle != NULL )
	{
		if( dump_handle_signal_abort(
		     fvdedump_dump_handle,
		     &error ) != 1 )
		{
			libcnotify_printf(
			 "%s: unable to signal dump handle to abort.\n",
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
	libfvde_error_t *error                    = NULL;
	system_character_t *source                = NULL;
	system_character_t *destination           = NULL;
	system_character_t *option_sample_blocks  = NULL;
	char *program                             = "fvdedump";
	system_integer_t option                   = 0;
	int best_metadata_only                    = 0;
	int compact_mode                          = 0;
	int force_overwrite                       = 0;
	int sample_blocks                         = 0;
	int verbose                               = 0;

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

	while( ( option = fvdetools_getopt(
	                   argc,
	                   argv,
	                   _SYSTEM_STRING( "bcfhs:vV" ) ) ) != (system_integer_t) -1 )
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

			case (system_integer_t) 'b':
				best_metadata_only = 1;

				break;

			case (system_integer_t) 'c':
				compact_mode = 1;

				break;

			case (system_integer_t) 'f':
				force_overwrite = 1;

				break;

			case (system_integer_t) 'h':
				usage_fprint(
				 stdout );

				return( EXIT_SUCCESS );

			case (system_integer_t) 's':
				option_sample_blocks = optarg;

				break;

			case (system_integer_t) 'v':
				verbose = 1;

				break;

			case (system_integer_t) 'V':
				fvdetools_output_copyright_fprint(
				 stdout );

				return( EXIT_SUCCESS );
		}
	}
	if( optind >= argc )
	{
		fprintf(
		 stderr,
		 "Missing source file or device.\n" );

		usage_fprint(
		 stdout );

		return( EXIT_FAILURE );
	}
	source = argv[ optind++ ];

	if( optind >= argc )
	{
		fprintf(
		 stderr,
		 "Missing destination file.\n" );

		usage_fprint(
		 stdout );

		return( EXIT_FAILURE );
	}
	destination = argv[ optind ];

	if( option_sample_blocks != NULL )
	{
#if defined( HAVE_WIDE_SYSTEM_CHARACTER )
		sample_blocks = _wtoi( option_sample_blocks );
#else
		sample_blocks = atoi( option_sample_blocks );
#endif
		if( sample_blocks < 0 )
		{
			fprintf(
			 stderr,
			 "Invalid sample blocks value.\n" );

			return( EXIT_FAILURE );
		}
	}
	libcnotify_verbose_set(
	 verbose );
	libfvde_notify_set_stream(
	 stderr,
	 NULL );
	libfvde_notify_set_verbose(
	 verbose );

	if( dump_handle_initialize(
	     &fvdedump_dump_handle,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to initialize dump handle.\n" );

		goto on_error;
	}
	fvdedump_dump_handle->verbose            = verbose;
	fvdedump_dump_handle->force              = force_overwrite;
	fvdedump_dump_handle->best_metadata_only = best_metadata_only;
	fvdedump_dump_handle->compact            = compact_mode;
	fvdedump_dump_handle->sample_blocks      = sample_blocks;

	if( dump_handle_open_source(
	     fvdedump_dump_handle,
	     source,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to open source: %" PRIs_SYSTEM ".\n",
		 source );

		goto on_error;
	}
	if( dump_handle_read_volume_header(
	     fvdedump_dump_handle,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to read volume header.\n" );

		goto on_error;
	}
	if( dump_handle_read_metadata(
	     fvdedump_dump_handle,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to read metadata.\n" );

		goto on_error;
	}
	if( dump_handle_open_destination(
	     fvdedump_dump_handle,
	     destination,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to open destination: %" PRIs_SYSTEM ".\n",
		 destination );

		goto on_error;
	}
	if( dump_handle_dump(
	     fvdedump_dump_handle,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to dump FVDE metadata.\n" );

		goto on_error;
	}
	if( dump_handle_close(
	     fvdedump_dump_handle,
	     &error ) != 0 )
	{
		fprintf(
		 stderr,
		 "Unable to close dump handle.\n" );

		goto on_error;
	}
	if( dump_handle_free(
	     &fvdedump_dump_handle,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to free dump handle.\n" );

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
	if( fvdedump_dump_handle != NULL )
	{
		dump_handle_free(
		 &fvdedump_dump_handle,
		 NULL );
	}
	return( EXIT_FAILURE );
}
