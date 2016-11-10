#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <libgen.h>

#define MAX_PARAMS 100

int add_parameters(const char *, char ***);

char * find_rvm(void);

char * find_rvm_path(void);
char * find_rvm_home(void);
char * find_rvm_system(void);

char ** build_rvm_params(char *rvm, char *argv[]);
char ** rvm_do_params(char **rvm_params);

int count_params(char **params);
int has_bundle_exec(char **rvm_params);
int rvm_exec(int quiet, char **envp, char **rvm_params, ...);
void bundle_install(char **rvm_params, char **envp);
char * working_dir(char *script_path);

int main(int argc, char *argv[], char **envp)
{

  char * rvm = find_rvm();
  char ** rvm_params = build_rvm_params(rvm, argv);

  if ( rvm == NULL )
  {
    fprintf(stderr, "I can't find an RVM installation anywhere!\n\n");
    return(1);
  }

  // check if we need to bundle install and do it
  bundle_install(rvm_params, envp);

  execve(rvm, rvm_params, envp);

  return 0;
}

// change the working dir to 
char * working_dir(char *script_path)
{
  char resolved_path[PATH_MAX];

  if ( ! realpath(script_path, resolved_path) )
  {
    perror("realpath");
    exit(1);
  }

  char *dir = dirname(strdup(resolved_path));

  if (  chdir(dir) < 0 )
  {
    perror("chdir");
    exit(1);
  }

  return strdup(resolved_path);
}

/* check if:
 *  1) "bundle exec" is in the rvm_params list
 *  2) run "bundle check", do nothing if exit status == 0
 *  3) run "bundle install"
 */
void bundle_install(char **rvm_params, char **envp)
{
  if ( ! has_bundle_exec(rvm_params) )
   return;

  // bundle check in quiet mode
  if ( rvm_exec(1, envp, rvm_params, "bundle", "check", NULL) == 0 )
    return;

  // bundle install as verbose, since we shouldn't see it often
  if ( rvm_exec(0, envp, rvm_params, "bundle", "install", NULL) != 0 )
  {
    // if the install fails, don't bother to run the script
    exit(1);
  }

}

// check if "bundle exec" is in the params list
int has_bundle_exec(char **rvm_params)
{
  int i;
  for ( i = 0 ; i < count_params(rvm_params) ; i++ )
  {
    if ( !strcmp(rvm_params[i], "bundle") && !strcmp(rvm_params[i + 1], "exec") )
      return 1;
  }
  return 0;
}
/* convenience function -- given the full rvm_params list
 * from the shebang, return only the "rvm VERSION do" 
 * list for making other calls
 */
char ** rvm_do_params(char **rvm_params)
{
  int i;
  char ** rvm_do_params = (char **)malloc(MAX_PARAMS * sizeof(char **));
  for ( i = 0 ; i < count_params(rvm_params) ; i++ )
  {
    if ( ! strcmp(rvm_params[i], "do") )
      break;
    rvm_do_params[i] = rvm_params[i];
  }
  rvm_do_params[i] = rvm_params[i];
  rvm_do_params[++i] = NULL;
  return rvm_do_params;
}

/* given the main rvm_params, pull out the "rvm VERSION do"
 * part and execute a different set of parameters, then 
 * return the exit status of the command
 */
int rvm_exec(int quiet, char **envp, char **rvm_params, ...)
{
  int rvm_count, i;
  va_list ap;
  char *str;

  char ** rvm_do = rvm_do_params(rvm_params);
  rvm_count = count_params(rvm_do);

  va_start(ap, rvm_params);

  // append varaiable args list to rvm_do
  i = rvm_count;
  do 
  {
    str = va_arg(ap, char *);
    if ( str == NULL )
      break;
    rvm_do[i++] = strdup(str);
  } while (str != NULL);
  va_end(ap);
  rvm_do[i] = NULL;

  // fork and execve()
  int pid = fork();
  int status;
 
  // child 
  if ( pid == 0 )
  { 
    /* reopen stdout to /dev/null to make this a quiet operation 
     * (note that fclose() seems to mess with bundler
     */
    if ( quiet && freopen("/dev/null", "w", stdout) == NULL )
      fprintf(stderr, "Error reopening stdout\n");

    execve(rvm_do[0], rvm_do, envp);
  }
  else if ( pid > 0 )
  { //parent
    waitpid(pid, &status, 0);
    return( WEXITSTATUS(status) );
  }
  else
  {
    fprintf(stderr, "Fork error\n");
    exit(1);
  }
 
  // fall back to returning error 
  return 1;
}
int count_params(char **params)
{
  int i;
  for ( i = 0 ; params[i] != NULL ; i++ );
  return(i);
}

