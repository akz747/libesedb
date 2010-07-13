/*
 * Long value functions
 *
 * Copyright (c) 2009-2010, Joachim Metz <jbmetz@users.sourceforge.net>
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This software is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <byte_stream.h>
#include <memory.h>
#include <types.h>

#include <liberror.h>

#include "libesedb_data_type_definition.h"
#include "libesedb_definitions.h"
#include "libesedb_libfdata.h"
#include "libesedb_long_value.h"
#include "libesedb_values_tree.h"
#include "libesedb_values_tree_value.h"

/* Creates a long value
 * Returns 1 if successful or -1 on error
 */
int libesedb_long_value_initialize(
     libesedb_long_value_t **long_value,
     libbfio_handle_t *file_io_handle,
     libfdata_vector_t *pages_vector,
     libfdata_cache_t *pages_cache,
     libfdata_tree_t *long_values_tree,
     libfdata_cache_t *long_values_cache,
     uint8_t *long_value_key,
     size_t long_value_key_size,
     uint8_t flags,
     liberror_error_t **error )
{
	uint8_t long_value_segment_key[ 8 ];

	libesedb_internal_long_value_t *internal_long_value = NULL;
	libesedb_values_tree_value_t *values_tree_value     = NULL;
	static char *function                               = "libesedb_long_value_initialize";
	uint32_t long_value_segment_offset                  = 0;
	int result                                          = 0;

	if( long_value == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid long value.",
		 function );

		return( -1 );
	}
	if( long_value_key == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid long value key.",
		 function );

		return( -1 );
	}
	if( long_value_key_size != 4 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupport long value key size: %" PRIzd ".",
		 function,
		 long_value_key_size );

		return( -1 );
	}
	if( ( flags & ~( LIBESEDB_ITEM_FLAG_MANAGED_FILE_IO_HANDLE ) ) != 0 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported flags: 0x%02" PRIx8 ".",
		 function,
		 flags );

		return( -1 );
	}
	if( *long_value == NULL )
	{
		internal_long_value = (libesedb_internal_long_value_t *) memory_allocate(
		                                                          sizeof( libesedb_internal_long_value_t ) );

		if( internal_long_value == NULL )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_MEMORY,
			 LIBERROR_MEMORY_ERROR_INSUFFICIENT,
			 "%s: unable to create long value.",
			 function );

			return( -1 );
		}
		if( memory_set(
		     internal_long_value,
		     0,
		     sizeof( libesedb_internal_long_value_t ) ) == NULL )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_MEMORY,
			 LIBERROR_MEMORY_ERROR_SET_FAILED,
			 "%s: unable to clear long value.",
			 function );

			memory_free(
			 internal_long_value );

			return( -1 );
		}
		if( ( flags & LIBESEDB_ITEM_FLAG_MANAGED_FILE_IO_HANDLE ) == 0 )
		{
			internal_long_value->file_io_handle = file_io_handle;
		}
		else
		{
			if( libbfio_handle_clone(
			     &( internal_long_value->file_io_handle ),
			     file_io_handle,
			     error ) != 1 )
			{
				liberror_error_set(
				 error,
				 LIBERROR_ERROR_DOMAIN_RUNTIME,
				 LIBERROR_RUNTIME_ERROR_COPY_FAILED,
				 "%s: unable to copy file io handle.",
				 function );

				memory_free(
				 internal_long_value );

				return( -1 );
			}
			if( libbfio_handle_set_open_on_demand(
			     internal_long_value->file_io_handle,
			     1,
			     error ) != 1 )
			{
				liberror_error_set(
				 error,
				 LIBERROR_ERROR_DOMAIN_RUNTIME,
				 LIBERROR_RUNTIME_ERROR_COPY_FAILED,
				 "%s: unable to set open on demand in file io handle.",
				 function );

				libbfio_handle_free(
				 &( internal_long_value->file_io_handle ),
				 NULL );
				memory_free(
				 internal_long_value );

				return( -1 );
			}
		}
		if( libfdata_block_initialize(
		     &( internal_long_value->data_block ),
		     NULL,
		     NULL,
		     NULL,
		     &libfdata_block_read_segment_data,
		     0,
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to create data block.",
			 function );

			if( ( flags & LIBESEDB_ITEM_FLAG_MANAGED_FILE_IO_HANDLE ) != 0 )
			{
				libbfio_handle_free(
				 &( internal_long_value->file_io_handle ),
				 NULL );
			}
			memory_free(
			 internal_long_value );

			return( -1 );
		}
		if( libfdata_cache_initialize(
		     &( internal_long_value->data_cache ),
		     LIBESEDB_MAXIMUM_CACHE_ENTRIES_LONG_VALUES_DATA,
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to create data cache.",
			 function );

			libfdata_block_free(
			 &( internal_long_value->data_block ),
			 NULL );

			if( ( flags & LIBESEDB_ITEM_FLAG_MANAGED_FILE_IO_HANDLE ) != 0 )
			{
				libbfio_handle_free(
				 &( internal_long_value->file_io_handle ),
				 NULL );
			}
			memory_free(
			 internal_long_value );

			return( -1 );
		}
		if( libesedb_values_tree_get_value_by_key(
		     long_values_tree,
		     internal_long_value->file_io_handle,
		     long_values_cache,
		     long_value_key,
		     long_value_key_size,
		     &values_tree_value,
		     LIBESEDB_PAGE_KEY_FLAG_REVERSED_KEY,
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve values tree value.",
			 function );

			libfdata_cache_free(
			 &( internal_long_value->data_cache ),
			 NULL );
			libfdata_block_free(
			 &( internal_long_value->data_block ),
			 NULL );

			if( ( flags & LIBESEDB_ITEM_FLAG_MANAGED_FILE_IO_HANDLE ) != 0 )
			{
				libbfio_handle_free(
				 &( internal_long_value->file_io_handle ),
				 NULL );
			}
			memory_free(
			 internal_long_value );

			return( -1 );
		}
		if( libesedb_values_tree_value_read_long_value(
		     values_tree_value,
		     internal_long_value->file_io_handle,
		     pages_vector,
		     pages_cache,
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_IO,
			 LIBERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read values tree value long value.",
			 function );

			libfdata_cache_free(
			 &( internal_long_value->data_cache ),
			 NULL );
			libfdata_block_free(
			 &( internal_long_value->data_block ),
			 NULL );

			if( ( flags & LIBESEDB_ITEM_FLAG_MANAGED_FILE_IO_HANDLE ) != 0 )
			{
				libbfio_handle_free(
				 &( internal_long_value->file_io_handle ),
				 NULL );
			}
			memory_free(
			 internal_long_value );

			return( -1 );
		}
		/* Reverse the reversed-key
		 */
		long_value_segment_key[ 0 ] = long_value_key[ 3 ];
		long_value_segment_key[ 1 ] = long_value_key[ 2 ];
		long_value_segment_key[ 2 ] = long_value_key[ 1 ];
		long_value_segment_key[ 3 ] = long_value_key[ 0 ];

		do
		{
			byte_stream_copy_from_uint32_little_endian(
			 &( long_value_segment_key[ 4 ] ),
			 long_value_segment_offset );

			result = libesedb_values_tree_get_value_by_key(
			          long_values_tree,
			          internal_long_value->file_io_handle,
			          long_values_cache,
			          long_value_segment_key,
			          8,
			          &values_tree_value,
			          0,
			          error );

			if( result == -1 )
			{
				liberror_error_set(
				 error,
				 LIBERROR_ERROR_DOMAIN_RUNTIME,
				 LIBERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve values tree value.",
				 function );

				libfdata_cache_free(
				 &( internal_long_value->data_cache ),
				 NULL );
				libfdata_block_free(
				 &( internal_long_value->data_block ),
				 NULL );

				if( ( flags & LIBESEDB_ITEM_FLAG_MANAGED_FILE_IO_HANDLE ) != 0 )
				{
					libbfio_handle_free(
					 &( internal_long_value->file_io_handle ),
					 NULL );
				}
				memory_free(
				 internal_long_value );

				return( -1 );
			}
			else if( result != 0 )
			{
				if( libesedb_values_tree_value_read_long_value_segment(
				     values_tree_value,
				     internal_long_value->file_io_handle,
				     pages_vector,
				     pages_cache,
				     long_value_segment_offset,
				     internal_long_value->data_block,
				     error ) != 1 )
				{
					liberror_error_set(
					 error,
					 LIBERROR_ERROR_DOMAIN_IO,
					 LIBERROR_IO_ERROR_READ_FAILED,
					 "%s: unable to read values tree value long value.",
					 function );

					libfdata_cache_free(
					 &( internal_long_value->data_cache ),
					 NULL );
					libfdata_block_free(
					 &( internal_long_value->data_block ),
					 NULL );

					if( ( flags & LIBESEDB_ITEM_FLAG_MANAGED_FILE_IO_HANDLE ) != 0 )
					{
						libbfio_handle_free(
						 &( internal_long_value->file_io_handle ),
						 NULL );
					}
					memory_free(
					 internal_long_value );

					return( -1 );
				}
				long_value_segment_offset += values_tree_value->data_size;
			}
		}
		while( result == 1 );

		internal_long_value->flags = flags;

		*long_value = (libesedb_long_value_t *) internal_long_value;
	}
	return( 1 );
}

