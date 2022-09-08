// Libraries Used
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <stack>
#include <iterator>
#include <sstream>
#include <regex>
#include <unordered_set>

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
extern struct wrapper p;					// Wrapper from main.h header
extern map <std::string, map<std::string, wrapper> > symbolTable;// Symbol Table from the parser
extern vector <ThreeAC> IR;					// ThreeAC from the parser
extern int blockCounter;					// Counter to number the blocks used
extern int registerCounter;					// Counter to number the registers used
extern int localCounter;					// Counter to number the local variables of a function
extern int paramCounter;					// Counter to number the pararmeters of a function
extern stack <std::string> scope;				// Stack to hold the current valid scopes during parsing
extern map <std::string, wrapper> table;			// Symbol Table of current scope
extern std::vector <std::string> strConst, vars;		// All of these store vatiable names. Used to deeclare variable names at the start of tiny code
extern std::map <std::string, std::string> varMap;  		// Map of variable names to local/parameter variable identifiers
extern std::map <std::string, std::vector <ThreeAC> > funcIR;	// Maps a function name to a vector of its IR/Three AC nodes
extern std::map <std::string, info> funcInfo; 			// Map from a function name to a struct holding information about the function
extern struct info f;						// Info struct from main.h to hold the total parameters and local variables count

/*===========================================================================*
 *			Data structures defined 		 	     *
 *===========================================================================*/
stack <string> registers;					// Stack to keep track of registers in an expression
std::stack <std::string> scopeHelp;				// Stack holding variable not declated in current scope
std::vector <TinyNode> assembly;				// Vector holding the tiny nodes for tiny code
extern stringstream ss;						// Temporary string to print ints or floats
vector<int> leadersList;					// Vector holding the leaders index in the 3 AC
vector<ThreeAC *> IRList;					// All functions 3 AC combined into IR List
string register4R[4];						// Array of 4 registers used, keeps track of variable name
bool dirty4R[4];						// Bool array to keep track of dirty registers
int Index = 0;							// The index of 3 AC
int localNum = 0;						// Keeps count of total local variables of a function
int paraNum = 0;						// Keeps count of total parameters of a function
int tempNum = 0;						// Keeps count of total temporaries used by a function
string currLabel;						// String to hold current function name
vector<string> usedList;					// Vector to keep track of temporaries used

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

void pushBlock();				// Pushes a block onto the scope stack
void addSymbolTable();				// Adds the table for a single scope to the symbol table
string ExprIR(ASTNode * node, string * t);	// Generates the 3 AC for an expression and returns the register holding the final value. 't' is the data type
void makeIR(ASTNode * node); 			// Generates the 3 AC/IR of assign statements
void removeAST(ASTNode * node);			// Removes/destroys an AST tree
map <string, wrapper> findSymbolTable(string ID);// Checks for the existence of ID in any of the currently valid scopes
void functionIRSetup(string functionID);	// Reset the register/offset counters and add IR code for the start of a function

void printSymbolTable();			// Prints all of the symbol tables
void printThreeACode();				// Prints the 3 AC code

void generateCFGraph();				// Function to generate CFG or more precisely generating successor and predecessor of each IR Node
void generateGenKill();				// Function to generate Gen / Kill set
void generateLeaders();				// Function to find leaders in 3 AC
void generateInOut();				// Function to generate LiveIn and LiveOut sets
void initializeRegAllocation();			// Function that initializes the register allocation
void performRegAllocation();			// Function that performs register allocation and converts 3 AC to tiny code
int ensure(string input, int idx);		// Function used in register allocation, loads a variable into a register 
int allocate(string input, int idx);		// Allocates a register to given vatiable
void free(int i);				// Frees a register
void spill();					// Function to spill all the registers
string calTinyReg(string str);			// Function to calculate variable location w.r.t base pointer
string getOp(string opCode);			// 3 AC to tiny opcode conversion
void registersStatus();				// Prints the status of all registers

