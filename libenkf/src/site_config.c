/*
   Copyright (C) 2011  Statoil ASA, Norway.

   The file 'site_config.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ert/util/util.h>
#include <ert/util/stringlist.h>
#include <ert/util/vector.h>

#include <ert/job_queue/job_queue.h>
#include <ert/job_queue/ext_job.h>
#include <ert/job_queue/ext_joblist.h>
#include <ert/job_queue/lsf_driver.h>
#include <ert/job_queue/rsh_driver.h>
#include <ert/job_queue/local_driver.h>
#include <ert/job_queue/queue_driver.h>

#include <ert/config/config_parser.h>
#include <ert/config/config_content_item.h>
#include <ert/config/config_content_node.h>
#include <ert/config/config_schema_item.h>

#include <ert/enkf/site_config.h>
#include <ert/enkf/queue_config.h>
#include <ert/enkf/enkf_defaults.h>
#include <ert/enkf/config_keys.h>
#include <ert/enkf/ert_workflow_list.h>

/**
   This struct contains information which is specific to the site
   where this enkf instance is running. Pointers to the fields in this
   structure are passed on to e.g. the enkf_state->shared_info object,
   but this struct is the *OWNER* of this information, and hence
   responsible for booting and deleting these objects.

   The settings held by the site_config object are by default set in
   the site-wide configuration file, but they can also be overridden
   in the users configuration file. This makes both parsing,
   validating and also storing the configuration information a bit
   more tricky:

   Parsing:
   --------
   When parsing the user configuration file all settings are optional,
   that means that the required validation of the config system, can
   not be used, instead every get must be preceeded by:

      if (config_content_has_item(config , KEY)) ...

   Furthermore everything is done twice; first with config as a
   site-config instance, and later as user-config instance.


   Saving:
   -------
   A setting which originates from the site_config file should not be
   stored in the user's config file, but additions/overrides from the
   user's config file should of course be saved. This is 'solved' with
   many fields having a xxx_site duplicate, where the xxx_site is only
   updated during the initial parsing of the site-config file; when
   the flag user_mode is set to true the xxx_site fields are not
   updated. When saving only fields which are different from their
   xxx_site counterpart are stored.
 */

struct site_config_struct {
  ext_joblist_type * joblist; /* The list of external jobs which have been installed.
                                                     These jobs will be the parts of the forward model. */
  hash_type * env_variables_user; /* The environment variables set in the user config file. */
  hash_type * env_variables_site; /* The environment variables set in site_config file - not exported. */

  mode_t umask;

  char * license_root_path; /* The license_root_path value set by the user. */
  char * license_root_path_site; /* The license_root_path value set by the site. */
  char * __license_root_path; /* The license_root_path value actually used - includes a user/pid subdirectory. */

  hash_type * path_variables_site; /* We store this so we can roll back when all user settings are cleared. */
  stringlist_type * path_variables_user; /* We can update the same path variable several times - i.e. it can not be a hash table. */
  stringlist_type * path_values_user;
   
  queue_config_type * queue_config;    
  job_queue_type * job_queue; /* The queue instance which will run the external jobs. */

  char * manual_url;    //not this one
  char * default_browser;   //not this one
  
  bool user_mode;
  bool search_path;
};

void site_config_set_umask(site_config_type * site_config, mode_t new_mask) {
  umask(new_mask);
  site_config->umask = new_mask;
}

mode_t site_config_get_umask(const site_config_type * site_config) {
  return site_config->umask;
}

bool site_config_has_queue_driver(const site_config_type * site_config, const char * driver_name) {
  return queue_config_has_queue_driver(site_config->queue_config, driver_name);
}

queue_driver_type * site_config_get_queue_driver(const site_config_type * site_config, const char * driver_name) {
  return queue_config_get_queue_driver(site_config->queue_config, driver_name);
}

static void site_config_set_queue_option(site_config_type * site_config, const char * driver_name, const char * option_key, const char * option_value) {
  if (site_config_has_queue_driver(site_config, driver_name)) {
    queue_driver_type * driver = site_config_get_queue_driver(site_config, driver_name);
    if (!queue_driver_set_option(driver, option_key, option_value))
      fprintf(stderr, "** Warning: Option:%s or its value is not recognized by driver:%s- ignored \n", option_key, driver_name);
  } else
    fprintf(stderr, "** Warning: Driver:%s not recognized - ignored \n", driver_name);
}

