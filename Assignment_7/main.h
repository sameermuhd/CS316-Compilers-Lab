#include <iostream>

struct wrapper {
    	std::string value[2];
};

// Info struct to hold the total parameters and local variables count
struct info {	
	int L_count;
	int P_count;
	int T_count;
};