/* Frees the long value
 * Returns 1 if successful or -1 on error
 */
int libesedb_long_value_free(
     libesedb_long_value_t **long_value,
     liberror_error_t **error )
{
	libesedb_internal_long_value_t *internal_long_value = NULL;
	static char *function                               = "libesedb_long_value_free";
	int result                                          = 1;

	if( long_value == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid long value.",
		 function );

		return( -1 );
	}
	if( *long_value != NULL )
	{
		internal_long_value = (libesedb_internal_long_value_t *) *long_value;
		*long_value         = NULL;

		if( ( internal_long_value->flags & LIBESEDB_ITEM_FLAG_MANAGED_FILE_IO_HANDLE ) != 0 )
		{
			if( internal_long_value->file_io_handle != NULL )
			{
				if( libbfio_handle_close(
				     internal_long_value->file_io_handle,
				     error ) != 0 )
				{
					liberror_error_set(
					 error,
					 LIBERROR_ERROR_DOMAIN_IO,
					 LIBERROR_IO_ERROR_CLOSE_FAILED,
					 "%s: unable to close file io handle.",
					 function );

					result = -1;
				}
				if( libbfio_handle_free(
				     &( internal_long_value->file_io_handle ),
				     error ) != 1 )
				{
					liberror_error_set(
					 error,
					 LIBERROR_ERROR_DOMAIN_RUNTIME,
					 LIBERROR_RUNTIME_ERROR_FINALIZE_FAILED,
					 "%s: unable to free file io handle.",
					 function );

					result = -1;
				}
			}
		}
		if( libfdata_block_free(
		     &( internal_long_value->data_block ),
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free data block.",
			 function );

			result = -1;
		}
		if( libfdata_cache_free(
		     &( internal_long_value->data_cache ),
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free data cache.",
			 function );

			result = -1;
		}
		memory_free(
		 internal_long_value );
	}
	return( result );
}

