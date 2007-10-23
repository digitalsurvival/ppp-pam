/*
  mpi-pfn.c

  A simple arbitrary-precision integer arithmetic calculator, based on
  the MPI library.  The calculator reads from its standard input, and
  writes results to standard output.  Numeric values are pushed on a
  stack, and operators perform their work on the stack contents, with
  results being pushed back to the stack.

  See the 'command' array for a list of the commands currently
  understood by the calculator.

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 2000 Michael J. Fromberger, All Rights Reserved

  $Id: mpicalc.c,v 1.1 2004/02/08 04:28:32 sting Exp $ 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "mpi.h"
#include "mpprime.h"

typedef enum {
  t_eof, t_num, t_op, t_badchar, t_badnum, t_nodata, NUM_TOKEN_TYPES
} token_t;

/* function called to handle an operator */
typedef int (*op_f)(char *);

/* 
   Each command is represented by a cmd_t record.  This structure has
   the name you enter to get the command, how many arguments it needs
   to see on the stack, and a pointer to the function called to handle
   the command.
 */
typedef struct {
  char       *name;   /* string used to invoke this command          */
  int         nargs;  /* arguments required for this command         */
  op_f        func;   /* function to handle this command             */
} cmd_t;

/*
  This is the structure used to represent a value on the stack.  Only
  values are stored on the stack, since there are no stored
  procedures.
 */
typedef struct stk {
  mp_int      *value; /* the value at this position of the stack     */
  struct stk  *link;  /* the item below this one on the stack        */
} stack2_t;

/* A token is the type returned by the lexical analyzer              */
typedef struct {
  token_t      type;  /* the type of the token                       */
  char        *start; /* where the literal text for the token starts */
  int          len;   /* length of the token, in characters          */
} token;

/* A lex_info is how you pass source text to the lexical analyzer    */
typedef struct {
  char        *src;   /* beginning of the source string              */
  char        *next;  /* next character to be processed              */
} lex_info;

/*------------------------------------------------------------------------*/
/* Global state information                                               */

int            obase = 10; /* current output radix           */

stack2_t       *s_top; /* top of the evaluation stack    */
int            s_num; /* number of items on the stack   */

/* This variable holds the "memory" of the calculator; the 'mem'
   command copies the top stack value here, the 'rst' command pulls it
   back to the stack.
 */
mp_int        *mem = NULL;

/* Handlers for the various commands (see the command[] vector below) */
int dup(char *cmd);
int exch(char *cmd);
int clear(char *cmd);
int pop(char *cmd);
int memory(char *cmd);
int restore(char *cmd);
int print(char *cmd);
int pstack(char *cmd);
int setbase(char *cmd);
int unop(char *cmd);
int binop(char *cmd);
int trinop(char *cmd);
int xgcd(char *cmd);
int compare(char *cmd);
int invmod(char *cmd);
int cmds(char *cmd);

/* A useful function for printing out the stack */
void recprint(stack2_t *stk);

/* 

   The vector mapping the available commands.  The find_cmd() function
   uses this to figure out what command you entered, if any.  To add a
   new command, add a similar entry to this list.  Make sure the last
   entry has NULL as its 'name' field, so that search will terminate.

   Linear search is employed, so put frequently used things near the
   top of the list. :)
 */
cmd_t command[] = {
  { "dup", 1, dup },     /* duplicate top value       */
  { "exch", 2, exch },   /* exchange top two values   */
  { "clear", 0, clear }, /* clear stack (make empty)  */
  { "pop", 1, pop },     /* pop and discard stack top */
  { "mem", 1, memory },  /* pop top elt to memory     */
  { "rst", 0, restore }, /* restore memory to stack   */
  { "print", 1, print }, /* print top stack value     */
  { "?", 1, print },

  { "neg", 1, unop },    /* negate                    */
  { "abs", 1, unop },    /* absolute value            */
  { "add", 2, binop },   /* add                       */
  { "sub", 2, binop },   /* subtract                  */
  { "mul", 2, binop },   /* multiply                  */
  { "div", 2, binop },   /* divide                    */
  { "mod", 2, binop },   /* remainder                 */
  { "expt", 2, binop },  /* exponentiate              */

  { "addm", 3, trinop }, /* modular add               */
  { "subm", 3, trinop }, /* modular subtract          */
  { "mulm", 3, trinop }, /* modular multiply          */
  { "exptm", 3, trinop },/* modular exponentiate      */

  { "cmp", 2, compare }, /* compare top two values    */

  { "gcd", 2, binop },   /* greatest common divisor   */
  { "xgcd", 2, xgcd },   /* extended GCD              */
  { "lcm", 2, binop },   /* least common multiple     */
  { "invm", 2, invmod }, /* modular inverse           */

  { "sqrt", 1, unop },   /* square root               */

  { "list", 0, pstack }, /* print all stack values    */
  { "hex", 0, setbase }, /* set output base to 16     */
  { "dec", 0, setbase }, /* set output base to 10     */
  { "oct", 0, setbase }, /* set output base to 8      */
  { "bin", 0, setbase }, /* set output base to 2      */

  { "cmds", 0, cmds },   /* list of commands          */
  
  { NULL, 0, NULL }
};

