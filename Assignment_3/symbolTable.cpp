#include "sTable.h"
#include "symbolTable.h"

#include <iostream>
#include <vector>
#include <string>
#include <stack>

void SymbolTable::addTable(){
	std::string tableName = "BLOCK " + std::to_string(blockCounter);
	STable* newTable = new STable(tableName);
	StackOfTables.push(newTable);
	STables.push_back(newTable);
	blockCounter += 1;
}
void SymbolTable::addTable(std::string name){
	STable* newTable = new STable(name);
	StackOfTables.push(newTable);
	STables.push_back(newTable);
}
void SymbolTable::removeTable(){
	StackOfTables.pop();
}
void SymbolTable::insertSymbol(std::string name, std::string type){
	STable* topTable = StackOfTables.top();
	if(topTable->symbolExists(name) && errorVar == ""){
		errorVar = name;
	}
	else{
		topTable->addSymbol(name, type);
	}
}
void SymbolTable::insertSymbol(std::string name, std::string type, std::string value){
	STable* topTable = StackOfTables.top();
	if(topTable->symbolExists(name) && errorVar == ""){
		errorVar = name;
	}
	else{
		topTable->addSymbol(name, type, value);
	}
}
Symbol* SymbolTable::getSymbol(std::string name){
	std::stack<STable *> TableStack = StackOfTables;
	while(TableStack.size() > 0){
		if(TableStack.top()->symbolExists(name))
			return TableStack.top()->getSymbol(name);
		else
			TableStack.pop();
	}
	return new Symbol("err", "err");
}
std::string SymbolTable::getType(std::string name){
	return getSymbol(name)->getType();
}
void SymbolTable::printSymbolTable(){
	if(errorVar != ""){
		std::cout << "DECLARATION ERROR" << " " << errorVar << std::endl;
	}
	else{
		for(auto table = STables.begin(); table != STables.end(); ){
			(*table)->printSTable();
			table++;
			if(table != STables.end())
				std::cout << std::endl;
		}
	}
}