/**
   This site_config object is not really ready for prime time.
 */
site_config_type * site_config_alloc_empty() {
  site_config_type * site_config = util_malloc(sizeof * site_config);
   
  site_config->queue_config = queue_config_alloc();

  site_config->joblist = ext_joblist_alloc();
 
  site_config->license_root_path = NULL;
  site_config->license_root_path_site = NULL;
  site_config->__license_root_path = NULL;
  site_config->manual_url = NULL;
  site_config->default_browser = NULL;
  site_config->user_mode = false;  

  site_config->job_queue = job_queue_alloc(DEFAULT_MAX_SUBMIT, "OK", "STATUS", "ERROR");
  site_config->env_variables_user = hash_alloc();
  site_config->env_variables_site = hash_alloc();

  site_config->path_variables_user = stringlist_alloc_new();
  site_config->path_values_user = stringlist_alloc_new();
  site_config->path_variables_site = hash_alloc();

  /* Some hooops to get the current umask. */
  site_config->umask = umask(0);
  site_config_set_umask(site_config, site_config->umask);
  site_config_set_manual_url(site_config, DEFAULT_MANUAL_URL);
  site_config_set_default_browser(site_config, DEFAULT_BROWSER);
  site_config_set_max_submit(site_config, DEFAULT_MAX_SUBMIT);
  site_config->search_path = false;
  return site_config;
}

const char * site_config_get_license_root_path(const site_config_type * site_config) {
  return site_config->license_root_path;
}

/**
   Observe that this variable can not "really" be set to different
   values during a simulation, when creating ext_job instances they
   will store a pointer to this variable on creation, if the variable
   is later changed they will be left with a dangling copy. That is
   not particularly elegant, however it should nonetheless work.
 */

void site_config_set_license_root_path(site_config_type * site_config, const char * license_root_path) {
  util_make_path(license_root_path);
  {
    char * full_license_root_path = util_alloc_realpath(license_root_path);
    {
      /**
         Appending /user/pid to the license root path. Everything
         including the pid is removed when exiting (gracefully ...).

         Dangling license directories after a crash can just be removed.
       */
      site_config->license_root_path = util_realloc_string_copy(site_config->license_root_path, full_license_root_path);
      site_config->__license_root_path = util_realloc_sprintf(site_config->__license_root_path, "%s%c%s%c%d", full_license_root_path, UTIL_PATH_SEP_CHAR, getenv("USER"), UTIL_PATH_SEP_CHAR, getpid());

      if (!site_config->user_mode)
        site_config->license_root_path_site = util_realloc_string_copy(site_config->license_root_path_site, full_license_root_path);
    }
    free(full_license_root_path);
  }
}

void site_config_init_user_mode(site_config_type * site_config) {
  queue_config_init_user_mode(site_config->queue_config);
  site_config->user_mode = true;
}

/**
   Will return 0 if the job is added correctly, and a non-zero (not
   documented ...) error code if the job is not added.
 */

int site_config_install_job(site_config_type * site_config, const char * job_name, const char * install_file) {
  ext_job_type * new_job = ext_job_fscanf_alloc(job_name, site_config->__license_root_path, site_config->user_mode, install_file, site_config->search_path);
  if (new_job != NULL) {
    ext_joblist_add_job(site_config->joblist, job_name, new_job);
    return 0;
  } else
    return 1; /* Some undocumented error condition - the job is NOT added. */
}

/**
   Will NOT remove shared jobs.
 */
bool site_config_del_job(site_config_type * site_config, const char * job_name) {
  return ext_joblist_del_job(site_config->joblist, job_name);
}