token lex(lex_info *lp);  /* get next token from the input           */
void  stack_init(void);   /* set up a new empty stack                */
void  stack_clear(void);  /* flush the stack, make it empty again    */
void  stack_push(mp_int *val);  /* push a value onto the stack       */
void  stack_pop(int n);   /* discard the top n values from the stack */
int   find_cmd(char *op, int len);

#define BUFFER_SIZE  4096

int main(void)
{
  unsigned char    *buf;
  token    next;
  lex_info info;
  int      cmd, esc = 0;
  mp_int  *value;

  buf = malloc(BUFFER_SIZE);

  while(fgets((char *)buf, BUFFER_SIZE, stdin) != NULL) {
    info.src = (char *)buf;
    info.next = NULL;

    do {
      next = lex(&info);
      esc = 0;

      switch(next.type) {
      case t_eof: /* done with this line */
	break;

      case t_num:
	value = malloc(sizeof(*value));
	mp_init(value);
	if(next.start[0] == '0') {
	  if(next.start[1] == 'x' || next.start[1] == 'X')
	    mp_read_radix(value, (unsigned char *)next.start + 2, 16);
	  else
	    mp_read_radix(value, (unsigned char *)next.start, 8);
	} else {
	  mp_read_radix(value, (unsigned char *)next.start, 10);
	}

	stack_push(value);
	break;

      case t_op:
	if((cmd = find_cmd(next.start, next.len)) < 0) {
	  printf("[Command not understood]\n");
	  esc = 1; /* break out of lex loop */
	} else {
	  esc = !((command[cmd].func)(command[cmd].name));
	}
	break;

      case t_badchar:
	printf("[Invalid input character]\n");
	esc = 1;
	break;

      case t_badnum:
	printf("[Invalid number format]\n");
	esc = 1;
	break;

      case t_nodata: case NUM_TOKEN_TYPES:
	break;
      }

    } while(!esc && next.type != t_eof);

  } /* end while(input) */

  stack_clear();
  free(buf);

  return 0;
}

int dup(char *cmd)
{
  mp_int  *tmp;

  if(s_num == 0) {
    printf("[Stack underflow]\n");
    return 0;
  }

  tmp = malloc(sizeof(*tmp));
  mp_init_copy(tmp, s_top->value);

  stack_push(tmp);
  return 1;

} /* end dup() */

int exch(char *cmd)
{
  if(s_num < 2) {
    printf("[Stack underflow]\n");
    return 0;
  }

  mp_exch(s_top->value, s_top->link->value);
  return 1;

} /* end exch() */

int clear(char *cmd)
{
  stack_clear();
  return 1;

} /* end clear() */

int pop(char *cmd)
{
  if(s_num < 1) {
    printf("[Stack underflow]\n");
    return 0;
  }
	   
  mp_clear(s_top->value);
  free(s_top->value);
  stack_pop(1);
  return 1;

} /* end pop() */

int memory(char *cmd)
{
  mp_int  *tmp;

  if(s_num < 1) {
    printf("[Stack underflow]\n");
    return 0;
  }

  tmp = malloc(sizeof(*tmp));
  mp_init_copy(tmp, s_top->value);

  if(mem != NULL) {
    mp_clear(mem);
    free(mem);
  }

  mem = tmp;
  return 1;

} /* end memory() */

int restore(char *cmd)
{
  mp_int  *tmp;

  if(mem == NULL) {
    printf("[Memory empty]\n");
    return 0;
  }

  tmp = malloc(sizeof(*tmp));
  mp_init_copy(tmp, mem);
  stack_push(tmp);

  return 1;

} /* end restore() */

int print(char *cmd)
{
  unsigned char  *buf;
  int    len;

  if(s_num < 1) {
    printf("[Stack underflow]\n");
    return 0;
  }

  len = mp_radix_size(s_top->value, obase);
  buf = malloc(len);
  mp_toradix(s_top->value, buf, obase);
  printf("%s\n", buf);
  free(buf);

  return 1;

} /* end print() */

int pstack(char *cmd)
{
  if(s_num == 0) {
    printf("[Stack empty]\n");
  } else {
    recprint(s_top);
  }

  return 1;

} /* end pstack() */

