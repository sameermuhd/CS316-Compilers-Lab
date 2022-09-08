#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include "parser.tab.hpp"
#include "symbolTable.h"

using namespace std;

extern FILE* yyin;
int yylex();
int yyparse();

extern SymbolTable* SymTable;

void yyerror(char const* msg){
	return;
}

int main(int argc, char* argv[]) {
	if(argc > 1){
		FILE *inFile = fopen(argv[1], "r");
		if(inFile)
			yyin = inFile;
	}
	int status = yyparse();
	SymTable->printSymbolTable();
	return 0;
}
