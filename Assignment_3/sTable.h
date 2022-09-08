#ifndef STABLE_H
#define STABLE_H

#include "symbol.h"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

class STable{
	public:
		std::string scopeName;

		std::unordered_map<std::string, Symbol *> scopeSymbols;
		std::vector<Symbol *> allSymbols;

		STable(std::string sName){
			this->scopeName = sName;
		}

		Symbol* getSymbol(std::string name);
		bool symbolExists(std::string name);
		void addSymbol(std::string name, std::string type);
		void addSymbol(std::string name, std::string type, std::string value);
		void printSTable();
};

#endif