static void site_config_add_jobs(site_config_type * site_config, const config_content_type * config) {
  if (config_content_has_item(config, INSTALL_JOB_KEY)) {
    const config_content_item_type * content_item = config_content_get_item(config, INSTALL_JOB_KEY);
    int num_jobs = config_content_item_get_size(content_item);
    for (int job_nr = 0; job_nr < num_jobs; job_nr++) {
      config_content_node_type * node = config_content_item_iget_node(content_item, job_nr);
      const char * job_key = config_content_node_iget(node, 0);
      const char * description_file = config_content_node_iget_as_abspath(node, 1);

      site_config_install_job(site_config, job_key, description_file);
    }
  }
  if (config_content_has_item(config, INSTALL_JOB_DIRECTORY_KEY)) {
    const config_content_item_type * content_item = config_content_get_item(config, INSTALL_JOB_DIRECTORY_KEY);
    int num_dirs = config_content_item_get_size(content_item);
    for (int dir_nr = 0; dir_nr < num_dirs; dir_nr++) {
      config_content_node_type * node = config_content_item_iget_node(content_item, dir_nr);
      const char * directory = config_content_node_iget_as_abspath(node, 0);

      ext_joblist_add_jobs_in_directory(site_config->joblist  , directory, site_config->__license_root_path, site_config->user_mode, site_config->search_path );
    }
  }

}

hash_type * site_config_get_env_hash(const site_config_type * site_config) {
  return site_config->env_variables_user;
}

/**
   Will only return the user-set variables. The variables set in the
   site config are hidden.
 */

stringlist_type * site_config_get_path_variables(const site_config_type * site_config) {
  return site_config->path_variables_user;
}

stringlist_type * site_config_get_path_values(const site_config_type * site_config) {
  return site_config->path_values_user;
}

/**
   Observe that the value inserted in the internal hash tables is the
   interpolated value returned from util_interp_setenv(), where $VAR
   expressions have been expanded.
 */

void site_config_setenv(site_config_type * site_config, const char * variable, const char * __value) {
  const char * value = util_interp_setenv(variable, __value);

  if (site_config->user_mode) {
    /* In the table meant for user-export we store the literal $var strings. */
    hash_insert_hash_owned_ref(site_config->env_variables_user, variable, util_alloc_string_copy(__value), free);

    if (!hash_has_key(site_config->env_variables_site, variable))
      hash_insert_ref(site_config->env_variables_site, variable, NULL); /* We insert a NULL so we can recover a unsetenv() in _clear_env(). */
  } else
    hash_insert_hash_owned_ref(site_config->env_variables_site, variable, util_alloc_string_copy(value), free);
}

/**
   Clears all the environment variables set by the user. This is done
   is follows:

     1. Iterate through the table config->env_variables_user and call
        unsetenv() on all of them

     2. Iterate through the table config->env_variables_site and call
        setenv() on all of them.

   This way the environment should be identical to what it is after
   the site parsing is completed.
 */


void site_config_clear_env(site_config_type * site_config) {
  /* 1: Clearing the user_set variables. */
  {
    hash_iter_type * hash_iter = hash_iter_alloc(site_config->env_variables_user);
    while (!hash_iter_is_complete(hash_iter)) {
      const char * var = hash_iter_get_next_key(hash_iter);
      util_unsetenv(var);
    }
    hash_iter_free(hash_iter);
    hash_clear(site_config->env_variables_user);
  }


  /* 2: Recovering the site_set variables. */
  {
    hash_iter_type * hash_iter = hash_iter_alloc(site_config->env_variables_site);
    while (!hash_iter_is_complete(hash_iter)) {
      const char * var = hash_iter_get_next_key(hash_iter);
      const char * value = hash_get(site_config->env_variables_site, var);
      util_interp_setenv(var, value); /* Will call unsetenv if value == NULL */
    }
    hash_iter_free(hash_iter);
  }
}

void site_config_clear_pathvar(site_config_type * site_config) {
  stringlist_clear(site_config->path_variables_user);
  stringlist_clear(site_config->path_values_user);
  {
    /* Recover the original values. */
    hash_iter_type * hash_iter = hash_iter_alloc(site_config->path_variables_site);
    while (!hash_iter_is_complete(hash_iter)) {
      const char * var = hash_iter_get_next_key(hash_iter);
      const char * site_value = hash_get(site_config->path_variables_site, var);

      if (site_value == NULL)
        util_unsetenv(var);
      else
        util_setenv(var, site_value);
    }
  }
}