void recprint(stack2_t *stk)
{
  unsigned char   *buf;
  int     len;

  if(stk == NULL)
    return;

  recprint(stk->link);

  len = mp_radix_size(stk->value, obase);
  buf = malloc(len);
  mp_toradix(stk->value, buf, obase);
  printf("%s\n", (char *)buf);
  free(buf);

} /* end recprint() */

int setbase(char *cmd)
{
  switch(cmd[0]) {
  case 'h':
    obase = 16;
    break;
  case 'd':
    obase = 10;
    break;
  case 'o':
    obase = 8;
    break;
  case 'b':
    obase = 2;
    break;
  default:
    printf("[Unknown output base]\n");
    return 0;

  }

  return 1;

} /* end hexbase() */

int unop(char *cmd)
{
  if(s_num < 1) {
    printf("[Stack underflow]\n");
    return 0;
  }

  switch(cmd[0]) {
  case 'a':  /* abs */
    mp_abs(s_top->value, s_top->value);
    break;
  case 'n':  /* neg */
    mp_neg(s_top->value, s_top->value);
    break;
  case 's':  /* sqrt */
    mp_sqrt(s_top->value, s_top->value);
    break;
  default:
    printf("[Unexpected unary operator: %s]\n", cmd);
    return 0;

  }
  
  return 1;

} /* end unop() */

int binop(char *cmd)
{
  mp_int   *result;

  if(s_num < 2) {
    printf("[Stack underflow]\n");
    return 0;
  }

  result = malloc(sizeof(*result));
  mp_init(result);

  switch(cmd[0]) {

  case 'a': /* add */
    mp_add(s_top->value, s_top->link->value, result);
    break;

  case 's': /* sub */
    mp_sub(s_top->link->value, s_top->value, result);
    break;

  case 'm': /* mul, mod */
    if(cmd[1] == 'u')
      mp_mul(s_top->value, s_top->link->value, result);
    else
      mp_mod(s_top->link->value, s_top->value, result);
    break;

  case 'd': /* div */
    mp_div(s_top->link->value, s_top->value, result, NULL);
    break;

  case 'e': /* expt */
    mp_expt(s_top->link->value, s_top->value, result);
    break;

  case 'g': /* gcd */
    mp_gcd(s_top->value, s_top->link->value, result);
    break;

  case 'l': /* lcm */
    mp_lcm(s_top->value, s_top->link->value, result);
    break;

  default:
    printf("[Unexpected binary operator: %s]\n", cmd);
    mp_clear(result);
    free(result);
    return 0;

  }

  stack_pop(2);
  stack_push(result);

  return 1;

} /* end binop() */

int trinop(char *cmd)
{
  mp_int  *result;

  if(s_num < 3) {
    printf("[Stack underflow]\n");
    return 0;
  }

  result = malloc(sizeof(*result));
  mp_init(result);

  switch(cmd[0]) {

  case 'a':  /* addm */
    mp_addmod(s_top->link->link->value,
	      s_top->link->value,
	      s_top->value,
	      result);
    break;

  case 's':  /* subm */
    mp_submod(s_top->link->link->value,
	      s_top->link->value,
	      s_top->value,
	      result);
    break;

  case 'm':  /* mulm */
    mp_mulmod(s_top->link->link->value,
	      s_top->link->value,
	      s_top->value,
	      result);
    break;

  case 'e':  /* exptm */
    mp_exptmod(s_top->link->link->value,
	       s_top->link->value,
	       s_top->value,
	       result);
    break;

  default:
    printf("[Unexpected trinary operator: %s]\n", cmd);
    mp_clear(result);
    free(result);
    return 0;

  }

  stack_pop(3);
  stack_push(result);

  return 1;

} /* end trinop() */

int xgcd(char *cmd)
{
  mp_int  *result, *x, *y;

  if(s_num < 2) {
    printf("[Stack underflow]\n");
    return 0;
  }

  result = malloc(sizeof(*result));
  x = malloc(sizeof(*x));
  y = malloc(sizeof(*y));
  mp_init(result);
  mp_init(x);
  mp_init(y);

  mp_xgcd(s_top->link->value, 
	  s_top->value,
	  result,
	  x,
	  y);

  stack_pop(2);
  stack_push(result);
  stack_push(x);
  stack_push(y);

  return 1;

} /* end xgcd() */

int compare(char *cmd)
{
  mp_int  *result;

  if(s_num < 2) {
    printf("[Stack underflow]\n");
    return 0;
  }

  result = malloc(sizeof(*result));
  mp_init(result);

  mp_set_int(result, 
	     mp_cmp(s_top->link->value,
		    s_top->value));

  stack_pop(2);
  stack_push(result);
  
  return 1;

} /* end compare() */

