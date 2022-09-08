// Libraries Used
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <stack>
#include <iterator>
#include <sstream>

// Header files
#include "headers/astNode.h"				// AST node class
#include "headers/threeAC.h"				// Three AC class
#include "headers/tinyNode.h"				// Tiny node class
#include "main.h"					// main header with a few structs
#include "parser.tab.h"					// Including the parser header

using namespace std;

/*===========================================================================*
 *			Data structures defined in parser 	   	     *
 *===========================================================================*/

extern FILE* yyin;
extern struct wrapper p;						// Wrapper from main.h header
extern map <std::string, map<std::string, wrapper> > symbolTable;	// Symbol Table from the parser
extern vector <ThreeAC> IR;						// ThreeAC from the parser
extern int blockCounter;						// Counter to number the blocks used
extern int registerCounter;						// Counter to number the registers used
extern int localCounter;						// Counter to number the local variables of a function
extern int paramCounter;						// Counter to number the pararmeters of a function
extern stack <std::string> scope;					// Stack to hold the current valid scopes during parsing
extern map <std::string, wrapper> table;				// Symbol Table of current scope
extern std::vector <std::string> strConst, vars;			// All of these store vatiable names. Used to deeclare variable names at the start of tiny code
extern std::map <std::string, std::string> varMap;  			// Map of variable names to local/parameter variable identifiers
extern std::map <std::string, std::vector <ThreeAC> > funcIR;		// Maps a function name to a vector of its IR/Three AC nodes
extern std::map <std::string, info> funcInfo; 				// Map from a function name to a struct holding information about the function
extern struct info f;							// Info struct from main.h to hold the total parameters and local variables count

/*===========================================================================*
 *			Data structures defined 		 	     *
 *===========================================================================*/
stack <string> registers;						// Stack to keep track of registers in an expression
std::stack <std::string> scopeHelp;					// Stack holding variable not declated in current scope
std::vector <TinyNode> assembly;					// Vector holding the tiny nodes for tiny code
extern stringstream ss;							// Temporary string to print ints or floats

/*===========================================================================*
 *			Functions defined 				     *
 *===========================================================================*/
int yylex();
int yyparse();

// Function that prints error messages
void yyerror(char const* msg){
	cout << msg << "\n";
	exit(1);
}

void pushBlock();					// Pushes a block onto the scope stack
void addSymbolTable();					// Adds the table for a single scope to the symbol table
string ExprIR(ASTNode * node, string * t);		// Generates the 3 AC for an expression and returns the register holding the final value. 't' is the data type
void makeIR(ASTNode * node); 				// Generates the 3 AC/IR of assign statements
void removeAST(ASTNode * node);				// Removes/destroys an AST tree
map <string, wrapper> findSymbolTable(string ID); 	// Checks for the existence of ID in any of the currently valid scopes
void functionIRSetup(string functionID);		// Reset the register/offset counters and add IR code for the start of a function

void generateCMPI(string op1, string op2, string savedReg, string outputRegister, int * currentReg, string funcName);
				// Takes the IR for an integer compare and generates assembly code
void generateCMPR(string op1, string op2, string savedReg, string outputRegister, int * currentReg, string funcName);
				// Takes the IR for a float compare and generates assembly code
void generateADD(string opcode, string op1, string op2, int * currentReg, string * addopTemp, string * mulopTemp, string * outputRegister, string funcName);
				// Takes the IR for an addop and turns it into assembly code
void generateMUL(string opcode, string op1, string op2, int * currentReg, string * temp, string * outputRegister, string funcName);
				// Takes the IR for a mulop and turns it into assembly code
string tinyOpr(string funcName, string opr, int regNum);	
				// Takes a function name and a register/stack value from the IR and returns a string with the tiny register/stack value
void ThreeACtoTiny(string fID);			
				// Takes a function name and translates the 3 AC of that function to tiny nodes

void printSymbolTable();				// Prints all of the symbol tables
void printThreeACode();					// Prints the 3 AC code
void printTinyCode();					// Prints the tiny code

/*===========================================================================*
 *				main functions  	  		     *
 *===========================================================================*/
int main(int argc, char * argv[]) {
	// Opening the input file
	if(argc > 1){
		FILE *inFile = fopen(argv[1], "r");
		if(inFile)
			yyin = inFile;
	}
	else {
		return -1;
	}

	yyparse();	// Parsing the input

	//printSymbolTable();		// Uncomment to print the symbol tables
	printThreeACode();		// Printing the 3 AC as tiny comments
	printTinyCode();		// Printing the tiny code

	return 0;
}