void site_config_update_pathvar(site_config_type * site_config, const char * pathvar, const char * value) {
  if (site_config->user_mode) {
    stringlist_append_copy(site_config->path_variables_user, pathvar);
    stringlist_append_copy(site_config->path_values_user, value);

    if (!hash_has_key(site_config->path_variables_site, pathvar))
      hash_insert_ref(site_config->path_variables_site, pathvar, NULL); /* This path variable has not been touched in the
                                                                              site_config. We store a NULL, so can roll back
                                                                              (i.e. call unsetenv()). */
  }
  util_update_path_var(pathvar, value, false);
}

static void site_config_select_job_driver(site_config_type * site_config, const char * driver_name) {
  queue_driver_type * driver = site_config_get_queue_driver(site_config, driver_name); 
  job_queue_set_driver(site_config->job_queue, driver);
}

/**
   These functions can be called repeatedly if you should want to
   change driver characteristics run-time.
 */
static void site_config_select_LOCAL_job_queue(site_config_type * site_config) {
  site_config_select_job_driver(site_config, LOCAL_DRIVER_NAME);
}

static void site_config_select_RSH_job_queue(site_config_type * site_config) {
  site_config_select_job_driver(site_config, RSH_DRIVER_NAME);
}

static void site_config_select_LSF_job_queue(site_config_type * site_config) {
  site_config_select_job_driver(site_config, LSF_DRIVER_NAME);
}

static void site_config_select_TORQUE_job_queue(site_config_type * site_config) {
  site_config_select_job_driver(site_config, TORQUE_DRIVER_NAME);
}

/*****************************************************************/

/*****************************************************************/
static int site_config_get_queue_max_running_option(queue_driver_type * driver) {
  const char * max_running_string = queue_driver_get_option(driver, MAX_RUNNING);
  int max_running = 0;
  if(!util_sscanf_int(max_running_string, &max_running)) {
    fprintf(stderr, "** Warning: String:%s for max_running is not parsable as int, using 0\n", max_running_string);
  }
  return max_running;
}


static void site_config_set_queue_max_running_option(site_config_type * site_config, const char* driver_name, int max_running) {
  char* max_running_string = util_alloc_sprintf("%d", max_running);
  site_config_set_queue_option(site_config, driver_name, MAX_RUNNING, max_running_string);
  free(max_running_string);
}

void site_config_set_max_running_lsf(site_config_type * site_config, int max_running_lsf) {
  site_config_set_queue_max_running_option(site_config, LSF_DRIVER_NAME, max_running_lsf);
}

int site_config_get_max_running_lsf(const site_config_type * site_config) {
  queue_driver_type * lsf_driver = site_config_get_queue_driver(site_config, LSF_DRIVER_NAME);
  return site_config_get_queue_max_running_option(lsf_driver);
}

void site_config_set_max_running_rsh(site_config_type * site_config, int max_running_rsh) {
  site_config_set_queue_max_running_option(site_config, RSH_DRIVER_NAME, max_running_rsh);  
}

int site_config_get_max_running_rsh(const site_config_type * site_config) {
  queue_driver_type * rsh_driver = site_config_get_queue_driver(site_config, RSH_DRIVER_NAME);
  return site_config_get_queue_max_running_option(rsh_driver);
}

void site_config_set_max_running_local(site_config_type * site_config, int max_running_local) {
  site_config_set_queue_max_running_option(site_config, LOCAL_DRIVER_NAME, max_running_local);
}

int site_config_get_max_running_local(const site_config_type * site_config) {
  queue_driver_type * local_driver = site_config_get_queue_driver(site_config, LOCAL_DRIVER_NAME);
  return site_config_get_queue_max_running_option(local_driver);
}

/*****************************************************************/

/*****************************************************************/

void site_config_clear_rsh_host_list(site_config_type * site_config) {
  queue_driver_type * rsh_driver = site_config_get_queue_driver(site_config, RSH_DRIVER_NAME);
  queue_driver_set_option(rsh_driver, RSH_CLEAR_HOSTLIST, NULL);
}