bool contains_key(string fID) { return funcInfo.find(fID) != funcInfo.end(); }
				// Function to check if a given function exists in code

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
	printThreeACode();		// Printing 3 AC
	generateCFGraph();		// Generating CFG, or more precisely generating successor and predecessor of each IR Node
	generateGenKill();		// Generating gen/kill sets
	generateInOut();		// Generating LiveIn and LiveOut sets
	initializeRegAllocation();	// Register allocation initialization
	performRegAllocation();		// Performing register allocation and converting & priting 3 AC to tiny code
	
	return 0;
}

/*===========================================================================*
 *				generateCFGraph	 	  	  	     *
     Function to generate CFG or more precisely generating successor and 
			predecessor of each IR Node
 *===========================================================================*/
void generateCFGraph() {
	for (map <string, vector <ThreeAC> >::iterator itr = funcIR.begin(); itr != funcIR.end(); ++itr) {
		vector <ThreeAC> &func = itr->second;
		for(int i = 0; i < func.size(); i++) {	// Iteraing through 3 AC of all functions
			ThreeAC* irNode = &func[i];
			if(irNode->opCode == "")	// Empty IRNode, continue
				continue;
			if(irNode->opCode == "JUMP") {	// JUMP has just one successor, its target
				for(int j = 0; j < func.size(); j++) {
					ThreeAC* destNode = &func[j];
					if(destNode->opCode == "") {
						continue;
					}
					if((destNode->opCode == "LABEL") && (irNode->result == destNode->result)) {
						(irNode->successor).push_back(destNode);
					}
				}
			}
			else if((irNode->opCode == "LE") || (irNode->opCode == "GE") || (irNode->opCode == "NE")  || \
				(irNode->opCode == "GT") || (irNode->opCode == "LT") || (irNode->opCode == "EQ")) {
						// These nodes have 2 successors
				for(int j = 0; j < func.size(); j++) {
					ThreeAC* destNode = &func[j];
					if(destNode->opCode == "") {
						continue;
					}
					if((destNode->opCode == "LABEL") && (irNode->result == destNode->result)) {
						(irNode->successor).push_back(destNode);
					}
				}
				for(int j = i+1; j < func.size(); j++) {
					ThreeAC* destNode2 = &func[j];
					if(destNode2->opCode == "")
						continue;
					else {
						(irNode->successor).push_back(destNode2);
						break;
					}
				}
			}
			else {	// If none of above nodes, then one successor
				for(int j = i+1; j < func.size(); j++) {
					ThreeAC* destNode = &func[j];
					if(destNode->opCode == "")
						continue;
					else {
						(irNode->successor).push_back(destNode);
						break;
					}
				}
			}
		}

		// Now we iterate via all successors and find predecessors
		for(int i = 0; i < func.size(); i++) {
			ThreeAC* irNode = &func[i];
			if(irNode->opCode == "")
				continue;
			
			vector<ThreeAC *> successors = irNode->successor;
			for(int j = 0; j < successors.size(); j++) {
				ThreeAC* destNode = successors[j];
				vector <ThreeAC *> predecessors = destNode->predecessor;
				bool found = false;
				for(int k = 0; k < predecessors.size(); k++) {
					ThreeAC* node = predecessors[k];
					if(node == irNode){
						found = true;
						break;
					}
				}
				if(!found){
					(destNode->predecessor).push_back(irNode);
				}
			}
		}
	}
}

/*===========================================================================*
 *				generateGenKill	 	  	  	     *
		      Function to generate Gen / Kill set
 *===========================================================================*/
