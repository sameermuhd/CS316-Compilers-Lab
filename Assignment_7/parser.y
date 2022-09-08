%{
    // Libraries Used
    #include <iostream>
    #include <stdio.h>
    #include <iterator>
    #include <map>
    #include <stack>
    #include <sstream>
    #include <vector>
    #include <string>

    // Header files
    #include "headers/astNode.h"		// AST node class
    #include "headers/threeAC.h"		// Three AC class
    #include "headers/tinyNode.h"		// Tiny node class
    #include "main.h"				// main header with a few structs

    int yylex();
    void yyerror(char const* msg);		// Function to handle errors
	
    /*===========================================================================*
    *			     Functions defined in main.c		         *
    *===========================================================================*/
    extern void functionIRSetup(std::string functionID);	// Reset the register/offset counters and add IR code for the start of a function
    extern void addSymbolTable();				// Adds the table for a single scope to the symbol table
    extern void makeIR(ASTNode * ast);				// Generates the 3 AC/IR of assign statements
    extern void removeAST(ASTNode * ast);			// Removes/destroys an AST tree
    extern void pushBlock();					// Pushes a block onto the scope stack
    extern std::string ExprIR(ASTNode * node, std::string * t);	//Generates the 3 AC for an expression and returns the register holding the final value. 't' is the data type
	extern std::map <std::string, wrapper> findSymbolTable(std::string ID); //Checks for the existence of ID in any of the currently valid scopes

    /*===========================================================================*
    *			Data structures defined			                 *
    *===========================================================================*/
    wrapper w, p;						// Wrapper from the main.h header
    info f;							// Info struct from main.h to hold the total parameters and local variables count
    std::stack <std::string> scope;				// Stack holding valid symbol tables
    std::pair <std::map <std::string, wrapper>::iterator, bool> r;
    std::map <std::string, wrapper> table;			// Symbol Table of current scope
    std::map <std::string, std::map<std::string, wrapper> > symbolTable;	// Stack of Symbol Tables 

    int blockCounter = 0;					// Counter to number the blocks in code
    int labelCounter = 0;					// Counter to number the labels used 
    std::stringstream ss;					// Temporary string to print ints or floats

    std::vector <std::string> idVec, vars, strConst;		// All of these store vatiable names. Used to deeclare variable names at the start of tiny code
    
    int registerCounter = 0;					// Counter to number the temporaries used
    int localCounter = 0;					// Counter to number the local variables of a function
    int paramCounter = 0;					// Counter to number the local variables of a function
    
    std::map <std::string, std::string> varMap;			// Map of variable names to local/parameter variable identifiers
    std::map <std::string, info> funcInfo;			// Map from a function name to a struct holding information about the function
    std::stack <std::string> regs;				// Stack to keep track of registers in an expression
    std::stack <std::string> labels;				// Stack to hold the labels in use
    std::stack <std::string> incrLabels;			// Stack to hold the increment labels in use
    std::stack <std::string> outLabels;				// Stack to hold the out labels in use

    std::vector <ThreeAC> IR;					// Vector holding the ThreeAC nodes for IR Code
    std::map <std::string, std::vector <ThreeAC> > funcIR;	// maps a function name to a vector of its IR/Three AC nodes
%}

%union {		//union definition
    int ival;
    float fval;
    char * sval;
    ASTNode * ASTPtr;
}

%token KW_PROGRAM
%token KW_BEGIN
%token KW_END
%token KW_FUNCTION
%token KW_READ
%token KW_WRITE
%token KW_IF
%token KW_ELSE
%token KW_FI
%token KW_FOR
%token KW_ROF
%token KW_BREAK
%token KW_CONTINUE
%token KW_RETURN
%token <sval> KW_INT
%token KW_VOID
%token KW_STRING
%token <sval> KW_FLOAT