hash_type * site_config_get_rsh_host_list(const site_config_type * site_config) {
  queue_driver_type * rsh_driver = site_config_get_queue_driver(site_config, RSH_DRIVER_NAME);
  return (hash_type *) queue_driver_get_option(rsh_driver, RSH_HOSTLIST);
}

void site_config_add_rsh_host_from_string(site_config_type * site_config, const char * host_string) {
  queue_driver_type * rsh_driver = site_config_get_queue_driver(site_config, RSH_DRIVER_NAME);
  queue_driver_set_option(rsh_driver, RSH_HOST, host_string);
}

void site_config_add_rsh_host(site_config_type * site_config, const char * rsh_host, int max_running) {
  char * host_max_running = util_alloc_sprintf("%s:%d", rsh_host, max_running);
  site_config_add_rsh_host_from_string(site_config, host_max_running);
  free(host_max_running);
}

void site_config_set_rsh_command(site_config_type * site_config, const char * rsh_command) {
  queue_driver_type * rsh_driver = site_config_get_queue_driver(site_config, RSH_DRIVER_NAME);
  queue_driver_set_option(rsh_driver, RSH_CMD, rsh_command);
}

const char * site_config_get_rsh_command(const site_config_type * site_config) {
  queue_driver_type * rsh_driver = site_config_get_queue_driver(site_config, RSH_DRIVER_NAME);
  return queue_driver_get_option(rsh_driver, RSH_CMD);
}

/*****************************************************************/

void site_config_set_lsf_queue(site_config_type * site_config, const char * lsf_queue) {
  queue_driver_type * lsf_driver = site_config_get_queue_driver(site_config, LSF_DRIVER_NAME);
  queue_driver_set_option(lsf_driver, LSF_QUEUE, lsf_queue);
}

const char * site_config_get_lsf_queue(const site_config_type * site_config) {
  queue_driver_type * lsf_driver = site_config_get_queue_driver(site_config, LSF_DRIVER_NAME);
  return queue_driver_get_option(lsf_driver, LSF_QUEUE);
}

void site_config_set_lsf_server(site_config_type * site_config, const char * lsf_server) {
  queue_driver_type * lsf_driver = site_config_get_queue_driver(site_config, LSF_DRIVER_NAME);
  queue_driver_set_option(lsf_driver, LSF_SERVER, lsf_server);
}

void site_config_set_lsf_request(site_config_type * site_config, const char * lsf_request) {
  queue_driver_type * lsf_driver = site_config_get_queue_driver(site_config, LSF_DRIVER_NAME);
  queue_driver_set_option(lsf_driver, LSF_RESOURCE, lsf_request);
}

const char * site_config_get_lsf_request(const site_config_type * site_config) {
  queue_driver_type * lsf_driver = site_config_get_queue_driver(site_config, LSF_DRIVER_NAME);
  return queue_driver_get_option(lsf_driver, LSF_RESOURCE);
}

/*****************************************************************/


const char * site_config_get_queue_name(const site_config_type * site_config) {
  return queue_config_get_queue_name(site_config->queue_config); //based on driver_type
}

static void site_config_set_job_queue__(site_config_type * site_config, job_driver_type driver_type) {
  if (site_config->job_queue != NULL) {
    switch (driver_type) {
      case(LSF_DRIVER):
        site_config_select_LSF_job_queue(site_config);
        break;
      case(TORQUE_DRIVER):
        site_config_select_TORQUE_job_queue(site_config);
        break;
      case(RSH_DRIVER):
        site_config_select_RSH_job_queue(site_config);
        break;
      case(LOCAL_DRIVER):
        site_config_select_LOCAL_job_queue(site_config);
        break;
      default:
        util_abort("%s: internal error \n", __func__);
    }
  }
}

bool site_config_queue_is_running(const site_config_type * site_config) {
  return job_queue_is_running(site_config->job_queue);
}

/**
   The job_script might be a relative path, and the cwd changes during
   execution, i.e. it is essential to get hold of the full path.
*/

