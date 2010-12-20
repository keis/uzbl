%{
	#include "uzbl.h"
	#include "uzbl.tab.h"

	YY_BUFFER_STATE oldbuffer;
%}

%x VAR
%option noyywrap

/* no nonsense string */
atom [a-zA-Z_]+

/* anything except special chars */
text (\\.|[^ ;\(\)\>\"\@\n])*

/* double/single quoted string literals */
dbllit \"(\\.|[^"])*\"
sgllit \'(\\.|[^'])*\'

var @[a-zA-Z_]+

jsexpand @<(\\.|[^>])*>@
shexpand @\((\\.|[^\)])*\)@

%%

<*>[ ]+			{ /*return WS;*/ }
<*>{atom}    	{ yylval.text = yytext; return ATOM; }
<*>{text}    	{ yylval.text = yytext; return TEXT; }
<*>{dbllit}  	{ yylval.text = yytext; return LITERAL; }
<*>{sgllit}  	{ yylval.text = yytext; return LITERAL; }
{jsexpand}      { yylval.text = yytext; return EXPANDJS; }
{shexpand}      { yylval.text = yytext; return EXPANDSHELL; }
{var} 		 	{
					/* dummy vars */
					char * key = yytext + 1;
					char * var;
					if(strcmp(key, "foo")==0)
						var = "set blah = (foo 'yadda yadda')";
					else if(strcmp(key, "bar")==0)
						var = "request blah blah";
					else
						var = "dummy";

					/* lex value with VAR start condition to avoid expanding further @-vars */
					oldbuffer = YY_CURRENT_BUFFER;
					yy_switch_to_buffer(yy_scan_string(var));
					BEGIN(VAR);
			 	}
<VAR>{var}   	{ yylval.text = yytext; return TEXT; }
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
