#ifndef ASTNODE_H
#define ASTNODE_H

#include <iostream>
#include <string>

#include "threeAC.h"

class ASTNode{
	public:
		std::string value;	//Value of the AST node
		std::string dataType;	//The data type of the node
		std::string nodeType;	//The type of AST node
		std::string reg;	//The register that the node is stored in 
		ASTNode * left;		//Left child of the node
		ASTNode * right;	//Right child of the node	
		
		virtual ThreeAC generateCode() = 0;	//Function to generate the 3 Address Code
};

class ASTNodeInt : public ASTNode{
	public:
		ASTNodeInt(std::string val){
			this->value = val;
			this->dataType = "INT";
			this->nodeType = "CONST";
			this->left = NULL;
			this->right = NULL;
		}

		ThreeAC generateCode(){
			return ThreeAC("STOREI", value, "", reg);
		}
};

class ASTNodeFloat : public ASTNode{
	public:
		ASTNodeFloat(std::string val){
			this->value = val;
			this->dataType = "FLOAT";
			this->nodeType = "CONST";
			this->left = NULL;
			this->right = NULL;
		}

		ThreeAC generateCode(){
			return ThreeAC("STOREF", value, "", reg);
		}
};

class ASTNodeVar : public ASTNode{
	public:
		ASTNodeVar(std::string val, std::string dType){
			this->value = val;
			this->reg = val;
			this->dataType = dType;
			this->nodeType = "VAR";
			this->left = NULL;
			this->right = NULL;
		}

		ThreeAC generateCode(){
			return ThreeAC("", "", "", reg, "VAR");
		}
};

class ASTNodeFunc : public ASTNode {
	public:
		ASTNodeFunc(std::string re){
			this->reg = re;
			this->nodeType = "FUNC";
			this->left = NULL;
			this->right = NULL;
		}

		ThreeAC generateCode(){
			return ThreeAC("", "", "", reg, "FUNC");
		}
};

class ASTNodeOp : public ASTNode{
	public:
		ASTNodeOp(std::string op){
			this->nodeType = "OP";
			this->value = op;
			this->left = NULL;
			this->right = NULL;
		}

		ASTNodeOp(std::string op, ASTNode * lNode, ASTNode * rNode){
			this->nodeType = "OP";
			this->value = op;
			this->left = lNode;
			this->right = rNode;
		}

		ThreeAC generateCode(){
			if (value == "+"){
				if(dataType == "INT")
					return ThreeAC("ADDI", this->left->reg, this->right->reg, reg);
				else
					return ThreeAC("ADDF", this->left->reg, this->right->reg, reg);
			}
			else if (value == "-"){
				if(dataType == "INT")
					return ThreeAC("SUBI", this->left->reg, this->right->reg, reg);
				else
					return ThreeAC("SUBF", this->left->reg, this->right->reg, reg);

			}
			else if (value == "*"){
				if (dataType == "INT")
					return ThreeAC("MULTI", this->left->reg, this->right->reg, reg);
				else
					return ThreeAC("MULTF", this->left->reg, this->right->reg, reg);

			}
			else if (value == "/"){
				if (dataType == "INT")
					return ThreeAC("DIVI", this->left->reg, this->right->reg, reg);
				else
					return ThreeAC("DIVF", this->left->reg, this->right->reg, reg);

			}
			else if (value == "="){
				if (dataType == "INT")
					return ThreeAC("STOREI", this->right->reg, "", reg);
				else
					return ThreeAC("STOREF", this->right->reg, "", reg);
			}
			else 
				return ThreeAC("", "", "", "");
		}
};

#endif
