%{
#include "parser.tab.hpp"
%}

DIGIT		[0-9]
IDENTIFIER 	[a-zA-Z][a-zA-Z0-9]*
COMMENT		--.*\n
WHITESPACE 	[ \r\n\t]

%%

PROGRAM			{ return KW_PROGRAM; }  	/* KEYWORD_PROGRAM */
BEGIN			{ return KW_BEGIN; }		/* KEYWORD_BEGIN */
END			{ return KW_END; }		/* KEYWORD_END */
FUNCTION		{ return KW_FUNCTION; }		/* KEYWORD_FUNCTION */
READ			{ return KW_READ; }		/* KEYWORD_READ */
WRITE			{ return KW_WRITE; }		/* KEYWORD_WRITE */
IF			{ return KW_IF; }		/* KEYWORD_IF */
ELSE			{ return KW_ELSE; }		/* KEYWORD_ELSE */
FI			{ return KW_FI; }		/* KEYWORD_FI */
FOR			{ return KW_FOR; }		/* KEYWORD_FOR */
ROF			{ return KW_ROF; }		/* KEYWORD_ROF */
BREAK			{ return KW_BREAK; }		/* KEYWORD_BREAK */
CONTINUE		{ return KW_CONTINUE; }		/* KEYWORD_CONTINUE */
RETURN			{ return KW_RETURN; }		/* KEYWORD_RETURN */
INT			{ return KW_INT; }		/* KEYWORD_INT */
VOID			{ return KW_VOID; }		/* KEYWORD_VOID */
STRING			{ return KW_STRING; }		/* KEYWORD_STRING */
FLOAT			{ return KW_FLOAT; }		/* KEYWORD_FLOAT */

":="			{ return OP_ASSIGN; }		/* OPERATOR_ASSIGN */
"+"			{ return OP_ADD; }		/* OPERATOR_ADD */
"-"			{ return OP_SUB; }		/* OPERATOR_SUB */
"*"			{ return OP_MUL; }		/* OPERATOR_MUL */
"/"			{ return OP_DIV; }		/* OPERATOR_DIV */
"="			{ return OP_EQ; }		/* OPERATOR_EQ */
"!="			{ return OP_NEQ; }		/* OPERATOR_NEQ */
"<"			{ return OP_LT; }		/* OPERATOR_LT */
">"			{ return OP_GT; }		/* OPERATOR_GT */
"("			{ return OP_OP; }		/* OPERATOR_OP */
")"			{ return OP_CP; }		/* OPERATOR_CP */
";"			{ return OP_SEMICOLON; }	/* OPERATOR_SEMICOLON */
","			{ return OP_COMMA; }		/* OPERATOR_COMMA */
"<="			{ return OP_LEQ; }		/* OPERATOR_LEQ */
">="			{ return OP_GEQ; }		/* OPERATOR_GEQ */

{IDENTIFIER} 		{ return IDENTIFIER; } 
{DIGIT}+ 		{ return INTLITERAL; }
{DIGIT}*"."{DIGIT}+ 	{ return FLOATLITERAL; }
\"[^\"]*\" 		{ return STRINGLITERAL; }
{COMMENT} 		{}
{WHITESPACE}+		{}
.			{}

%%

int yywrap(){
	return 1;
}