/* Retrieve the number of data segments
 * Return 1 if successful or -1 on error
 */
int libesedb_long_value_get_number_of_segments(
     libesedb_long_value_t *long_value,
     int *number_of_segments,
     liberror_error_t **error )
{
	libesedb_internal_long_value_t *internal_long_value = NULL;
	static char *function                               = "libesedb_long_value_get_number_of_segments";

	if( long_value == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid long value.",
		 function );

		return( -1 );
	}
	internal_long_value = (libesedb_internal_long_value_t *) long_value;

	if( libfdata_block_get_number_of_segments(
	     internal_long_value->data_block,
	     number_of_segments,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of segments.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Retrieve the segment data
 * Return 1 if successful or -1 on error
 */
int libesedb_long_value_get_segment_data(
     libesedb_long_value_t *long_value,
     int data_segment_index,
     uint8_t **segment_data,
     size_t *segment_data_size,
     liberror_error_t **error )
{
	libesedb_internal_long_value_t *internal_long_value = NULL;
	static char *function                               = "libesedb_long_value_get_segment_data";

	if( long_value == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid long value.",
		 function );

		return( -1 );
	}
	internal_long_value = (libesedb_internal_long_value_t *) long_value;

	if( libfdata_block_get_segment_data(
	     internal_long_value->data_block,
	     internal_long_value->file_io_handle,
	     internal_long_value->data_cache,
	     data_segment_index,
	     segment_data,
	     segment_data_size,
	     0,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve segment data.",
		 function );

		return( -1 );
	}
	return( 1 );
}

