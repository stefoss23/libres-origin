/*
   Copyright (C) 2012  Statoil ASA, Norway. 
    
   The file 'enkf_plot_data.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/
#include <time.h>
#include <stdbool.h>

#include <ert/util/double_vector.h>
#include <ert/util/vector.h>
#include <ert/util/thread_pool.h>
#include <ert/util/type_macros.h>

#include <ert/enkf/enkf_fs.h>
#include <ert/enkf/enkf_plot_tvector.h>
#include <ert/enkf/enkf_plot_data.h>


#define ENKF_PLOT_DATA_TYPE_ID 3331063

struct enkf_plot_data_struct {
  UTIL_TYPE_ID_DECLARATION;
  const enkf_config_node_type * config_node;
  int                       alloc_size;
  int                       size;
  enkf_plot_tvector_type ** ensemble;
};



static void enkf_plot_data_resize( enkf_plot_data_type * plot_data , int new_size ) {
  plot_data->ensemble = util_realloc( plot_data->ensemble , sizeof * plot_data->ensemble * new_size );
  {
    int iens;
    for (iens = plot_data->alloc_size; iens < new_size; iens++) 
      plot_data->ensemble[iens] = enkf_plot_tvector_alloc( );
  }
  plot_data->alloc_size = new_size;
}


void enkf_plot_data_free( enkf_plot_data_type * plot_data ) {
  int iens;
  for (iens = 0; iens < plot_data->alloc_size; iens++) {
    if ( plot_data->ensemble[iens] != NULL)
      enkf_plot_tvector_free( plot_data->ensemble[iens] );
  }
  
  free( plot_data );
}

UTIL_IS_INSTANCE_FUNCTION( enkf_plot_data , ENKF_PLOT_DATA_TYPE_ID )


enkf_plot_data_type * enkf_plot_data_alloc( const enkf_config_node_type * config_node ) {
  enkf_plot_data_type * plot_data = util_malloc( sizeof * plot_data);
  UTIL_TYPE_ID_INIT( plot_data , ENKF_PLOT_DATA_TYPE_ID );
  plot_data->size        = 0;
  plot_data->config_node = config_node;
  plot_data->alloc_size  = 0;
  plot_data->ensemble    = NULL;
  enkf_plot_data_resize( plot_data , 32 );
  
  return plot_data;
}



void * enkf_plot_data_load__( void *arg ) {
  arg_pack_type * arg_pack            = arg_pack_safe_cast( arg );
  enkf_plot_data_type * plot_data     = arg_pack_iget_ptr( arg_pack , 0 );
  enkf_config_node_type * config_node = arg_pack_iget_ptr( arg_pack , 1 );
  enkf_fs_type * fs                   = arg_pack_iget_ptr( arg_pack , 2 );
  const char * user_key               = arg_pack_iget_const_ptr( arg_pack , 3 );
  state_enum state                    = arg_pack_iget_int( arg_pack , 4 );
  const int_vector_type * iens_list   = arg_pack_iget_ptr( arg_pack , 5 );         
  int index1                          = arg_pack_iget_int( arg_pack , 6 );
  int index2                          = arg_pack_iget_int( arg_pack , 7 );

  enkf_node_type * enkf_node = enkf_node_alloc( config_node ); // Shared node used for all loading.
  
  for (int index = index1; index < index2; index++) {
    int iens = int_vector_iget( iens_list , index );
    if (iens >= plot_data->alloc_size)
      // This must be protected by a lock of some sort ....
      enkf_plot_data_resize( plot_data , 2*iens + 1);
    
    {
      enkf_plot_tvector_type * plot_tvector = plot_data->ensemble[iens];
      
      enkf_plot_tvector_safe_cast( plot_tvector );
      enkf_plot_tvector_load( plot_tvector , enkf_node , fs , user_key , iens , state );
      
    }
  }

  enkf_node_free( enkf_node );
  return NULL;
}




void enkf_plot_data_load( enkf_plot_data_type * plot_data , 
                          enkf_config_node_type * config_node , 
                          enkf_fs_type * fs , 
                          const char * user_key , 
                          state_enum state , 
                          const bool_vector_type * active) {
  int iens;
  int_vector_type * iens_list = int_vector_alloc( 0 , 0 );
  {
    int ens_size = bool_vector_size( active );

    for (iens = 0; iens < ens_size; iens++) {
      if (bool_vector_iget( active , iens ))
        int_vector_append( iens_list , iens );
    }
  }
  
  
  {
    int active_size = int_vector_size( iens_list );
    int num_threads = 4;
    int block_size  = active_size / num_threads;
    arg_pack_type ** arg_list = util_calloc( num_threads , sizeof * arg_list );
    thread_pool_type * tp = thread_pool_alloc( num_threads , true );
  
    if (block_size == 0)  /* Fewer tasks than threads */
      block_size = 1;
  
    for (int i=0; i < num_threads; i++) {
      arg_list[i] = arg_pack_alloc();

      arg_pack_append_ptr( arg_list[i] , plot_data );
      arg_pack_append_ptr( arg_list[i] , config_node );
      arg_pack_append_ptr( arg_list[i] , fs );
      arg_pack_append_const_ptr( arg_list[i] , user_key );
      
      arg_pack_append_int( arg_list[i] , state );

      {
        int index1 = i * block_size;
        int index2 = index1 + block_size;
        
        if (index1 < active_size) {
          if (index2 > active_size)
            index2 = active_size;
        }  
        
        arg_pack_append_ptr( arg_list[i] , iens_list );
        arg_pack_append_int( arg_list[i] , index1 );
        arg_pack_append_int( arg_list[i] , index2 );
      }
      
      thread_pool_add_job(tp , enkf_plot_data_load__ , arg_list[i]);
    }
    
    thread_pool_join( tp );
    thread_pool_free( tp );
    for (int i=0; i < num_threads; i++) 
      arg_pack_free( arg_list[i] );
    free( arg_list );
  }
  int_vector_free( iens_list );
}
  