bool site_config_set_job_script(site_config_type * site_config, const char * job_script) {
  return queue_config_set_job_script(site_config->queue_config, job_script);
}


bool site_config_has_job_script( const site_config_type * site_config ) {
  return queue_config_has_job_script(site_config->queue_config);
}


const char * site_config_get_job_script(const site_config_type * site_config) {
  return queue_config_get_job_script(site_config->queue_config);
}

const char * site_config_get_manual_url(const site_config_type * site_config) {
  return site_config->manual_url;
}

void site_config_set_manual_url(site_config_type * site_config, const char * manual_url) {
  site_config->manual_url = util_realloc_string_copy(site_config->manual_url, manual_url);
}

const char * site_config_get_default_browser(const site_config_type * site_config) {
  return site_config->default_browser;
}

void site_config_set_default_browser(site_config_type * site_config, const char * default_browser) {
  site_config->default_browser = util_realloc_string_copy(site_config->default_browser, default_browser);
}


void site_config_set_max_submit(site_config_type * site_config, int max_submit) {  
  job_queue_set_max_submit(site_config->job_queue, max_submit);
}

int site_config_get_max_submit(const site_config_type * site_config) {
  return job_queue_get_max_submit(site_config->job_queue);
}

static void site_config_install_job_queue(site_config_type * site_config) {
  /*
     All the various driver options are set, unconditionally of which
     driver is actually selected in the end.
   */
  job_driver_type driver = queue_config_get_driver_type(site_config->queue_config);
  if (driver != NULL_DRIVER)
    site_config_set_job_queue__(site_config, driver);
}

void site_config_init_env(site_config_type * site_config, const config_content_type * config) {
  {
    if (config_content_has_item( config , SETENV_KEY)) {
      config_content_item_type * setenv_item = config_content_get_item(config, SETENV_KEY);
      int i;
      for (i = 0; i < config_content_item_get_size(setenv_item); i++) {
        const config_content_node_type * setenv_node = config_content_item_iget_node(setenv_item, i);
        const char * var = config_content_node_iget(setenv_node, 0);
        const char * value = config_content_node_iget(setenv_node, 1);

        site_config_setenv(site_config, var, value);
      }
    }
  }

  {
    if (config_content_has_item( config , UPDATE_PATH_KEY)) {
      config_content_item_type * path_item = config_content_get_item(config, UPDATE_PATH_KEY);
      int i;
      for (i = 0; i < config_content_item_get_size(path_item); i++) {
        const config_content_node_type * path_node = config_content_item_iget_node(path_item, i);
        const char * path = config_content_node_iget(path_node, 0);
        const char * value = config_content_node_iget(path_node, 1);

        site_config_update_pathvar(site_config, path, value);
      }
    }
  }
}

/**
   This function will be called twice, first when the config instance
   is an internalization of the site-wide configuration file, and
   secondly when config is an internalisation of the user's
   configuration file. The @user_config parameter will be true in the
   latter case.
 */


