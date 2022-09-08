#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>

class Symbol{
	public:
		std::string name, type, value;
		
		Symbol(std::string nam, std::string typ){
			this->name = nam;
			this->type = typ;
			this->value = "";
		}
		
		Symbol(std::string nam, std::string typ, std::string val){
			this->name = nam;
			this->type = typ;
			this->value = val;
		}

		std::string getName();
		std::string getType();
		std::string getValue();
};

#endif
