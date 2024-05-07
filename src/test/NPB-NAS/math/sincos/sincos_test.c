#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include "nas_sincos.h"

int printf(const char*, ...);
static int sincos(char *__buf, void* __priv);

double sin(double);
double cos(double);

static struct shell_cmd_impl nas_sincos_impl = {
    .cmd      = "sincos",
    .help_str = "sincos [integer]",
    .handler  = sincos,
};
nk_register_shell_cmd(nas_sincos_impl);
/*--------------------------------------------------------------------
      program BT
c-------------------------------------------------------------------*/
static int sincos(char *buf, void* __priv) 
{
  int num;

  if(sscanf(buf, "sincos %d", &num) != 1) {
      printf("Unrecognized arguments: usage: sincos [integer]\n");
      return 0;
  }

  double d = (double)num;
  printf("sin(%8.8lf) = %8.8lf, cos(%8.8lf) = %8.8lf\n",d,sin(d),d,cos(d));
  return 0;
}

