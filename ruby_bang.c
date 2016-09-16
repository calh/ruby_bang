#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int add_parameters(const char *, char ***);

char * find_rvm(void);

char * find_rvm_path(void);
char * find_rvm_home(void);
char * find_rvm_system(void);

int main(int argc, char *argv[], char **envp)
{
  int i, num_params;

  char * rvm = find_rvm();

  if ( rvm == NULL )
  {
    fprintf(stderr, "I can't find an RVM installation anywhere!\n\n");
    return(1);
  }
  // simple hard coded limits, 100 params @ 1KB each 
  // should be more than enough
  char **params;
  params = (char **)malloc(100 * sizeof(char **));

  params[0] = rvm;

  num_params = add_parameters(argv[1], &params);
  // argv[2] contains the shebang script's name.
  // pass it to RVM
  params[++num_params] = argv[2];
  
  // Everthing after argv[2] are parameters passed from
  // the command line on the shell to the ruby script
  for (i = 3 ; i < argc ; i++ )
    params[++num_params] = argv[i];
    
  params[++num_params] = NULL;

  execve(rvm, params, envp);

  return 0;
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