void generateGenKill() {
	for (map <string, vector <ThreeAC> >::iterator itr = funcIR.begin(); itr != funcIR.end(); ++itr) {
		vector <ThreeAC> &func = itr->second;
		for(int i = 0; i < func.size(); i++){		// Iteraing through 3 AC of all functions
			ThreeAC* irNode = &func[i];
			string opCode = irNode->opCode;
			string operand1 = irNode->operand1;
			string operand2 = irNode->operand2;
			string result = irNode->result;
			if(opCode == "") {
				continue;
			}
			if(opCode == "PUSH" || opCode == "WRITE") {
				if(result != "") {
					(irNode->genSet).insert(result);
				}
			} 
			else if(opCode == "POP" || opCode == "READ") {
				if(result != "") {
					(irNode->killSet).insert(result);
				}
			} 
			else if(opCode == "LE" || opCode == "GE" || opCode == "NE" \
				|| opCode == "GT" || opCode == "LT" || opCode == "EQ") {
				(irNode->genSet).insert(operand1);
				(irNode->genSet).insert(operand2);
			} 
			else if(opCode == "JSR") {
				for(string str : strConst) {
					(irNode->genSet).insert(str);
				}
			} 
			else if(opCode == "STOREI" || opCode == "STOREF") {
				regex intNum("[0-9]+");
				regex floatNum("[0-9]*\\.[0-9]+");
				if(operand1 != "" && !(regex_match(operand1.begin(), operand1.end(), intNum) || \
						regex_match(operand1.begin(), operand1.end(), floatNum))){
					(irNode->genSet).insert(operand1);
				}
				if(result != "") {
					(irNode->killSet).insert(result);
				}
			} 
			else {
				if(opCode == "JUMP" || opCode == "LABEL" || opCode == "LINK") {
				} else {
					if(operand1 != "") {
						(irNode->genSet).insert(operand1);
					}
					if(operand2 != "") {
						(irNode->genSet).insert(operand2);
					}
					if(result != "") {
						(irNode->killSet).insert(result);
					}
				}
			}
		}
	}
}

/*===========================================================================*
 *				generateLeaders	 	  	  	     *
			Function to find leaders in 3 AC
 *===========================================================================*/
void generateLeaders() {
	// First we combine 3 AC of all functions into IR List
	for (map <string, vector <ThreeAC> >::iterator itr = funcIR.begin(); itr != funcIR.end(); ++itr) {
		vector <ThreeAC> &func = itr->second;
		for(int i = 0; i < func.size(); i++){
			IRList.push_back(&func[i]);
		}
	}
	for (int i = 0; i < IRList.size(); i++) {
		leadersList.push_back(0);
	}
	// Iterating through all of the IR List
	for(int i = 0; i < IRList.size();i++) {
		ThreeAC* irNode = IRList[i];
		unordered_set<string>* inSet = &(irNode->inSet);
		unordered_set<string>* outSet = &(irNode->outSet);
		vector<ThreeAC* >* predecessors = &(irNode->predecessor);
		if(predecessors->size() == 0) {		// Starting node of every function is a leader
			irNode->isLeader = true;
			leadersList[i] = 1;
		} 
		else {
			for(int j = 0; j < predecessors->size(); j++) {
				ThreeAC* predecessor = (*predecessors).at(j);
				string opCode = predecessor->opCode;
				if(opCode == "JUMP" || opCode == "EQ" || opCode == "NE" || opCode == "LE" || opCode == "LT" || \
						opCode == "GE" || opCode == "GT") { // These nodes are also leaders
					irNode->isLeader = true;
					leadersList[i] = 1;
				}
			}
		}
	}
}

/*===========================================================================*
 *				generateInOut	 	  	  	     *
		   Function to generate LiveIn and LiveOut sets
 *===========================================================================*/