%token OP_ASSIGN	/* := */
%token OP_ADD		/* +  */
%token OP_SUB		/* -  */
%token OP_MUL		/* *  */
%token OP_DIV		/* /  */ 
%token OP_EQ		/* =  */
%token OP_NEQ		/* != */
%token OP_LT		/* <  */
%token OP_GT		/* >  */
%token OP_OP		/* (  */
%token OP_CP		/* )  */
%token OP_SEMICOLON	/* ;  */
%token OP_COMMA		/* ,  */
%token OP_LEQ		/* <= */
%token OP_GEQ		/* >= */

%token <sval> IDENTIFIER
%token <sval> INTLITERAL
%token <sval> FLOATLITERAL
%token <sval> STRINGLITERAL

%type <sval> id str var_type compop 
%type <ASTPtr> primary postfix_expr call_expr expr_list expr_list_tail addop mulop
%type <ASTPtr> init_stmt incr_stmt assign_expr factor factor_prefix expr_prefix expr

%%

/* Program */
program     
		    : KW_PROGRAM id KW_BEGIN {
				scope.push("GLOBAL");
		    }
		    pgm_body KW_END {
				//f.L_count = vars.size() + strConst.size();
				//f.P_count = 0;
				//funcInfo["GLOBAL"] = f;
				scope.pop();
		    }
		    ;

id          
		    : IDENTIFIER {
		    	$$ = $1;
		    }
		    ;

pgm_body    
		    : decl func_declarations 
		    ;

decl        
		    : string_decl decl 
		    | var_decl decl 
		    | %empty {
		    	addSymbolTable();
		    }
		    ;

/* Global String Declaration */
string_decl
		    : KW_STRING id OP_ASSIGN str OP_SEMICOLON {
				w.value[0] = "STRING";
				w.value[1] = $4;
				r = table.insert(std::pair<std::string, wrapper> ($2, w));
				if(!r.second){
					yyerror($2);
				}
				ss.str("");
				ss << "str " << $2 << " " << $4;
				strConst.push_back(ss.str());
		    }
		    ;

str
		    : STRINGLITERAL {
		    	$$ = $1;
		    }
		    ;

/* Variable Declaration */
var_decl
		    : var_type id_list OP_SEMICOLON {
				for(typename std::vector <std::string>::reverse_iterator itr = idVec.rbegin(); itr != idVec.rend(); ++itr){
					// Adding the declarations to the symbol table
					w.value[0] = $1;
					w.value[1] = "";
					r = table.insert(std::pair<std::string, wrapper> (*itr, w));
					if(!r.second){
						std::string temp = *itr;
						yyerror(temp.c_str());
					}
					
					//If the scope is in a function, map the variables to a local register
					if(scope.top() != "GLOBAL"){
						ss.str("");
						ss << "$L"  << ++localCounter;
						varMap[*itr] = ss.str();
						ss.str("");
					}
					//If the variable is global, then flag it to be listed at the start of the tiny code
					else
						vars.push_back(*itr);
				}
				idVec.clear();
		    }
		    ;

var_type
		    : KW_FLOAT { $$ = $1; }
		    | KW_INT { $$ = $1; }
		    ;

any_type
		    : var_type 
		    | KW_VOID 
		    ;

id_list
		    : id id_tail {
				idVec.push_back($1);
		    }
		    ;

id_tail
		    : OP_COMMA id id_tail {
				idVec.push_back($2);
		    }
		    | %empty 
		    ;

/* Function Parameter List */
param_decl_list
		    : param_decl param_decl_tail 
		    | %empty 
		    ; 

param_decl
		    : var_type id {
				w.value[0] = $1;
				w.value[1] = "";
				r = table.insert(std::pair<std::string, wrapper>($2, w));
				if(!r.second){
					yyerror($2);
				}
				std::string id = $2;
				ss.str("");
				ss << "$P" << ++paramCounter;
				varMap[id] = ss.str();
				ss.str("");
		    }
		    ;

param_decl_tail
		    : OP_COMMA param_decl param_decl_tail 
		    | %empty 
		    ;

/* Function Declarations */
func_declarations
		    : func_decl func_declarations 
		    | %empty 
		    ;