bool site_config_init(site_config_type * site_config, const config_content_type * config) {
  
  queue_config_init(site_config->queue_config, config);

  site_config_add_jobs(site_config, config);
  site_config_init_env(site_config, config);

  /*
     When LSF is used several enviroment variables must be set (by the
     site wide file) - i.e.  the calls to SETENV must come first.
   */
  


  /*
     Set the umask for all file creation. A value of '0' will ensure
     that all files and directories are created with 'equal rights'
     for everyone - might be handy if you are helping someone... The
     default statoil value is 0022, i.e. write access is removed from
     group and others.

     The string is supposed to be in OCTAL representation (without any
     prefix characters).
   */

  if (config_content_has_item(config, UMASK_KEY)) {
    const char * string_mask = config_content_get_value(config, UMASK_KEY);
    mode_t umask_value;
    if (util_sscanf_octal_int(string_mask, &umask_value))
      site_config_set_umask(site_config, umask_value);
    else
      util_abort("%s: failed to parse:\"%s\" as a valid octal literal \n", __func__, string_mask);
  }

  if (config_content_has_item(config, MAX_SUBMIT_KEY)) 
    site_config_set_max_submit(site_config, config_content_get_value_as_int(config, MAX_SUBMIT_KEY));
  


  

  // Setting driver_type 
  if (config_content_has_item(config, QUEUE_SYSTEM_KEY)) {
    job_driver_type driver_type;
    {
      const char * queue_system = config_content_get_value(config, QUEUE_SYSTEM_KEY);
      if (strcmp(queue_system, LSF_DRIVER_NAME) == 0) {
        driver_type = LSF_DRIVER;
      } else if (strcmp(queue_system, RSH_DRIVER_NAME) == 0)
        driver_type = RSH_DRIVER;
      else if (strcmp(queue_system, LOCAL_DRIVER_NAME) == 0)
        driver_type = LOCAL_DRIVER;
      else if (strcmp(queue_system, TORQUE_DRIVER_NAME) == 0)
        driver_type = TORQUE_DRIVER;
      else {
        util_abort("%s: queue system :%s not recognized \n", __func__, queue_system);
        driver_type = NULL_DRIVER;
      }
    }
    //create a pointer to the driver_in_use
    site_config_set_job_queue__(site_config, driver_type);
  }

  if (config_content_has_item(config, LICENSE_PATH_KEY))
  site_config_set_license_root_path(site_config, config_content_get_value_as_abspath(config, LICENSE_PATH_KEY));

  site_config_install_job_queue(site_config);

  if (config_content_has_item(config, EXT_JOB_SEARCH_PATH_KEY)){
      site_config_set_ext_job_search_path(site_config, config_content_get_value_as_bool(config, EXT_JOB_SEARCH_PATH_KEY));
  }


  /* Setting QUEUE_OPTIONS */
  {
    int i;
    for (i = 0; i < config_content_get_occurences(config, QUEUE_OPTION_KEY); i++) {
      const stringlist_type * tokens = config_content_iget_stringlist_ref(config, QUEUE_OPTION_KEY, i);
      const char * driver_name = stringlist_iget(tokens, 0);
      const char * option_key = stringlist_iget(tokens, 1);
      char * option_value = stringlist_alloc_joined_substring(tokens, 2, stringlist_get_size(tokens), " ");
      /*
         If it is desirable to keep the exact number of spaces in the
         option_value it should be quoted with "" in the configuration
         file.
       */
      site_config_set_queue_option(site_config, driver_name, option_key, option_value);
      free( option_value );
    }
  }
  return true;
}

void site_config_set_ext_job_search_path(site_config_type * site_config, bool search_path){
    site_config->search_path = search_path;
}


void site_config_free(site_config_type * site_config) {
  ext_joblist_free(site_config->joblist);
  job_queue_free(site_config->job_queue);

  stringlist_free(site_config->path_variables_user);
  stringlist_free(site_config->path_values_user);
  hash_free(site_config->path_variables_site);

  hash_free(site_config->env_variables_site);
  hash_free(site_config->env_variables_user);

  if (site_config->__license_root_path != NULL)
    util_clear_directory(site_config->__license_root_path, true, true);

  util_safe_free(site_config->manual_url);
  util_safe_free(site_config->default_browser);
  util_safe_free(site_config->license_root_path);
  util_safe_free(site_config->license_root_path_site);
  util_safe_free(site_config->__license_root_path);

  queue_config_free(site_config->queue_config);
  free(site_config);
}

ext_joblist_type * site_config_get_installed_jobs(const site_config_type * site_config) {
  return site_config->joblist;
}

job_queue_type * site_config_get_job_queue(const site_config_type * site_config) {
  return site_config->job_queue;
}

void site_config_set_ens_size(site_config_type * site_config, int ens_size) {
  //job_queue_set_size( site_config->job_queue , ens_size );
}

/*****************************************************************/


void site_config_add_queue_config_items(config_parser_type * config, bool site_mode) {
  queue_config_add_config_items(config, site_mode);

  config_schema_item_type * item = config_add_schema_item(config, MAX_SUBMIT_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, 1);
  config_schema_item_iset_type(item, 0, CONFIG_INT);
}