void generateInOut() {
	generateLeaders();		// Generating leaders first
	vector<int> indexList;	// Worklist
	for(int i = 0; i < IRList.size(); i++) {
		indexList.push_back(i);
	}
	while(indexList.size() != 0) {
		int i = indexList.back();
		indexList.pop_back();
		ThreeAC* irNode = IRList[i];
		unordered_set<string> prevInSet = irNode->inSet;
		unordered_set<string> prevOutSet = irNode->outSet;
		unordered_set<string> useSet = irNode->genSet;
		unordered_set<string> defSet = irNode->killSet;
		if(irNode->opCode == "") {
			continue;
		}
		if(irNode->opCode == "RET") {
			// Update inSet
			unordered_set<string> inSet = useSet;
			unordered_set<string> outSet = prevOutSet;
			for (string def : defSet) {
				outSet.erase(def);
			}
			for (string out : outSet) {
				inSet.insert(out);
			}
			irNode->inSet.clear();
			irNode->inSet.insert(inSet.begin(), inSet.end());
		} 
		else {
			// Updating ouset
			unordered_set<string> outSet = prevOutSet;
			for (ThreeAC* successor : irNode->successor) {
				for (string in : successor->inSet) {
					outSet.insert(in);
				}
			}
			irNode->outSet.clear();
			irNode->outSet.insert(outSet.begin(), outSet.end());

			unordered_set<string> inSet = useSet;
			unordered_set<string> tempOut = outSet;
			for (string def : defSet) {
				tempOut.erase(def);
			}
			for (string out : tempOut) {
				inSet.insert(out);
			}
			irNode->inSet.clear();
			irNode->inSet.insert(inSet.begin(), inSet.end());

			unordered_set<string> diffOne;
			unordered_set<string> diffTwo;
			for(unordered_set<string>::iterator itr = inSet.begin(); itr != inSet.end(); ++itr) {
				if (prevInSet.find(*itr) != prevInSet.end()) 
					diffOne.insert(*itr);
			}
			for(unordered_set<string>::iterator itr = prevInSet.begin(); itr != prevInSet.end(); ++itr) {
				if (inSet.find(*itr) != inSet.end()) 
					diffTwo.insert(*itr);
			}
			// Adding into worklist (indexList)
			if (diffOne.size() != 0 || diffTwo.size() != 0) {
				for(ThreeAC* predecessor : irNode->predecessor) {
					vector<ThreeAC *>::iterator itr = find(IRList.begin(), IRList.end(), predecessor);
					int idx = distance(IRList.begin(), itr);
					if (find(indexList.begin(), indexList.end(), idx) != indexList.end()) {
						indexList.push_back(idx);
					}
				}
			}
		}
	}
}

/*===========================================================================*
 *			initializeRegAllocation	 	  	  	     *
 	Function that initializes the register allocation
 *===========================================================================*/
void initializeRegAllocation() {
	for(int i = 0; i < 4; i++) {
		register4R[i] = "";		// Setting all registers as free
		dirty4R[i] = false;		// Setting all registers as not dirty
	}
}

/*===========================================================================*
 *				getOP	 	  	  	     *
 	     3 AC to tiny opcode conversion for some op codes
 *===========================================================================*/
string getOp(string opCode) {
		if(opCode ==  "ADDI") return "addi";
		else if(opCode ==  "SUBI") return "subi";
		else if(opCode ==  "ADDF") return "addr";
		else if(opCode ==  "SUBF") return "subr";
		else if(opCode ==  "MULTI") return "muli";
		else if(opCode ==  "DIVI") return "divi";
		else if(opCode ==  "MULTF") return "mulr";
		else if(opCode ==  "DIVF") return "divr";
		else if(opCode ==  "WRITEI") return "writei";
		else if(opCode ==  "WRITEF") return "writer";
		else if(opCode ==  "READI") return "readi";
		else if(opCode ==  "READF") return "readr";
		else if(opCode ==  "EQ") return "jeq";
		else if(opCode ==  "NE") return "jne";
		else if(opCode ==  "GT") return "jgt";
		else if(opCode ==  "GE") return "jge";
		else if(opCode ==  "LT") return "jlt";
		else if(opCode ==  "LE") return "jle";
		else if(opCode ==  "JUMP") return "jmp";
		else if(opCode ==  "LABEL") return "label";
		else if(opCode ==  "LINK") return "link";
		else if(opCode ==  "WRITES") return "writes";
		else if(opCode ==  "PUSH") return "push";
		else if(opCode ==  "POP") return "pop";
		else return opCode;
}

/*===========================================================================*
 *				ensure	 		  	  	     *
    Function used in register allocation, loads a variable into a register
 *===========================================================================*/
int ensure(string input, int idx) {
	string start = ";ensure(): " + input;
	//Checking if a variable is already in register
	for (int i = 3; i >= 0; i--) {
		if(input == register4R[i]) {
			//cout << start + " has register r" + to_string(i) + "\n";		// Used in debugging
			return i;
		}
	}

	// Variable not in register
	int regIndex = allocate(input, idx);		// Get a free register
	string regToUse = "r" + to_string(regIndex);
	//cout << start + " gets register " + regToUse + "\n";		// Used in debugging
	//registersStatus();		// Used in debugging
	if(input.find("$T") == string::npos || (find(usedList.begin(), usedList.end(), input) != usedList.end())) {
		//assembly.push_back(TinyNode("move",  calTinyReg(input), "r" + to_string(regIndex)));
		//cout << ";loading " + input + " to register " + regToUse + "\n";	// Used for debugging
		cout << "move " + calTinyReg(input) + " " + regToUse + "\n";		// Moving variable into free register
	}
	if(input.find("$T") != string::npos && (find(usedList.begin(), usedList.end(), input) == usedList.end())) {
		usedList.push_back(input);
	}

	return regIndex;
}