func_decl
		    : KW_FUNCTION any_type id {
				scope.push($3);
				functionIRSetup($3);
		    }
		    OP_OP param_decl_list OP_CP KW_BEGIN func_body KW_END {
		    	ThreeAC node = IR.back();
		    	if(node.opCode != "RET")
		    		IR.push_back(ThreeAC("RET", "", "", ""));
		    	std::string functionID = $3;
		    	funcIR[functionID] = IR;
		    	IR.clear();
		    	f.L_count = localCounter;
				f.P_count = paramCounter;
				f.T_count = registerCounter;
				funcInfo[functionID] = f;
				scope.pop();
		    } 
		    ;

func_body
		    : decl stmt_list 
		    ;

/* Statement List */ 
stmt_list
		    : stmt stmt_list 
		    | %empty 
		    ;

stmt
		    : base_stmt 
		    | if_stmt 
		    | for_stmt 
		    ;

base_stmt
		    : assign_stmt 
		    | read_stmt 
		    | write_stmt 
		    | return_stmt 
		    ;

/* Basic Statements */
assign_stmt
		    : assign_expr OP_SEMICOLON {
		    	makeIR($1);
		    	removeAST($1);
		    }
		    ;

assign_expr
		    : id OP_ASSIGN expr {
				std::string key = $1;
		    	std::map <std::string, wrapper> mapp = findSymbolTable(key);
		    	ASTNodeVar * node = new ASTNodeVar(key, mapp[key].value[0]);
		    	$$ = new ASTNodeOp("=", node, $3);
		    }
		    ;

read_stmt
		    : KW_READ OP_OP id_list OP_CP OP_SEMICOLON {
				for(typename std::vector <std::string>::reverse_iterator itr = idVec.rbegin(); itr != idVec.rend(); ++itr){
					std::string temp;
					std::map <std::string, wrapper> mapp = findSymbolTable(*itr);
					// Reading into a local variable
					if(varMap.count(*itr) > 0)
						temp = varMap[*itr];
					// Reading into a global variable
					else	
						temp = *itr;
					
					if (mapp[*itr].value[0] == "INT")
						IR.push_back(ThreeAC("READI", "", "", temp));
					else
						IR.push_back(ThreeAC("READF", "", "", temp));
				}
				idVec.clear();
		    }
		    ;

write_stmt
		    : KW_WRITE OP_OP id_list OP_CP OP_SEMICOLON {
		    	for(typename std::vector <std::string>::reverse_iterator itr = idVec.rbegin(); itr != idVec.rend(); ++itr){
					std::string temp;
					std::map <std::string, wrapper> mapp = findSymbolTable(*itr);
					// Writing a local variable
					if(varMap.count(*itr) > 0)
						temp = varMap[*itr];
					// // Writing a global variable
					else
						temp = *itr;

					if (mapp[*itr].value[0] == "INT")
						IR.push_back(ThreeAC("WRITEI", "", "", temp));
					else if (mapp[*itr].value[0] == "FLOAT")
						IR.push_back(ThreeAC("WRITEF", "", "", temp));
					else
						IR.push_back(ThreeAC("WRITES", "", "", temp));

				}
				idVec.clear();
		    }
		    ;

return_stmt
		    : KW_RETURN expr OP_SEMICOLON {
				std::string t;
				std::string r = ExprIR($2, &t);
				if(t == "INT")
					IR.push_back(ThreeAC("STOREI", r, "", "$R"));
				else 
					IR.push_back(ThreeAC("STOREF", r, "", "$R"));
				IR.push_back(ThreeAC("RET", "", "", ""));
				removeAST($2);
			}
		    ;

/* Expressions */
expr
		    : expr_prefix factor {
		    	if($1 != NULL){
		    		$1->right = $2;
		    		$$ = $1;
		    	}
		    	else{
		    		$$ = $2;
		    	}
		    }
		    ;

