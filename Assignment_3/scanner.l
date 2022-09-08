%{
#include <iostream>
#include <vector>
#include <string>
#include "parser.tab.hpp"
#include "symbol.h"
using namespace std;
%}

DIGIT		[0-9]
IDENTIFIER 	[a-zA-Z][a-zA-Z0-9]*
COMMENT		--.*\n
WHITESPACE 	[ \r\n\t]

%%

PROGRAM			{ return KW_PROGRAM; }  
BEGIN			{ return KW_BEGIN; }	
END			{ return KW_END; }	
FUNCTION		{ return KW_FUNCTION; }
READ			{ return KW_READ; }	
WRITE			{ return KW_WRITE; }	
IF			{ return KW_IF; }	
ELSE			{ return KW_ELSE; }	
FI			{ return KW_FI; }	
FOR			{ return KW_FOR; }	
ROF			{ return KW_ROF; }	
BREAK			{ return KW_BREAK; }	
CONTINUE		{ return KW_CONTINUE; }
RETURN			{ return KW_RETURN; }	
INT			{ yylval.str = new string(yytext); return KW_INT; }	
VOID			{ return KW_VOID; }		
STRING			{ yylval.str = new string(yytext); return KW_STRING; }		
FLOAT			{ yylval.str = new string(yytext); return KW_FLOAT; }		

":="			{ return OP_ASSIGN; }		
"+"			{ return OP_ADD; }	
"-"			{ return OP_SUB; }		
"*"			{ return OP_MUL; }		
"/"			{ return OP_DIV; }		
"="			{ return OP_EQ; }	
"!="			{ return OP_NEQ; }		
"<"			{ return OP_LT; }		
">"			{ return OP_GT; }		
"("			{ return OP_OP; }		
")"			{ return OP_CP; }		
";"			{ return OP_SEMICOLON; }	
","			{ return OP_COMMA; }		
"<="			{ return OP_LEQ; }		
">="			{ return OP_GEQ; }	

{IDENTIFIER} 		{ yylval.str = new string(yytext); return IDENTIFIER; } 
{DIGIT}+ 		{ return INTLITERAL; }
{DIGIT}*"."{DIGIT}+ 	{ return FLOATLITERAL; }
\"[^\"]*\" 		{ yylval.str = new string(yytext); return STRINGLITERAL; }
{COMMENT} 		{}
{WHITESPACE}+		{}
.			{}

%%

int yywrap(){
	return 1;
}
