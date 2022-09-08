#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "sTable.h"

#include <iostream>
#include <vector>
#include <string>
#include <stack>

class SymbolTable{
	private:
		std::string errorVar = "";

	public:
		std::vector<STable *> STables;
		std::stack<STable *> StackOfTables;
		int blockCounter = 1;

		void addTable();
		void addTable(std::string name);
		void removeTable();
		void insertSymbol(std::string name, std::string type);
		void insertSymbol(std::string name, std::string type, std::string value);
		Symbol* getSymbol(std::string name);
		std::string getType(std::string name);
		void printSymbolTable();
};

#endif