expr_prefix
		    : expr_prefix factor addop {
		    	if($1 != NULL){
		    		$3->left = $1;
		    		$1->right = $2;
		    	}
		    	else{
		    		$3->left = $2;
		    	}
		    	$$ = $3;
		    }
		    | %empty {
		    	$$ = NULL;
		    }
		    ;

factor
		    : factor_prefix postfix_expr {
		    	if($1 != NULL){
		    		$1->right = $2;
		    		$$ = $1;
		    	}
		    	else{
		    		$$ = $2;
		    	}
		    }
		    ;

factor_prefix
		    : factor_prefix postfix_expr mulop {
		    	if($1 != NULL){
		    		$3->left = $1;
		    		$1->right = $2;
		    	}
		    	else{
		    		$3->left = $2;
		    	}
		    	$$ = $3;
		    }
		    | %empty {
		    	$$ = NULL;
		    }
		    ;

postfix_expr
		    : primary { $$ = $1; }
		    | call_expr { $$ = $1; }
		    ;

call_expr
		    : id OP_OP expr_list OP_CP {
		    	IR.push_back(ThreeAC("PUSH", "", "", ""));		// Pushing space for the return value
				// Pushing all arguments onto the stack
				for(typename std::vector <std::string>::iterator itr = idVec.begin(); itr != idVec.end(); ++itr){
					IR.push_back(ThreeAC("PUSH", "", "", *itr));
				}
				IR.push_back(ThreeAC("JSR", "", "", $1));		// Jump to the function
				
				// Poping all arguments from the stack
				for(typename std::vector <std::string>::iterator itr = idVec.begin(); itr != idVec.end(); ++itr){
					IR.push_back(ThreeAC("POP", "", "", ""));
				}
				idVec.clear();
				ss.str("");
				ss << "$T" << ++registerCounter;
				IR.push_back(ThreeAC("POP", "", "", ss.str()));	// Pop the return value into a new register
				$$ = new ASTNodeFunc(ss.str());
				ss.str("");
		    }
		    ;

expr_list
		    : expr expr_list_tail {
		    	std::string t;
				std::string r = ExprIR($1, &t);
				removeAST($1);
				idVec.push_back(r);
				$$ = $1;
		    }
		    | %empty {
		    	$$ = NULL;
		    }
		    ;

expr_list_tail
		    : OP_COMMA expr expr_list_tail {
				std::string t;
				std::string r = ExprIR($2, &t);
				removeAST($2);
				idVec.push_back(r);
		    	$$ = $2;
		    }
		    | %empty {
		    	$$ = NULL;
		    }
		    ;

primary
		    : OP_OP expr OP_CP { $$ = $2; }
		    | id { 
				std::string key = $1;
		    	std::map <std::string, wrapper> mapp = findSymbolTable(key);
		    	$$ = new ASTNodeVar(key, mapp[key].value[0]);
		    }
		    | INTLITERAL { $$ = new ASTNodeInt($1); }
		    | FLOATLITERAL { $$ = new ASTNodeFloat($1); }
		    ;

addop
		    : OP_ADD { $$ = new ASTNodeOp("+"); }
		    | OP_SUB { $$ = new ASTNodeOp("-"); }
		    ;

mulop
		    : OP_MUL { $$ = new ASTNodeOp("*"); }
		    | OP_DIV { $$ = new ASTNodeOp("/"); }
		    ;


/* Complex Statements and Condition */
if_stmt
		    : KW_IF {
				pushBlock();
		    }
		    OP_OP cond OP_CP decl stmt_list {
				ss.str("");
				ss << "label" << labelCounter++;
				IR.push_back(ThreeAC("JUMP", "", "", ss.str()));
				IR.push_back(ThreeAC("LABEL", "", "", labels.top()));
				labels.pop();
				labels.push(ss.str());
		    }
		    else_part {
		    	IR.push_back(ThreeAC("LABEL", "", "", labels.top()));
		    	labels.pop();
		    }
		    KW_FI {
		    	scope.pop();
		    }
		    ;

