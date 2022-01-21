
DIGIT		[0-9]
IDENTIFIER 	[a-zA-Z][a-zA-Z0-9]*
COMMENT		--.*\n
WHITESPACE 	[ \r\n\t]

%%

PROGRAM			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }  	/* KEYWORD_PROGRAM */
BEGIN			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_BEGIN */
END			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_END */
FUNCTION		{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_FUNCTION */
READ			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_READ */
WRITE			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_WRITE */
IF			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_IF */
ELSE			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_ELSE */
FI			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_FI */
FOR			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_FOR */
ROF			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_ROF */
BREAK			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_BREAK */
CONTINUE		{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_CONTINUE */
RETURN			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_RETURN */
INT			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_INT */
VOID			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_VOID */
STRING			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_STRING */
FLOAT			{ fprintf(yyout, "Token Type: KEYWORD\nValue: %s\n", yytext); }		/* KEYWORD_FLOAT */

":="			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_ASSIGN */
"+"			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_ADD */
"-"			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_SUB */
"*"			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_MUL */
"/"			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_DIV */
"="			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_EQ */
"!="			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_NEQ */
"<"			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_LT */
">"			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_GT */
"("			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_OP */
")"			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_CP */
";"			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_SEMICOLON */
","			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_COMMA */
"<="			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_LEQ */
">="			{ fprintf(yyout, "Token Type: OPERATOR\nValue: %s\n", yytext); }	/* OPERATOR_GEQ */

{IDENTIFIER} 		{ fprintf(yyout, "Token Type: IDENTIFIER\nValue: %s\n", yytext); } 
{DIGIT}+ 		{ fprintf(yyout, "Token Type: INTLITERAL\nValue: %d\n", atoi(yytext)); }
{DIGIT}*"."{DIGIT}+ 	{ fprintf(yyout, "Token Type: FLOATLITERAL\nValue: %s\n", yytext); }
\"[^\"]*\" 		{ fprintf(yyout, "Token Type: STRINGLITERAL\nValue: %s\n", yytext); }
{COMMENT} 		{}
{WHITESPACE}+		{}
.			{}

%%

int yywrap(){
	return 1;
}

int main(int argc, char* argv[]) {
	if(argc > 1){
		FILE *inFile = fopen(argv[1], "r");
		FILE *outFile = fopen(argv[2], "w");
		if(inFile)
			yyin = inFile;
		if(outFile)
			yyout = outFile;
	}
	yylex();
	return 0;
}