/*===========================================================================*
 *				pushBlock	 	  	  	     *
		  Pushes a block onto the scope stack
 *===========================================================================*/
void pushBlock() {
	ss.str("");
	ss << "BLOCK " << ++blockCounter;
	scope.push(ss.str());
}

/*===========================================================================*
 *				addSymbolTable	  	  		     *
	Adds the table for a single scope to the symbol table
 *===========================================================================*/
void addSymbolTable() {
	symbolTable[scope.top()] = table;
	table.clear();
}

/*===========================================================================*
 *				functionIRSteup	  	  		     *
Reset the register/offset counters and add IR code for the start of a function
 *===========================================================================*/
void functionIRSetup(string functionID) {
	registerCounter = 0;
	localCounter = 0;
	paramCounter = 0;

	IR.push_back(ThreeAC("LABEL", "", "", functionID));
	IR.push_back(ThreeAC("LINK", "", "", ""));
}

/*===========================================================================*
 *				findSymbolTable	   			     *
     Checks for the existence of ID in any of the currently valid scopes
 *===========================================================================*/
map <string, wrapper> findSymbolTable(string ID){
	map <string, wrapper> m;
	while(!scope.empty()){		// Checking every available scope
		m = symbolTable[scope.top()];
		// ID found!
		if (m.count(ID) > 0)
			break;
		// ID not found, save the current scope and check the next one
		else {
			scopeHelp.push(scope.top());
			scope.pop();
		}
	}
	// ID never found and is used illegally
	if(scope.empty()){
		string error = "Variable " + ID + "is not found in any scope";
		yyerror(error.c_str());
	}
	//  Put all the saved scopes back in order
	while(!scopeHelp.empty()){
		scope.push(scopeHelp.top());
		scopeHelp.pop();
	}

	return m;
}

/*===========================================================================*
 *				ExprIR	  	  			     *
Generates the 3 AC for an expression and returns the register holding the final
			value. 't' is the data type
 *===========================================================================*/
string ExprIR(ASTNode * node, string * t) {
	if(node != NULL){
		ExprIR(node->left, t);
		ExprIR(node->right, t);
		ss.str("");
		if(node->nodeType == "OP") {
			node->dataType = node->left->dataType;
			ss << "$T" << registerCounter++;
			node->reg = ss.str();
		}
		if(node->nodeType == "CONST") {
			ss << "$T" << registerCounter++;
			node->reg = ss.str();
		}
		if(node->nodeType == "VAR"){
			// If the variable is part of the current stack frame, change the register to the correct value
			if(varMap.count(node->value) > 0){
				node->reg = varMap[node->value];
			}
		}
		IR.push_back(node->generateCode());
		*t = node->dataType;
		return node->reg;
	}
	*t = "NONE";
	return "";
}

/*===========================================================================*
 *				makeIR	  	  		  	     *
		Generates the 3 AC/IR of assign statements
 *===========================================================================*/
void makeIR(ASTNode * node) {
	if (node != NULL){
		makeIR(node->left);
		makeIR(node->right);
		ss.str("");
		if (node->nodeType == "OP"){
			node->dataType = node->left->dataType;
			if (node->value != "="){
				ss << "$T" << registerCounter++;
				node->reg = ss.str();
			}
			else
				node->reg = node->left->reg;
		}
		if(node->nodeType == "CONST"){
			ss << "$T" << registerCounter++;
			node->reg = ss.str();
		}
		if(node->nodeType == "VAR") {
			if(varMap.count(node->value) > 0) {
				node->reg = varMap[node->value];
			}
		}
		IR.push_back(node->generateCode());
	}
}

/*===========================================================================*
 *				removeAST		  	    	     *
			Removes/destroys an AST treee
 *===========================================================================*/
void removeAST(ASTNode * node) {
	if (node != NULL){
		removeAST(node->left);
		removeAST(node->right);
		delete node;
	}
}

/*===========================================================================*
 *				tinyOpr			  	  	     *
  Takes a function name and a register/stack value from the IR and returns a
 			string with the tiny register/stack value
 *===========================================================================*/
