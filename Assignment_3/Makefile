compiler: scanner.l parser.ypp main.cpp symbol.cpp sTable.cpp symbolTable.cpp
	@bison -d -o parser.tab.cpp parser.ypp
	@flex scanner.l
	@g++ lex.yy.c parser.tab.cpp symbol.cpp sTable.cpp symbolTable.cpp main.cpp -o compiler
	
clean: 
	@rm -f lex.yy.c parser.tab.cpp parser.tab.hpp
dev:
	@echo "Mohammad Sameer"
	@echo "190010024@iitdh.ac.in"
