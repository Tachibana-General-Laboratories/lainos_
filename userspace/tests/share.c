#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
  if(argc <= 1){
    printf(2, "usage: share [mod]\n");
    printf(2, "  d - exec test\n");
    printf(2, "  n - server test\n");
    printf(2, "  a - attach to server test\n");
    printf(2, "\n");
    printf(2, "Server test:\n");
    printf(2, "  $ share n &\n");
    printf(2, "  $ share a\n");
    exit();
  }

  if (argv[1][0] == 'n') {
    char *sh;
    sh = (char *)shared(666);
    strcpy(sh, "Fuck World!");
	while(sh[0] == 'F');
    printf(1, "Ok. Server exit. sh=%s\n", sh);
	exit();
  }

  if (argv[1][0] == 'a') {
    char *sh;
    sh = (char *)shared_attach(666);
    printf(2, "sh is %x\n", sh);
    printf(1, "shared mem has: %s\n", sh);
	sh[1] = 'a';
	sh[0] = 'H';
	exit();
  }

  if (argv[1][0] == 'd') {
    char *sh;
    // first exec
    argv[1][0] = ' ';
    sh = (char *)shared(777);
    printf(2, "sh is %x\n", sh);
    strcpy(sh, "Hello World!");
    exec(argv[0], argv);
  } else {
    // second exec
    char *sh = (char *)shared(777);
    printf(2, "sh is %x\n", sh);
    printf(1, "shared mem has: %s\n", sh);
  }
  exit();
}