void site_config_add_config_items(config_parser_type * config, bool site_mode) {
  queue_config_add_config_items(config, site_mode);

  config_schema_item_type * item;
  ert_workflow_list_add_config_items(config);
  site_config_add_queue_config_items(config, site_mode);


  /*
     You can set environment variables which will be applied to the
     run-time environment. Can unfortunately not use constructions
     like PATH=$PATH:/some/new/path, use the UPDATE_PATH function instead.
   */
  item = config_add_schema_item(config, SETENV_KEY, false);
  config_schema_item_set_argc_minmax(item, 2, 2);
  config_schema_item_set_envvar_expansion(item, false); /* Do not expand $VAR expressions (that is done in util_interp_setenv()). */

  item = config_add_schema_item(config, UMASK_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, 1);

  /**
     UPDATE_PATH   LD_LIBRARY_PATH   /path/to/some/funky/lib

     Will prepend "/path/to/some/funky/lib" at the front of LD_LIBRARY_PATH.
   */
  item = config_add_schema_item(config, UPDATE_PATH_KEY, false);
  config_schema_item_set_argc_minmax(item, 2, 2);
  config_schema_item_set_envvar_expansion(item, false); /* Do not expand $VAR expressions (that is done in util_interp_setenv()). */

  if (!site_mode) {
    item = config_add_schema_item(config, LICENSE_PATH_KEY, false);
    config_schema_item_set_argc_minmax(item, 1, 1);
    config_schema_item_iset_type(item, 0, CONFIG_PATH);
  }


  /*****************************************************************/
  /* Items related to running jobs with lsf/rsh/local ...          */

  /* These must be set IFF QUEUE_SYSTEM == LSF */
  item = config_add_schema_item(config, LSF_QUEUE_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, 1);

  item = config_add_schema_item(config, LSF_RESOURCES_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, CONFIG_DEFAULT_ARG_MAX);

  item = config_add_schema_item(config, MAX_RUNNING_LSF_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, 1);
  config_schema_item_iset_type(item, 0, CONFIG_INT);

  item = config_add_schema_item(config, LSF_SERVER_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, 1);

  /* These must be set IFF QUEUE_SYSTEM == RSH */
  if (!site_mode)
    config_add_schema_item(config, RSH_HOST_KEY, false); /* Only added when user parse. */
  item = config_add_schema_item(config, RSH_COMMAND_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, 1);
  config_schema_item_iset_type(item, 0, CONFIG_EXECUTABLE);

  item = config_add_schema_item(config, MAX_RUNNING_RSH_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, 1);
  config_schema_item_iset_type(item, 0, CONFIG_INT);

  /* These must be set IFF QUEUE_SYSTEM == LOCAL */
  item = config_add_schema_item(config, MAX_RUNNING_LOCAL_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, 1);
  config_schema_item_iset_type(item, 0, CONFIG_INT);


  /*****************************************************************/

  item = config_add_schema_item(config, INSTALL_JOB_KEY, false);
  config_schema_item_set_argc_minmax(item, 2, 2);
  config_schema_item_iset_type(item, 1, CONFIG_EXISTING_PATH);

  item = config_add_schema_item(config, INSTALL_JOB_DIRECTORY_KEY, false);
  config_schema_item_set_argc_minmax(item, 1, 1);
  config_schema_item_iset_type(item, 0, CONFIG_PATH);

  item = config_add_schema_item( config , ANALYSIS_LOAD_KEY , false  );
  config_schema_item_set_argc_minmax( item , 2 , 2);
}

const char * site_config_get_location() {
    const char * site_config = NULL;

    #ifdef SITE_CONFIG_FILE
        site_config = SITE_CONFIG_FILE;
    #endif

    const char * env_site_config  = getenv("ERT_SITE_CONFIG");

    if(env_site_config != NULL) {
        if (util_file_exists(env_site_config)) {
            site_config = env_site_config;
        } else {
            fprintf(stderr, "The environment variable ERT_SITE_CONFIG points to non-existing file: %s - ignored\n", env_site_config);
        }
    }

    if (site_config == NULL) {
        fprintf(stderr, "**WARNING** main enkf_config file is not set. Use environment variable \"ERT_SITE_CONFIG\" - or recompile.\n");
    }

    return site_config;
}
