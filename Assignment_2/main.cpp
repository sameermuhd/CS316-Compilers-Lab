#include <iostream>
#include <stdio.h>
#include "parser.tab.hpp"
using namespace std;

extern FILE* yyin;
int yylex();
int yyparse();

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
	if(status == 0){
		cout << "Accepted\n";
	}
	else if(status == 1){
		cout << "Not accepted\n";
	}
	return 0;
}
