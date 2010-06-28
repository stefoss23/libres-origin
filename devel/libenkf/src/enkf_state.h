#ifndef __ENKF_STATE_H__
#define __ENKF_STATE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <fortio.h>
#include <stdbool.h>
#include <enkf_types.h>
#include <enkf_node.h>
#include <enkf_util.h>
#include <ecl_file.h>
#include <meas_vector.h>
#include <enkf_fs.h>
#include <sched_file.h>
#include <hash.h>
#include <ext_joblist.h>
#include <stringlist.h>
#include <job_queue.h>
#include <enkf_serialize.h>
#include <model_config.h>
#include <site_config.h>
#include <ecl_config.h>
#include <ensemble_config.h>
#include <forward_model.h>
#include <matrix.h>
#include <log.h>
#include <ert_template.h>
#include <job_queue.h>
#include <member_config.h>
#include <subst_list.h>


typedef struct enkf_state_struct    enkf_state_type;

bool               enkf_state_get_pre_clear_runpath( const enkf_state_type * enkf_state );
void               enkf_state_set_pre_clear_runpath( enkf_state_type * enkf_state , bool pre_clear_runpath );

keep_runpath_type  enkf_state_get_keep_runpath( const enkf_state_type * enkf_state );
void               enkf_state_set_keep_runpath( enkf_state_type * enkf_state , keep_runpath_type keep_runpath);
keep_runpath_type  member_config_get_keep_runpath(const member_config_type * member_config);
void             * enkf_state_complete_forward_model__(void * arg );
job_status_type    enkf_state_get_run_status( const enkf_state_type * enkf_state );
time_t             enkf_state_get_start_time( const enkf_state_type * enkf_state );
time_t             enkf_state_get_submit_time( const enkf_state_type * enkf_state );
bool               enkf_state_resubmit_simulation( enkf_state_type * enkf_state , bool resample);
bool               enkf_state_kill_simulation( const enkf_state_type * enkf_state );
void *             enkf_state_internalize_results_mt( void * arg );
void               enkf_state_initialize(enkf_state_type * enkf_state , const stringlist_type * param_list);
void               enkf_state_fread(enkf_state_type *  , int  , int  , state_enum );
enkf_fs_type     * enkf_state_get_fs_ref(const enkf_state_type *);
bool               enkf_state_get_analyzed(const enkf_state_type * );
void               enkf_state_set_analyzed(enkf_state_type * , bool );
void               enkf_state_swapout_node(const enkf_state_type * , const char *);
void               enkf_state_swapin_node(const enkf_state_type *  , const char *);
meas_vector_type * enkf_state_get_meas_vector(const enkf_state_type *);
enkf_state_type  * enkf_state_copyc(const enkf_state_type * );
void               enkf_state_iset_eclpath(enkf_state_type * , int , const char *);
enkf_node_type   * enkf_state_get_node(const enkf_state_type * , const char * );
void               enkf_state_del_node(enkf_state_type * , const char * );
void               enkf_state_load_ecl_summary(enkf_state_type * , bool , int );
void             * enkf_state_run_eclipse__(void * );
void             * enkf_state_start_forward_model__(void * );
enkf_state_type  * enkf_state_alloc(int ,
                                    enkf_fs_type   * fs, 
                                    const char * casename , 
                                    bool         pre_clear_runpath, 
				    keep_runpath_type , 
				    const model_config_type * ,
				    ensemble_config_type * ,
				    const site_config_type * ,
				    const ecl_config_type * ,
                                    log_type * logh,
                                    ert_templates_type * templates,
                                    subst_list_type    * parent_subst);

void               enkf_state_invalidate_cache( enkf_state_type * enkf_state );
int                enkf_state_get_last_restart_nr(const enkf_state_type * enkf_state );
void               enkf_state_add_node(enkf_state_type * , const char *  , const enkf_config_node_type * );
void               enkf_state_load_ecl_restart(enkf_state_type * , bool , int );
void               enkf_state_sample(enkf_state_type * , int);
void               enkf_state_fwrite(const enkf_state_type *  , int  , int  , state_enum );
void               enkf_state_ens_read(       enkf_state_type * , const char * , int);
void               enkf_state_ecl_write(enkf_state_type *);
void               enkf_state_free(enkf_state_type * );
void               enkf_state_apply(enkf_state_type * , enkf_node_ftype1 * , int );
void               enkf_state_serialize(enkf_state_type * , size_t);
void               enkf_state_set_iens(enkf_state_type *  , int );
int                enkf_state_get_iens(const enkf_state_type * );
member_config_type *enkf_state_get_member_config(const enkf_state_type * enkf_state);
const char       * enkf_state_get_run_path(const enkf_state_type * );

//void               enkf_state_matrix_serialize(enkf_state_type * enkf_state , const char * key , const active_list_type * , matrix_type * A , int row_offset , int column);
//void               enkf_state_matrix_deserialize(enkf_state_type * enkf_state , const char * key , const active_list_type * active_list, const matrix_type * A , int row_offset , int column);

void enkf_state_printf_subst_list(enkf_state_type * enkf_state , int step1 , int step2);


/*****************************************************************/
const sched_file_type * enkf_state_get_sched_file(const enkf_state_type * enkf_state);
void enkf_state_set_inactive(enkf_state_type * state);

void enkf_state_init_run(enkf_state_type * state , 
                         run_mode_type           ,  
                         bool active             , 
                         int max_internal_submit , 
                         int init_step_parameter , 
                         state_enum init_state_parameter , 
                         state_enum init_state_dynamic , 
                         int load_start , 
                         int step1 , 
                         int step2 );
                         

bool enkf_state_runOK(const enkf_state_type * );
#ifdef __cplusplus
}
#endif
#endif