string tinyOpr(string funcName, string opr, int regNum) {
	if (opr[0] == '$') {		// the operand is a register or stack value
		if (opr[1] == 'T') {	// the operand is a register
			ss.str("");
			ss << "r" << regNum;
			string t = ss.str();
			ss.str("");

			return t;
		}
		else if (opr[1] == 'L') {	// the operand is a local variable
			string t = "$-" + opr.substr(2, string::npos);
			return t;
		}
		else if (opr[1] == 'R') {	 // the operand is a return value
			ss.str("");
			ss << "$" << funcInfo[funcName].P_count + 6;
			string t  = ss.str();
			ss.str("");

			return t;
		}
		else {				// the operand is a parameter
			string t = opr.substr(2, string::npos);
			ss.str("");
			ss << "$" << atoi(t.c_str()) + 5;
			t = ss.str();
			ss.str("");

			return t;
		}
	}
	else
		return opr;			// the operand is a global variable
}

/*===========================================================================*
 *				generateCMPI				     *
		Adds the table for a single scope to the symbol table
 *===========================================================================*/
void generateCMPI(string op1, string op2, string savedReg, string outputRegister, int * currentReg, string funcName) {
    if (op1[0] != '$' && op2[0] != '$' || (op1[1] != 'T' && op2[1] != 'T')) {	// op1 and op2 are variables or stack values
		ss << "r" << *currentReg;
		string t = ss.str();
		ss.str("");
		assembly.push_back(TinyNode("move", tinyOpr(funcName, op2, *currentReg), t));
		outputRegister = t;
		*currentReg = *currentReg + 1;
		assembly.push_back(TinyNode("cmpi", tinyOpr(funcName, op1, *currentReg), outputRegister));
	}
	else if (op1[0] != '$' || op1[1] != 'T') {	// only op1 is variable or stack value
		assembly.push_back(TinyNode("cmpi", tinyOpr(funcName, op1, *currentReg), outputRegister));
	}
	else if (op2[0] != '$' || op2[1] != 'T') {	// only op2 is variable or stack value
		assembly.push_back(TinyNode("cmpi", outputRegister, tinyOpr(funcName, op2, *currentReg)));
	}
	else {		// both ops are registers
		assembly.push_back(TinyNode("cmpi", savedReg, outputRegister));
	}
}

/*===========================================================================*
 *				generateCMPR	   			     *
	Takes the IR for a float compare and generates assembly code
 *===========================================================================*/
void generateCMPR(string op1, string op2, string savedReg, string outputRegister, int * currentReg, string funcName) {
    if (op1[0] != '$' && op2[0] != '$' || (op1[1] != 'T' && op2[1] != 'T')) {	// op1 and op2 are variables or stack values
		ss << "r" << *currentReg;
		string t = ss.str();
		ss.str("");
		assembly.push_back(TinyNode("move", tinyOpr(funcName, op2, *currentReg), t));
		outputRegister = t;
		*currentReg = *currentReg + 1;
		assembly.push_back(TinyNode("cmpr", tinyOpr(funcName, op1, *currentReg), outputRegister));
	}
	else if (op1[0] != '$' || op1[1] != 'T') {	// only op1 is variable or stack value
		assembly.push_back(TinyNode("cmpr", tinyOpr(funcName, op1, *currentReg), outputRegister));
	}
	else if (op2[0] != '$' || op2[1] != 'T') {	// only op2 is variable or stack value
		assembly.push_back(TinyNode("cmpr", outputRegister, tinyOpr(funcName, op2, *currentReg)));
	}
	else {		// both ops are registers
		assembly.push_back(TinyNode("cmpr", savedReg, outputRegister));
	}
}

/*===========================================================================*
 *				generateADD				     *
	Takes the IR for an addop and turns it into assembly code
 *===========================================================================*/