char ** build_rvm_params(char *rvm, char *argv[])
{
  int i, num_params;

  int argc = count_params(argv);
  // simple hard coded limits, 100 params @ 1KB each 
  // should be more than enough
  char **params;
  params = (char **)malloc(MAX_PARAMS * sizeof(char **));

  params[0] = rvm;

  num_params = add_parameters(argv[1], &params);
  /* argv[2] contains the shebang script's name.
   * pass it to RVM as a full canonical path.
   * also change working directory to script dirname
   */
  char *canon_script = working_dir(argv[2]);
  params[++num_params] = canon_script;
  
  // Everthing after argv[2] are parameters passed from
  // the command line on the shell to the ruby script
  for (i = 3 ; i < argc ; i++ )
    params[++num_params] = argv[i];
    
  params[++num_params] = NULL;
  return(params);
}
/* given an input string, split it into space delimited tokens
 * and append it on element 1 of the passed in argv array.
 * Return the number of parameters seen.
 */
int add_parameters(const char *input, char ***params)
{
  int i;
  char *piece;

  // dup for strtok safety
  char *input_copy = strdup(input);

  piece = strtok(input_copy, " ");
  if ( piece == NULL )
    return 0;

  (*params)[1] = piece;

  for( i = 2 ; i < 1024 && piece != NULL ; i++ )
  {
    piece = strtok(NULL, " ");
    if ( piece == NULL )
      break;
    (*params)[i] = piece;
  }
  return (i-1);
} 

/* Find the rvm executable using the 
 * method below
 */
char * find_rvm(void)
{
  char *rvm;
  rvm = find_rvm_path();
  if ( rvm )
    return(rvm);
  rvm = find_rvm_home();
  if ( rvm )
    return(rvm);
  rvm = find_rvm_system();
  if ( rvm )
    return(rvm);
  return NULL;
}

/* Search the current $PATH for rvm */
char * find_rvm_path(void)
{
  char *path = getenv("PATH");
  if ( path == NULL )
    return NULL;
  // dup for strchr()
  path = strdup(path);
  char *str = path;
  char *dir = NULL;
  char buffer[PATH_MAX];

  do
  {
    dir = strchr(str, ':');
    if ( dir != NULL )
        dir[0] = 0;
    sprintf(buffer, "%s/rvm", str);
    if ( access(buffer, X_OK) == 0 )
    {
      free(path);
      return(strdup(buffer));
    }
    str = dir + 1;
  } while (dir != NULL );
  free(path);
  return NULL;
}

/* Next search $HOME/.rvm/bin/rvm */
char * find_rvm_home(void)
{
  char * home = getenv("HOME");
  char buffer[PATH_MAX];

  if ( home == NULL )
    return NULL;
  sprintf(buffer, "%s/.rvm/bin/rvm", home);
  if ( access(buffer, X_OK) == 0 )
    return strdup(buffer);

  return NULL;
}
/* And finally search a few possible system paths */
char * find_rvm_system(void)
{
  char buffer[PATH_MAX];
  char *locations[] = {
    "/usr/local/rvm/bin/rvm",
    "/opt/rvm/bin/rvm",
    "/usr/bin/rvm",
    NULL
  };
  int i;
  for ( i = 0 ; locations[i] != NULL ; i++ )
  {
    if ( access(buffer, X_OK) == 0 )
      return strdup(locations[i]);
  }
  return NULL;
}

