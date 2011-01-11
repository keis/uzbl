%{
	#include "uzbl.h"
	#include "uzbl.tab.h"

	YY_BUFFER_STATE oldbuffer;
	char * get_var(char * key) {
		/* dummy vars */
		if(strcmp(key, "foo")==0)
			return "set blah = (foo 'yadda yadda')";
		if(strcmp(key, "bar")==0)
			return "request blah blah";
		return "dummy";
	}

%}

%x VAR
%option noyywrap

comment #.*$

/* no nonsense string */
atom [a-zA-Z_]+

/* anything except special chars */
text (\\.|[^ ;\(\)\>\"\@\n])*

/* double/single quoted string literals */
dbllit \"(\\.|[^"])*\"
sgllit \'(\\.|[^'])*\'

var @[a-zA-Z_\.]+
crlyvar @\{[a-zA-Z_\.]+\}

jsexpand @<(\\.|[^>])*>@
shexpand @\((\\.|[^\)])*\)@

%%

<*>[ ]+			{ return WS; }
<*>{comment}    { }
<*>{atom}    	{ yylval.text = yytext; return ATOM; }
<*>{text}    	{ yylval.text = yytext; return TEXT; }
<*>{dbllit}  	{ yylval.text = yytext; return LITERAL; }
<*>{sgllit}  	{ yylval.text = yytext; return LITERAL; }
{jsexpand}      { yylval.text = yytext; return EXPANDJS; }
{shexpand}      { yylval.text = yytext; return EXPANDSHELL; }
{var} 		 	{
					char * var = get_var(yytext + 1);

					/* lex value with VAR start condition to avoid expanding further @-vars */
					oldbuffer = YY_CURRENT_BUFFER;
					yy_switch_to_buffer(yy_scan_string(var));
					BEGIN(VAR);
			 	}
{crlyvar}       {
					yytext[yyleng-1] = 0;
					char * var = get_var(yytext + 2);

					/* lex value with VAR start condition to avoid expanding further @{}-vars */
					oldbuffer = YY_CURRENT_BUFFER;
					yy_switch_to_buffer(yy_scan_string(var));
					BEGIN(VAR);
				}
<VAR>{var}   	{ yylval.text = yytext; return TEXT; }
<VAR>{crlyvar}  { yylval.text = yytext; return TEXT; }
<VAR><<EOF>> 	{
					/* return to original buffer when done lexing var */
					yy_delete_buffer(YY_CURRENT_BUFFER);
					yy_switch_to_buffer(oldbuffer);
					BEGIN(INITIAL);
			 	}
<*>"("       	{ return OPEN; }
<*>")"       	{ return CLOSE; }
<*>"\n"      	{ return NEWLINE; }
<*>";"       	{ return SEMICOLON; }