else_part
		    : KW_ELSE {
				pushBlock();
		    }
		    decl stmt_list {
				scope.pop();
		    }
		    | %empty 
		    ;

cond
		    : expr compop expr {
		    	std::string t;
		    	std::string op1 = ExprIR($1, &t);
		    	IR.push_back(ThreeAC("", "", "", "", "SAVE"));
		    	std::string op2 = ExprIR($3, &t);
		    	ss.str("");
		    	ss << "label" << labelCounter++;  
		    	IR.push_back(ThreeAC($2, op1, op2, ss.str(), t)); 
		    	labels.push(ss.str()); 
		    	removeAST($1); 
		    	removeAST($3);
		    }
		    ;

compop
		    : OP_LT { $$ = (char *) "GE"; }
		    | OP_GT { $$ = (char *) "LE"; }
		    | OP_EQ { $$ = (char *) "NE"; }
		    | OP_NEQ { $$ = (char *) "EQ"; }
		    | OP_LEQ { $$ = (char *) "GT"; }
		    | OP_GEQ { $$ = (char *) "LT"; }
		    ;

init_stmt
		    : assign_expr { $$ = $1; }
		    | %empty 	   { $$ = NULL; }
		    ;

incr_stmt
		    : assign_expr { $$ = $1; }
		    | %empty 	   { $$ = NULL; }
		    ;

for_stmt
		    : KW_FOR {
			pushBlock();
		    }
		    OP_OP init_stmt OP_SEMICOLON {
		    	makeIR($4);
		    	removeAST($4);
		    	ss.str("");
		    	ss << "label" << labelCounter++;
		    	labels.push(ss.str());
		    	IR.push_back(ThreeAC("LABEL", "", "", ss.str()));
		    }
		    cond OP_SEMICOLON {
		    	outLabels.push(labels.top());
		    }incr_stmt {
		    	ss.str("");
		    	ss << "label" << labelCounter++;
		    	incrLabels.push(ss.str());
		    }
		    OP_CP decl aug_stmt_list {
	    		std::string temp = incrLabels.top();
		    	IR.push_back(ThreeAC("LABEL", "", "", temp));
		    	incrLabels.pop();
		    	makeIR($10);
		    	removeAST($10);
		    	temp = labels.top();
		    	labels.pop();
		    	IR.push_back(ThreeAC("JUMP", "", "", labels.top()));
		    	labels.pop();
		    	outLabels.pop();
		    	IR.push_back(ThreeAC("LABEL", "", "", temp));
		    }
		    KW_ROF {
				scope.pop();
		    }
		    ;

aug_stmt_list
		    : aug_stmt aug_stmt_list 
		    | %empty 
		    ;

aug_stmt
		    : base_stmt 
		    | aug_if_stmt 
		    | for_stmt 
		    | KW_CONTINUE OP_SEMICOLON {
		    	std::string temp = incrLabels.top();
		    	IR.push_back(ThreeAC("JUMP", "", "", temp));
		    }
		    | KW_BREAK OP_SEMICOLON {
		    	IR.push_back(ThreeAC("JUMP", "", "", outLabels.top()));
		    }
		    ;

aug_if_stmt
		    : KW_IF {
		    	pushBlock();
		    } 
		    OP_OP cond OP_CP decl aug_stmt_list {
		    	ss.str("");
				ss << "label" << labelCounter++;
				IR.push_back(ThreeAC("JUMP", "", "", ss.str()));
				IR.push_back(ThreeAC("LABEL", "", "", labels.top()));
				labels.pop();
				labels.push(ss.str());
		    }
		    aug_else_part {
		    	IR.push_back(ThreeAC("LABEL", "", "", labels.top()));
		    	labels.pop();
		    }
		    KW_FI {
		    	scope.pop();
		    }
		    ;

aug_else_part
		    : KW_ELSE {
		    	pushBlock();
		    }
		    decl aug_stmt_list {
		    	scope.pop();
		    }
		    | %empty 
		    ;

%%

