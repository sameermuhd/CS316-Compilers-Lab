#include "symbol.h"
#include "sTable.h"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

Symbol* STable::getSymbol(std::string name){
	return scopeSymbols[name];
}
bool STable::symbolExists(std::string name){
	if(scopeSymbols.find(name) == scopeSymbols.end())
		return false;
	else
		return true;
}
void STable::addSymbol(std::string name, std::string type){
	Symbol* newSymbol = new Symbol(name, type);
	allSymbols.push_back(newSymbol);
	scopeSymbols[name] = newSymbol; 
}
void STable::addSymbol(std::string name, std::string type, std::string value){
	Symbol* newSymbol = new Symbol(name, type, value);
	allSymbols.push_back(newSymbol);
	scopeSymbols[name] = newSymbol; 
}
void STable::printSTable(){
	std::cout << "Symbol table" << " " << scopeName << std::endl;
	for(auto itr = allSymbols.begin(); itr != allSymbols.end(); itr++){
		if((*itr)->getValue() == "")
			std::cout << "name" << " " << (*itr)->getName() << " type" << " " << (*itr)->getType() << std::endl;
		else
			std::cout << "name" << " " << (*itr)->getName() << " type" << " " << (*itr)->getType() << " value" << " " << (*itr)->getValue() << std::endl;
	}
}