int invmod(char *cmd)
{
  mp_int  *result;

  if(s_num < 2) {
    printf("[Stack underflow]\n");
    return 0;
  }
  
  result = malloc(sizeof(*result));
  mp_init(result);

  if(mp_invmod(s_top->link->value,
	       s_top->value,
	       result) == MP_UNDEF) 
    mp_zero(result);

  stack_pop(2);
  stack_push(result);
  
  return 1;

} /* end invmod() */

int cmds(char *cmd)
{
  int  ix = 0;

  while(command[ix].name != NULL) {
    printf("[%s:%d]\n", command[ix].name,
	   command[ix].nargs);
    ++ix;
  }

  return 1;

} /* end cmds() */

void  stack_init(void)
{
  s_top = NULL;
  s_num = 0;

} /* end stack_init() */

void  stack_clear(void)
{
  while(s_top != NULL) {
    if(s_top->value != NULL) {
      mp_clear(s_top->value);
      free(s_top->value);
    }

    stack_pop(1);
  }

} /* end stack_clear() */

void  stack_push(mp_int *value)
{
  stack2_t *elt;

  elt = malloc(sizeof(*elt));
  elt->value = value;
  elt->link = s_top;
  s_top = elt;
  ++s_num;

} /* end stack_push() */

void  stack_pop(int n)
{
  if(n > s_num)
    n = s_num;

  while(n > 0) {
    stack2_t  *elt = s_top->link;

    free(s_top);
    s_top = elt;
    --s_num;
    --n;
  }

} /* end stack_pop() */

int   find_cmd(char *op, int len)
{
  int   ix = 0, out = -1; /* -1 indicates not found */
  char  save = op[len];

  op[len] = '\0';
  while(command[ix].name != NULL) {
    if(strcmp(command[ix].name, op) == 0) {
      out = ix;
      break;
    }

    ++ix;
  }

  op[len] = save;
  return out; 

} /* end find_cmd() */

token lex(lex_info *lp)
{
  token   out;
  char   *next, *pos;

  /* Rational behaviour in the face of bad input... */
  if(lp == NULL || lp->src ==  NULL) {
    out.type = t_nodata;
    out.start = NULL;
    out.len = 0;
    return out;
  }

  /* A NULL 'next' field indicates that we're being given a new string
     to process.  This requires resetting 'next' appropriately
   */
  if(lp->next == NULL) 
    lp->next = lp->src;

  next = lp->next;
  for(;;) {
    switch(*next) {
    case '\0':
      out.type = t_eof;
      out.start = next;
      out.len = 1;
      return out;

    case ' ': case '\t': case '\f':  case '\n': case '\r':
      while(isspace((int)*next))
	++next;
      lp->next = next;
      break;

    case '-':
    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9':
      pos = next++;

      /* If there is a leading zero, it may be either octal or hex.  Check
	 the next character, and if that is an 'x' or an 'X', we assume hex;
	 otherwise, we assume octal
       */
      if(*pos == '0') {
	if(*next == 'x' || *next == 'X') {
	  /* Handle hexadecimal */
	  ++next;
	  while(isxdigit((int)*next))
	    ++next;
	} else {
	  /* Handle octal */
	  while(isdigit((int)*next) && *next != '8' && *next != '9')
	    ++next;
	}
      } else if(*pos == '-' && !isdigit((int)*next)) {
	out.type = t_badnum;
	out.start = pos;
	out.len = next - pos;
	lp->next = next;
	return out;

      } else {
	/* Handle decimal */
	while(isdigit((int)*next))
	  ++next;
      }
      out.type = t_num;
      out.start = pos;
      out.len = next - pos;
      lp->next = next;
      return out;
      
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z': 
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
    case 'v': case 'w': case 'x': case 'y': case 'z': 
    case '~': case '!': case '@': case '#': case '$': case '%': case '^':
    case '&': case '*': case '(': case ')': case '_': case '+': case '=': 
    case '[': case ']': case '{': case '}': case '\\': case '|':
    case ':': case ';': case '\'': case '\"': case ',': case '.': case '<':
    case '>': case '/': case '?':
      pos = next++;
      while(isalpha((int)*next) || ispunct((int)*next))
	++next;

      out.type = t_op;
      out.start = pos;
      out.len = next - pos;
      lp->next = next;
      return out;

    default:
      pos = next++;
      out.type = t_badchar;
      out.start = pos;
      out.len = 1;
      lp->next = next;
      return out;
    
    } /* end switch(*next) */
  } /* end for(;;) */

} /* end lex() */

/*------------------------------------------------------------------------*/
/* HERE THERE BE DRAGONS                                                  */
