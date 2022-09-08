#ifndef TINYNODE_H
#define TINYNODE_H

#include <iostream>
#include <string>

class TinyNode{
	public:
		std::string opCode;
		std::string operand1, operand2;
		
		TinyNode(std::string op, std::string op1, std::string op2){
			this->opCode = op;
			this->operand1 = op1;
			this->operand2 = op2;
		} 
		
		void printNode(){
			if(opCode != ""){
				std::cout << opCode;
				if(operand1 != "")
					std::cout << " " << operand1;
				if(operand2 != "")
					std::cout << " " << operand2;
				std::cout << "\n";
			}
		}
};

#endif
