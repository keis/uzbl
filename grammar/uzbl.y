/* I can haz grammar? */

/* should merge TEXT/ATOM/LITERAL when not separated by whitespace
    -- done for ATOM + ATOM, and ATOM + TEXT
spawn @scripts_dir/woop.py
blah test"foo foo"
*/
%{
#include <glib.h>
#include "uzbl.h"

void indent(int level) {
    while(level--) {
        g_print("\t");
    }
}

void dump_command(struct Command * c, int level) {
    struct Argument * p;
    indent(level); g_print ("-- %s\n", c->command);
    for(p = c->args; p; p = p->next) {
        if(p->type == ARG_STR) {
            indent(level+1); g_print("%s\n", p->argument.str);
        } else if(p->type == ARG_COMMAND)
            dump_command(p->argument.command, level+1);
    }
}

struct Argument * reverse_arguments(struct Argument * head, struct Argument * acc) {
    if(head == NULL) {
        return acc;
    } else {
        struct Argument * tmp = head->next;
        head->next = acc;
        return reverse_arguments(tmp, head);
    }
}
%}

%union {
    char * text;
    struct Command * command;
    struct Argument * arg;
}

%token ATOM TEXT LITERAL SEMICOLON OPEN CLOSE NEWLINE WS
%token EXPANDJS EXPANDSHELL
%start input

%%

/* in reality something useful should happen here */
input       : /* empty */               { }
            | input command             { dump_command($<command>2, 0); }
            /* "command1 ; command 2 ; command3" would replace current use of chain command */
            | input SEMICOLON           { g_print("CHAIN\n"); }
            | input NEWLINE             { g_print("GO\n"); }
;

ws          : WS                        { }
            | /* empty */               { }

atom        : ATOM                      { $<text>$ = g_strdup($<text>1); }
            | atom ATOM                 { $<text>$ = g_strdup_printf("%s%s", $<text>1, $<text>2); }
;

/* should unescape string (and remove quotes?) */
literal     : LITERAL                   { $<text>$ = g_strdup($<text>1); }
;

/* should unescape string */
text        : TEXT                      { $<text>$ = g_strdup($<text>1); }
            | text TEXT                 { $<text>$ = g_strdup_printf("%s%s", $<text>1, $<text>2); }
            | atom text                 { $<text>$ = g_strdup_printf("%s%s", $<text>1, $<text>2); }
            | text atom                 { $<text>$ = g_strdup_printf("%s%s", $<text>1, $<text>2); }
;

/* simple name followed by 0 or more arguments */
command     : atom args                 {
                                            struct Command * comm = g_malloc(sizeof(struct Command));
                                            comm->command = $<text>1;
                                            comm->args = reverse_arguments($<arg>2, 0);
                                            $<command>$ = comm;
                                        }
;

args        : /* empty */               { $<arg>$ = NULL; }
            /* strings are used as is */
            | args WS atom              {
                                            struct Argument * narg = g_malloc(sizeof(struct Argument));
                                            narg->argument.str = $<text>3;
                                            narg->type = ARG_STR;
                                            narg->next = $<arg>1;
                                            $<arg>$ = narg;
                                        }
            | args WS text              {
                                            struct Argument * narg = g_malloc(sizeof(struct Argument));
                                            narg->argument.str = $<text>3;
                                            narg->type = ARG_STR;
                                            narg->next = $<arg>1;
                                            $<arg>$ = narg;
                                        }
            | args WS literal           {
                                            struct Argument * narg = g_malloc(sizeof(struct Argument));
                                            narg->argument.str = $<text>3;
                                            narg->type = ARG_STR;
                                            narg->next = $<arg>1;
                                            $<arg>$ = narg;
                                        }
            /* run a command use its output as the argument
             * could be ` or $( to be more shell like, but I like plain brackets better
             */
            | args WS OPEN ws command ws CLOSE
                                        {
                                            struct Argument * narg = g_malloc(sizeof(struct Argument));
                                            narg->argument.command = $<command>5;
                                            narg->type = ARG_COMMAND;
                                            narg->next = $<arg>1;
                                            $<arg>$ = narg;
                                        }
            /* syntatic sugar for "(eval_js arg)"
             * assumes we will have a friendly eval_js command the returns the js-value
             */
            | args WS EXPANDJS          {
                                            struct Argument * carg = g_malloc(sizeof(struct Argument));
                                            carg->argument.str = $<text>3;
                                            carg->type = ARG_STR;
                                            carg->next = NULL;

                                            struct Command * comm = g_malloc(sizeof(struct Command));
                                            comm->command = "eval_js";
                                            comm->args = carg;

                                            struct Argument * narg = g_malloc(sizeof(struct Argument));
                                            narg->argument.command = comm;
                                            narg->type = ARG_COMMAND;
                                            narg->next = $<arg>1;
                                            $<arg>$ = narg;
                                        }
            /* syntatic sugar for "(sync_sh arg)"
             * assumes we will have a friendly sync_sh that returns the captured output
            */
            | args WS EXPANDSHELL       {
                                            struct Argument * carg = g_malloc(sizeof(struct Argument));
                                            carg->argument.str = $<text>3;
                                            carg->type = ARG_STR;
                                            carg->next = NULL;

                                            struct Command * comm = g_malloc(sizeof(struct Command));
                                            comm->command = "sync_sh";
                                            comm->args = carg;

                                            struct Argument * narg = g_malloc(sizeof(struct Argument));
                                            narg->argument.command = comm;
                                            narg->type = ARG_COMMAND;
                                            narg->next = $<arg>1;
                                            $<arg>$ = narg;
                                        }
;

%%

#include <stdio.h>
#include <ctype.h>

int yylex ();

main()
{
    //yydebug = 1;
    yyparse();
}

yyerror( char * str )
{
    printf( "yyerror: %s\n", str );
}