/*===========================================================================*
 *				allocate	 	  	  	     *
		Allocates a register to given vatiable
 *===========================================================================*/
int allocate(string input, int idx) {
	// Finding free register
	for(int i = 3; i >= 0; i--) {
		if(register4R[i] == "") {
			register4R[i] = input;
			return i;
		}
	}

	// Free register not found
	ThreeAC* irNode = IRList[idx - 1];
	int regIndex = 3;
	for(int i = 0; i < 4; i++) {
		string reg = register4R[i];
		if(irNode->operand1 != "" && irNode->operand1 == reg) {
			continue;
		} 
		else if(irNode->operand2 != "" && irNode->operand2 == reg) {
			continue;
		} 
		else if(irNode->result != "" && irNode->result == reg) {
			continue;
		} 
		else {
			regIndex = i;
		}
	}
	//cout << ";allocate() has to spill " + register4R[regIndex] + "\n";		// Used in debugging
	free(regIndex);					// Free register found
	register4R[regIndex] = input;
	return regIndex;
}

/*===========================================================================*
 *		          		free	 	  	  	     *
				Frees a register
 *===========================================================================*/
void free(int i) {
	//cout << ";Freeing unused variable " + register4R[i] + "\n";
	if(dirty4R[i]) {
		//cout << ";Spilling variable: " + register4R[i] + "\n";		// Used in debugging
		string mem = calTinyReg(register4R[i]);
		//assembly.push_back(TinyNode("move", "r" + to_string(i), mem));
		cout << "move r" + to_string(i) + " " + mem + "\n"; 			// Spilling the dirty resigter
	}
	register4R[i] = "";
	dirty4R[i] = false;
}

/*===========================================================================*
 *				calTinyReg	 	  	  	     *
	Function to calculate variable location w.r.t base pointer
 *===========================================================================*/
string calTinyReg(string str) {
	if (str.find("$") != string::npos) {
		if (str.at(1) == 'L') {
			return "$-" + str.substr(2);
		}
		else if (str.at(1) == 'T') {
			return "$-" + to_string(localNum + stoi(str.substr(2)));
		}
		else if (str.at(1) == 'P') {
			return "$" + to_string(6 -1 + stoi(str.substr(2)));
		}
		else if (str == "$R"){
			return "$" + to_string(paraNum + 6);
		}
	}
	
	return str;
}

/*===========================================================================*
 *		        		spill	 	  	  	     *
			Function to spill all the registers
 *===========================================================================*/
void spill() {
	//cout << ";Spilling registers at the end of BB\n";		// Used in debugging
	for (int i = 3; i >= 0; i--) {
		if (register4R[i] != "") {
			//cout << ";Spilling variable: " + register4R[i] + "\n";		// Used in debugging
			string mem = calTinyReg(register4R[i]);
			//assembly.push_back(TinyNode("move", "r" + to_string(i), mem));
			cout << "move r" + to_string(i) + " " + mem + "\n";
			register4R[i] = "";
			dirty4R[i] = false;
		}
	}
}

/*===========================================================================*
 *				registersStatus	 	  	  	     *
			Prints the status of all registers
 *===========================================================================*/
void registersStatus() {
		string r4 = ";{";
		for(int i = 0; i < 4; i++){
			r4 += " r" + to_string(i) + "->";
			if(register4R[i] == ""){
				r4 += "null";
			}else{
				r4 += register4R[i];
			}
		}
		r4 += " }\n";
		cout << r4;
}

/*===========================================================================*
 *				performRegAllocation	  	  	     *
Function that performs register allocation and converts & prints 3 AC to tiny code
 *===========================================================================*/
