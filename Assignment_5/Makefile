compiler: scanner.l parser.y main.cpp
	@bison -d -o parser.tab.c parser.y
	@flex scanner.l
	@g++ lex.yy.c parser.tab.c headers/*.h main.cpp -o compiler
	
clean: 
	@rm -f lex.yy.c parser.tab.c parser.tab.h compiler
dev:
	@echo "Mohammad Sameer"
	@echo "190010024@iitdh.ac.in"
