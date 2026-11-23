/* Extremely inefficient but portable POSIX getch() */

#include <stdio.h>
#include <termios.h>    /* for tcgetattr() and tcsetattr() */
#include <unistd.h>     /* for read() */

#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif

int
getch (void)
{
  struct termios save_termios;
  struct termios ios;
  int c;

  if (tcgetattr (STDIN_FILENO, &save_termios) < 0)
    return EOF;

  ios = save_termios;
  ios.c_lflag &= ~(ICANON | ECHO);
  ios.c_cc[VMIN] = 1;
  ios.c_cc[VTIME] = 0;

  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &ios) < 0)
    return EOF;

  if (read (STDIN_FILENO, &c, 1) != 1)
    c = EOF;

  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &save_termios) < 0)
    return EOF;

  return c;
}

#if defined TEST_GETCH
int
main ()
{
  int i;
  char c[4];

  printf("Enter %d keys to continue: ", sizeof c - 1);
  fflush (stdout);

  memset(c, 0, sizeof c);
  for (i=0; i<sizeof c - 1; i++)
    c[i] = getch ();

  printf("Your input: [%s]\n", c);
  return 0;
}
#endif