void generateADD(string opcode, string op1, string op2, int * currentReg, string * addopTemp, string * mulopTemp, string * outputRegister, string funcName) {
    if (op1[0] != '$' || op1[1] != 'T') {	// op1 is a variable or a stack value
		ss << "r" << *currentReg;
		string t = ss.str();
		ss.str("");
		assembly.push_back(TinyNode("move", tinyOpr(funcName, op1, *currentReg), t));

		ss << "r" << *currentReg - 1;
		*addopTemp = ss.str();
		ss.str("");

		*currentReg = *currentReg + 1;

		if (op2[0] != '$' || op2[1] != 'T') {	// op2 is a variable or stack value
			assembly.push_back(TinyNode(opcode, tinyOpr(funcName, op2, *currentReg - 1), t));
		}
		else {		// op2 is a register
			assembly.push_back(TinyNode(opcode, *addopTemp, t));
			if (!registers.empty()) 
			{
				registers.pop();
			}
		}
		registers.push(t);
		*outputRegister = t;
		*addopTemp = t;
	}
	else {		// op1 is a register
		if (op2[0] != '$' || op2[1] != 'T') {	// op2 is a variable or a stack value
			ss << "r" << *currentReg - 1; 
			string t = ss.str();
			ss.str("");
			assembly.push_back(TinyNode(opcode, tinyOpr(funcName, op2, *currentReg - 1), t));
			*outputRegister = t;
		}
		else {	// op2 is a register
			while (!registers.empty())
			{
				*addopTemp = registers.top();
				registers.pop();
			}
			assembly.push_back(TinyNode(opcode, tinyOpr(funcName, op1, *currentReg - 1), *addopTemp));
			*outputRegister = *addopTemp;
			registers.push(*addopTemp);
		}
	}
	*mulopTemp = *addopTemp;
}

/*===========================================================================*
 *				generateMUL		  		     *
	Takes the IR for a mulop and turns it into assembly code
 *===========================================================================*/
void generateMUL(string opcode, string op1, string op2, int * currentReg, string * temp, string * outputRegister, string funcName) {
    if (op1[0] != '$' || op1[1] != 'T') {	 // op1 is a variable or a stack value
		ss << "r" << *currentReg;
		string t = ss.str();
		ss.str("");
		assembly.push_back(TinyNode("move", tinyOpr(funcName, op1, *currentReg), t));

		ss << "r" << *currentReg - 1;
		*temp = ss.str();
		ss.str("");

		*currentReg = *currentReg + 1;

		if (op2[0] != '$' || op2[1] != 'T') {	// op2 is a variable or a stack value
			assembly.push_back(TinyNode(opcode, tinyOpr(funcName, op2, *currentReg - 1), t));
		}
		else {		// op2 is a register
			assembly.push_back(TinyNode(opcode, *temp, t));
			if (!registers.empty()) 
			{
				registers.pop();
			}
		}
		registers.push(t);
		*outputRegister = t;
		*temp = t;
	}
	else {		// op1 is a register
		if (op2[0] != '$' || op1[1] != 'T') {	// op2 is a variable or a stack value
			ss << "r" << *currentReg - 1;
			string t  = ss.str();
			ss.str("");
			assembly.push_back(TinyNode(opcode, tinyOpr(funcName, op2, *currentReg - 1), t));
			*outputRegister = t;
		}
		else {	// op2 is a register
			while (!registers.empty()) {
				*temp = registers.top();
				registers.pop();
			}
			assembly.push_back(TinyNode(opcode, tinyOpr(funcName, op1, *currentReg - 1), *temp));
			*outputRegister = *temp;
			registers.push(*temp);
		}
	}
}

/*===========================================================================*
 *			printSymbolTable  	  			     *
		Prints all of the symbol tables
 *===========================================================================*/
void printSymbolTable() {
	for(map <string, map<string, wrapper> >::iterator itr = symbolTable.begin(); itr != symbolTable.end(); ++itr){
		cout << "Symbol table " << itr->first << "\n";
		map <string, wrapper> &internal_map = itr->second;
		for(map <string, wrapper>::iterator itr2 = internal_map.begin(); itr2 != internal_map.end(); ++itr2){
			p = itr2->second;
			if(p.value[0] == "STRING")
				cout << "name " << itr2->first << " type " << p.value[0] << " value " << p.value[1] << "\n";
			else
				cout << "name " << itr2->first << " type " << p.value[0] << "\n";
		}
		cout << "\n";
	}
}

/*===========================================================================*
 *			    printThreeACode	 			     *
			 Prints the 3 AC code
 *===========================================================================*/
void printThreeACode() {
	cout << ";IR code" << "\n";
	cout << "\n";
	for (map <string, vector <ThreeAC> >::iterator itr = funcIR.begin(); itr != funcIR.end(); ++itr) {
		vector <ThreeAC> &func = itr->second;
		for (vector <ThreeAC>::iterator itr2 = func.begin(); itr2 != func.end(); ++itr2){
			itr2->printThreeAC();
		}
		cout << "\n";
	}
}

