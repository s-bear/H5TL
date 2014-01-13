// H5TLTest.cpp : Defines the entry point for the console application.
//


#include "../H5TL/H5TL.hpp"
#include <vector>
#include <set>
#include <array>
#include <iostream>
#include <algorithm>
#include <numeric>
using namespace std;

template<typename T>
void out(const vector<T>& vec) {
	cout << "vector[";
	bool first = true;
	for(auto t : vec) {
		if(first) {cout << t; first = false;}
		else {cout << ", " << t;}
	}
	cout << "]" << endl;
}

int main(int argc, char* argv[])
{
	H5TL::File f("test.h5",H5TL::File::TRUNCATE);
	f.writeDataset("data/x",15);
	return 0;
}