void performRegAllocation() {
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
	/*assembly.push_back(TinyNode("push", "", ""));
	assembly.push_back(TinyNode("push", "", "r0"));
	assembly.push_back(TinyNode("push", "", "r1"));
	assembly.push_back(TinyNode("push", "", "r2"));
	assembly.push_back(TinyNode("push", "", "r3"));
	assembly.push_back(TinyNode("jsr", "", "main"));
	assembly.push_back(TinyNode("sys halt", "", ""));*/
	cout << "push\n";
	cout << "push r0\n";
	cout << "push r1\n";
	cout << "push r2\n";
	cout << "push r3\n";
	cout << "jsr main\n";
	cout << "sys halt\n";

	// Iterating through the IR List
	for(ThreeAC* irNode : IRList) {
		string opCode = irNode->opCode;
		string operand1 = irNode->operand1;
		string operand2 = irNode->operand2;
		string result = irNode->result;
		Index++;
		unordered_set<string> currOutSet = irNode->outSet;
		if(opCode == "") {
			//usedList.clear();
			continue;
		}
		//irNode->printLiveness(currOutSet);		// Used in debugging
		if(opCode == "LINK") {
			TinyNode node = TinyNode(getOp(opCode), "", "" + to_string(localNum + tempNum));
			//assembly.push_back(TinyNode(getOp(opCode), "", "" + to_string(localNum + tinyCount)));
			node.printNode();
			continue;
		}  
		if(opCode == "LE" || opCode == "GE" || opCode == "NE" \
			|| opCode == "GT" || opCode == "LT" || opCode == "EQ") {
			int reg1 = ensure(operand1, Index);
			string r1 = "r" + to_string(reg1);
			int reg2 = ensure(operand2, Index);
			string r2 = "r" + to_string(reg2);

			string type = irNode->cmpType;
			if(type == "INT") {
				TinyNode node = TinyNode("cmpi", r1, r2);
				//assembly.push_back(TinyNode("cmpi", r1, r2));
				node.printNode();
			} 
			else {
				TinyNode node = TinyNode("cmpr", r1, r2);
				//assembly.push_back(TinyNode("cmpr", r1, r2));
				node.printNode();
			}
			if(currOutSet.find(register4R[reg1]) == currOutSet.end()) {
				free(reg1);		// Freeing dead register
			}
			if(currOutSet.find(register4R[reg2]) == currOutSet.end()) {
				free(reg2);		// Freeing dead register
			}
		}

		if(Index >= leadersList.size()){

		} 
		else if(!(opCode == "LE" || opCode == "GE" || opCode == "NE" \
			|| opCode == "GT" || opCode == "LT" || opCode == "EQ" || opCode == "JUMP")) {

		} 
		else if(leadersList[Index] == 1 && !(opCode == "RET")) {
			spill();		// Spilling all register all the end of basic block
		}

		if(opCode == "JSR") {
			vector<TinyNode> assNew;
			assNew.push_back(TinyNode("push", "", "r0"));
			assNew.push_back(TinyNode("push", "", "r1"));
			assNew.push_back(TinyNode("push", "", "r2"));
			assNew.push_back(TinyNode("push", "", "r3"));
			assNew.push_back(TinyNode("jsr", "", result));
			assNew.push_back(TinyNode("pop", "", "r3"));
			assNew.push_back(TinyNode("pop", "", "r2"));
			assNew.push_back(TinyNode("pop", "", "r1"));
			assNew.push_back(TinyNode("pop", "", "r0"));

			/*assembly.push_back(TinyNode("push", "", "r0"));
			assembly.push_back(TinyNode("push", "", "r1"));
			assembly.push_back(TinyNode("push", "", "r2"));
			assembly.push_back(TinyNode("push", "", "r3"));
			assembly.push_back(TinyNode("jsr", "", result));
			assembly.push_back(TinyNode("pop", "", "r3"));
			assembly.push_back(TinyNode("pop", "", "r2"));
			assembly.push_back(TinyNode("pop", "", "r1"));
			assembly.push_back(TinyNode("pop", "", "r0"));*/
			for(int i = 0; i < assNew.size(); i++)
				assNew[i].printNode();
			
			continue;
		} 
		if(opCode == "LABEL") {
			if (contains_key(result)){
				currLabel = result;
				localNum = funcInfo[currLabel].L_count;
				paraNum = funcInfo[currLabel].P_count;
				tempNum = funcInfo[currLabel].T_count;
			}
			TinyNode node =TinyNode(getOp(opCode), "", result);
			//assembly.push_back(TinyNode(getOp(opCode), "", result));
			node.printNode();
			continue;
		} 
		if(opCode == "PUSH") {
			if(result == ""){
				TinyNode node = TinyNode(getOp(opCode), "", "");
				//assembly.push_back(TinyNode(getOp(opCode), "", ""));
				node.printNode();
				continue;
			}
		}
		if(opCode == "POP") {
			if(result == ""){
				TinyNode node = TinyNode(getOp(opCode), "", "");
				//assembly.push_back(TinyNode(getOp(opCode), "", ""));
				node.printNode();
				continue;
			}
		}  
		if(opCode == "RET") {
			spill();
			//assembly.push_back(TinyNode("unlnk", "", ""));
			//assembly.push_back(TinyNode("ret", "", ""));
			cout << "unlnk \n";
			cout << "ret \n";
		} 
		else if(opCode == "LE" || opCode == "GE" || opCode == "NE" \
				|| opCode == "GT" || opCode == "LT" || opCode == "EQ") {
			TinyNode node = TinyNode(getOp(opCode), "", result);
			//assembly.push_back(TinyNode(getOp(opCode), "", result));
			node.printNode();
		} 
		else if(opCode == "READI" || opCode == "READF" || opCode == "WRITEI" || opCode == "WRITEF") {
			int reg1 = ensure(result, Index);
			string r1 = "r" + to_string(reg1);
			dirty4R[reg1] = true;
			TinyNode node = TinyNode("sys", getOp(opCode), r1);
			//assembly.push_back(TinyNode("sys", getOp(opCode), r1));
			node.printNode();
			if(currOutSet.find(register4R[reg1]) == currOutSet.end()) {
				free(reg1);			// Freeing dead register
			}
		} 
		else if(opCode == "WRITES") {
			TinyNode node = TinyNode("sys", getOp(opCode), result);
			//assembly.push_back(TinyNode("sys", getOp(opCode), result));
			node.printNode();
		}	 
		else if(opCode == "STOREI" || opCode == "STOREF") {
			regex intNum("[0-9]+");
			regex floatNum("[0-9]*\\.[0-9]+");
			if(regex_match(operand1.begin(), operand1.end(), intNum) \
					|| regex_match(operand1.begin(), operand1.end(), floatNum)) {
				int reg1 = ensure(result, Index);
				string r1 = "r" + to_string(reg1);
				dirty4R[reg1] = true;
				TinyNode node = TinyNode("move", operand1, r1);
				//assembly.push_back(TinyNode("move", operand1, r1));
				node.printNode();
				if(currOutSet.find(register4R[reg1]) == currOutSet.end()) {
					free(reg1);
				}
			} 
			else{
				if(result != "$R") {
					int reg1 = ensure(operand1, Index);
					string r1 = "r" + to_string(reg1);
					int reg2 = ensure(result, Index);
					string r2 = "r" + to_string(reg2);
					
					dirty4R[reg2] = true;
					TinyNode node = TinyNode("move", r1, r2);
					//assembly.push_back(TinyNode("move", r1, r2));
					node.printNode();
					if(currOutSet.find(register4R[reg1]) == currOutSet.end()) {
						free(reg1);
					}
					if(currOutSet.find(register4R[reg2]) == currOutSet.end()) {
						free(reg2);
					} 
				} else {
					int reg1 = ensure(operand1, Index);
					string r1 = "r" + to_string(reg1);
					string r2 = calTinyReg(result);
					TinyNode node = TinyNode("move", r1, r2);
					//assembly.push_back(TinyNode("move", r1, r2));
					node.printNode();
					if(currOutSet.find(register4R[reg1]) == currOutSet.end()) {
						free(reg1);
					}
				}
			} 
		}
		else if(opCode == "ADDI" || opCode == "ADDF" \
				|| opCode == "SUBI" || opCode == "SUBF" \
				|| opCode == "MULTI" || opCode == "MULTF" \
				|| opCode == "DIVI" || opCode == "DIVF") {
			int reg1 = ensure(operand1, Index);
			string r1 = "r" + to_string(reg1);
			int reg2 = ensure(operand2, Index);
			string r2 = "r" + to_string(reg2);

			bool isdead1 = (currOutSet.find(operand1) != currOutSet.end()) ? true : false;
			bool isdead2 = (currOutSet.find(operand2) != currOutSet.end()) ? true : false;
			
			//std::cout << ";Switching owner of register " + r1 + " to " + result + " \n";	// Used in debugging
			//registersStatus();		// Used in debugging
			if(dirty4R[reg1]) {
				//std::cout << ";Spilling variable: " + operand1 + "\n";		// Used in debugging
				string mem = calTinyReg(operand1);
				//assembly.push_back(TinyNode("move", "r" + to_string(reg1), calTinyReg(operand1)));
				std::cout << "move r" + to_string(reg1) + " " + mem + "\n";
			}
			//registersStatus();		// Used in debugging
			if((result.find("$T") != string::npos) && \
					(find(usedList.begin(), usedList.end(), result) == usedList.end())) {
				usedList.push_back(result);
			}
			dirty4R[reg1] = false;
			register4R[reg1] = result;
			r1 = "r" + to_string(reg1);
			r2 = "r" + to_string(reg2);
			dirty4R[reg1] = true;
			TinyNode node = TinyNode(getOp(opCode), r2, r1);
			//assembly.push_back(TinyNode(getOp(opCode), r2, r1));
			node.printNode();
			if(currOutSet.find(register4R[reg1]) == currOutSet.end()) {
				free(reg1);
			}
			if(currOutSet.find(register4R[reg2]) == currOutSet.end()) {
				free(reg2);
			} 
		} 
		else if(opCode == "JUMP") {
			if (contains_key(result)){
				currLabel = result;
				localNum = funcInfo[currLabel].L_count;
				paraNum = funcInfo[currLabel].P_count;
			}
			TinyNode node = TinyNode(getOp(opCode), "", result);
			//assembly.push_back(TinyNode(getOp(opCode), "", result));
			node.printNode();
			continue;
		} 
		else if(opCode == "PUSH") {
			if(result == ""){
				assembly.push_back(TinyNode(getOp(opCode), "", ""));
			} else {
				int reg1 = ensure(result, Index);
				string r1 = "r" + to_string(reg1);
				TinyNode node = TinyNode(getOp(opCode), "", r1);
				//assembly.push_back(TinyNode(getOp(opCode), "", r1));
				node.printNode();
				if(currOutSet.find(register4R[reg1]) == currOutSet.end()) {
					free(reg1);
				}
			}
		} 
		else if(opCode == "POP") {
			if(result == ""){
				assembly.push_back(TinyNode(getOp(opCode), "", ""));
			} else {
				int reg1 = ensure(result, Index);
				string r1 = "r" + to_string(reg1);
				dirty4R[reg1] = true;
				TinyNode node = TinyNode(getOp(opCode), "", r1);
				//assembly.push_back(TinyNode(getOp(opCode), "", r1));
				node.printNode();
				if(currOutSet.find(register4R[reg1]) == currOutSet.end()) {
					free(reg1);
				}
			}
		}

		if(Index >= leadersList.size()){

		} 
		else if(!(opCode == "LE" || opCode == "GE" || opCode == "NE" \
			|| opCode == "GT" || opCode == "LT" || opCode == "EQ" || opCode == "JUMP")) {
			if(leadersList[Index] == 1 && opCode != "RET") {
				spill();		// Spilling all registers at the end of basic block
			}
		}
	}
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
		string error = "Variable " + ID + " not found in any scope";
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
			ss << "$T" << ++registerCounter;
			node->reg = ss.str();
		}
		if(node->nodeType == "CONST") {
			ss << "$T" << ++registerCounter;
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
				ss << "$T" << ++registerCounter;
				node->reg = ss.str();
			}
			else
				node->reg = node->left->reg;
		}
		if(node->nodeType == "CONST"){
			ss << "$T" << ++registerCounter;
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