/*===========================================================================*
 *				printTinyCode	  			     *
			     Prints the tiny code
 *===========================================================================*/
void printTinyCode() {
	cout << ";Tiny Code" << "\n";
	// Printing all global int/float variable used
	for (vector<string>::iterator itr = vars.begin(); itr != vars.end(); ++itr) {
	    cout << "var " << *itr << "\n";
	}

	// Printing all global string variable used
	for (vector<string>::iterator itr = strConst.begin(); itr != strConst.end(); ++itr) {
	    cout << *itr << "\n";
	}

	// Tiny code to enter the main function
	assembly.push_back(TinyNode("push", "", ""));
	assembly.push_back(TinyNode("push", "", "r0"));
	assembly.push_back(TinyNode("push", "", "r1"));
	assembly.push_back(TinyNode("push", "", "r2"));
	assembly.push_back(TinyNode("push", "", "r3"));
	assembly.push_back(TinyNode("jsr", "", "main"));
	assembly.push_back(TinyNode("sys halt", "", ""));

	// Converting the 3 AC to tiny code
	ThreeACtoTiny("main");
	for(map <string, vector <ThreeAC> >::iterator itr = funcIR.begin(); itr != funcIR.end(); ++itr){
		if(itr->first != "main")
			ThreeACtoTiny(itr->first);
	}

	// Loop through the tiny nodes in order and printing the tiny code
	for(vector <TinyNode>::iterator itr = assembly.begin(); itr != assembly.end(); ++itr){
		itr->printNode();
	}
}

/*===========================================================================*
 *			ThreeACToTiny	  	  			     *
 Takes a function name and translates the 3 AC of that function to tiny nodes
 *===========================================================================*/
