#ifndef THREEAC_H
#define THREEAC_H

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>

class ThreeAC{
	public:
		std::string opCode;
		std::string operand1, operand2, result;
		std::string cmpType;
		std::vector<ThreeAC *> successor;
		std::vector<ThreeAC *> predecessor;
		std::unordered_set<std::string> genSet;
		std::unordered_set<std::string> killSet;
		std::unordered_set<std::string> inSet;
		std::unordered_set<std::string> outSet;
		bool isLeader;

		ThreeAC(std::string op, std::string op1, std::string op2, std::string res){
			this->opCode = op;
			this->operand1 = op1;
			this->operand2 = op2;
			this->result = res;
			this->cmpType = "none";
			this->successor.clear();
			this->predecessor.clear();
			this->isLeader = false;
		}

		ThreeAC(std::string op, std::string op1, std::string op2, std::string res, std::string cmp){
			this->opCode = op;
			this->operand1 = op1;
			this->operand2 = op2;
			this->result = res;
			this->cmpType = cmp;
			this->successor.clear();
			this->predecessor.clear();
			this->isLeader = false;
		}

		void printThreeAC(){
			if(opCode != ""){
				std::cout << ";" << opCode;
				if(operand1 != "")
					std::cout << " " << operand1;
				if(operand2 != "")
					std::cout << " " << operand2;
				std::cout << " " << result << "\n";
			}
		}

		void printLiveness(std::unordered_set<std::string> LiveVar) {
			if((opCode == "PUSH" || opCode == "POP") && (operand1 == "") && (operand2 == "") && (result == "")){
				std::cout << ";" + opCode + " \n\t";
				std::cout << ";live vars: ";
				for(std::string var : LiveVar)
					std::cout << var + ", ";
				std::cout << "\n";
				return; 
			}
			std::string partFirst = "";
			std::string partSecond = "";
			std::string partDest = "";
			if(operand1 != "")
				partFirst = " " + operand1;
			if(operand2 != "") 
				partSecond = " " + operand2;
			if(result != "")
				partDest = " " + result;
			std::cout << ";" + opCode + partFirst + partSecond + partDest + "\n";
			std::cout << "\t";
			std::cout << ";live vars: ";
			for(std::string var : LiveVar)
				std::cout << var + ", ";
			std::cout << "\n";
			return; 
		}
};

#endif