void ThreeACtoTiny(string fID) {
	vector <ThreeAC> node = funcIR[fID];
	f = funcInfo[fID];

	int currentRegister = 0;
	string outputRegister = "";
	string addopTemp = "";
	string mulopTemp = "";
	string code, op1, op2, result, savedReg;

	for (vector<ThreeAC>::iterator itr = node.begin(); itr != node.end(); ++itr) {
		code = itr->opCode;
		op1 = itr->operand1;
		op2 = itr->operand2;
		result = itr->result;

	    if (itr->cmpType == "SAVE") {	// This saves the register holding the value of the left side of a compare
			savedReg = outputRegister;
		}
		// This if-else chain checks the 3 AC and generates the corresponding tiny node.
		if (code == "WRITEI") {
			assembly.push_back(TinyNode("sys writei", tinyOpr(fID, result, currentRegister), ""));
		}
		else if (code == "WRITEF") {
			assembly.push_back(TinyNode("sys writer", tinyOpr(fID, result, currentRegister), ""));
		}
		else if (code == "WRITES") {
			assembly.push_back(TinyNode("sys writes", tinyOpr(fID, result, currentRegister), ""));
		}
		else if (code == "READI") {
			assembly.push_back(TinyNode("sys readi", tinyOpr(fID, result, currentRegister), ""));
		}
		else if (code == "READF") {
			assembly.push_back(TinyNode("sys readr", tinyOpr(fID, result, currentRegister), ""));
		}
		else if (code == "JUMP") {
			assembly.push_back(TinyNode("jmp", result, ""));
		}
	    else if (code == "GT") {
			if (itr->cmpType == "INT") {
				generateCMPI(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}
			else {
				generateCMPR(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}

			assembly.push_back(TinyNode("jgt", result, ""));
			while (!registers.empty())
				registers.pop();
		}
		else if (code == "GE") {
			if (itr->cmpType == "INT") {
				generateCMPI(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}
			else {
				generateCMPR(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}

			assembly.push_back(TinyNode("jge", result, ""));
			while (!registers.empty())
				registers.pop();
		}
		else if (code == "LT") {
			if (itr->cmpType == "INT") {
				generateCMPI(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}
			else {
				generateCMPR(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}

			assembly.push_back(TinyNode("jlt", result, ""));
			while (!registers.empty())
				registers.pop();
		}
		else if (code == "LE") {
			if (itr->cmpType == "INT"){
				generateCMPI(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}
			else {
				generateCMPR(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}

			assembly.push_back(TinyNode("jle", result, ""));
			while (!registers.empty())
				registers.pop();
		}
		else if (code == "NE") {
			if (itr->cmpType == "INT") {
				generateCMPI(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}
			else {
				generateCMPR(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}

			assembly.push_back(TinyNode("jne", result, ""));
			while (!registers.empty())
				registers.pop();
		}
		else if (code == "EQ") {
			if (itr->cmpType == "INT") {
				generateCMPI(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}
			else {
				generateCMPR(op1, op2, savedReg, outputRegister, &currentRegister, fID);
			}

			assembly.push_back(TinyNode("jeq", result, ""));
			while (!registers.empty())
				registers.pop();
		}
	    else if (code == "LABEL") {
			assembly.push_back(TinyNode("label", result, ""));
		}
		else if (code == "JSR") {
			assembly.push_back(TinyNode("push" , "r0", ""));
			assembly.push_back(TinyNode("push" , "r1", ""));
			assembly.push_back(TinyNode("push" , "r2", ""));
			assembly.push_back(TinyNode("push" , "r3", ""));
			assembly.push_back(TinyNode("jsr", result, ""));
			assembly.push_back(TinyNode("pop" , "r3", ""));
			assembly.push_back(TinyNode("pop" , "r2", ""));
			assembly.push_back(TinyNode("pop" , "r1", ""));
			assembly.push_back(TinyNode("pop" , "r0", ""));
		}
		else if (code == "PUSH") {
			assembly.push_back(TinyNode("push", tinyOpr(fID, result, currentRegister - 1), ""));
		}
		else if (code == "POP") {
			assembly.push_back(TinyNode("pop", tinyOpr(fID, result, currentRegister), ""));
			outputRegister =  tinyOpr(fID, result, currentRegister);
			currentRegister++;
		}
		else if (code == "LINK") {
			ss.str("");
			ss << f.L_count;
			assembly.push_back(TinyNode("link", ss.str(), ""));
			ss.str("");
		}
		else if (code == "RET") {
			assembly.push_back(TinyNode("unlnk", "", ""));
			assembly.push_back(TinyNode("ret", "", ""));
		}
	    else if (code == "STOREI" || code == "STOREF")
		{
			if (result[0] != '$' || result[1] != 'T') {	// storing into a variable or stack value
				if (op1[0] != '$' || op1[1] != 'T') {	// storing a variable or stack value
						ss << "r" << currentRegister;
						string t = ss.str();
						ss.str("");
						assembly.push_back(TinyNode("move", tinyOpr(fID, op1, currentRegister), t));
						assembly.push_back(TinyNode("move", t, tinyOpr(fID, result, currentRegister)));
						currentRegister++;
				}
				else {	// storing a register
					assembly.push_back(TinyNode("move", outputRegister, tinyOpr(fID, result, currentRegister)));
					while (!registers.empty())
						registers.pop();
				}
			}
			else {	// storing into a register
				assembly.push_back(TinyNode("move", op1, tinyOpr(fID, result, currentRegister)));
				outputRegister = tinyOpr(fID, result, currentRegister);
				registers.push(outputRegister);
				currentRegister++;
			}
		}
		else if (code == "ADDI") {
			generateADD("addi", op1, op2, &currentRegister, &addopTemp, &mulopTemp, &outputRegister, fID);
		}
		else if (code == "ADDF") {
			generateADD("addr", op1, op2, &currentRegister, &addopTemp, &mulopTemp, &outputRegister, fID);
		}
		else if (code == "SUBI") {
			generateADD("subi", op1, op2, &currentRegister, &addopTemp, &mulopTemp, &outputRegister, fID);
		}
		else if (code == "SUBF") {
			generateADD("subr", op1, op2, &currentRegister, &addopTemp, &mulopTemp, &outputRegister, fID);
		}
		else if (code == "MULTI") {
			generateMUL("muli", op1, op2, &currentRegister, &mulopTemp, &outputRegister, fID);
		}
		else if (code == "MULTF") {
			generateMUL("mulr", op1, op2, &currentRegister, &mulopTemp, &outputRegister, fID);
		}
		else if (code == "DIVI") {
			generateMUL("divi", op1, op2, &currentRegister, &mulopTemp, &outputRegister, fID);
		}
		else if (code == "DIVF") {
			generateMUL("divr", op1, op2, &currentRegister, &mulopTemp, &outputRegister, fID);
		}
	}
}
