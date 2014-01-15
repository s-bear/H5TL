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

#include "blitz/Array.h"

template<typename T>
ostream& operator<<(ostream& os, const vector<T>& vec) {
	os << "vector{";
	bool first = true;
	for(auto t : vec) {
		if(first) {os << t; first = false;}
		else {os << ", " << t;}
	}
	os << "}" << endl;
}

int main(int argc, char* argv[])
{
	//Testing:
	//creating & opening files
	//creating & opening groups
	//creating & opening datasets
	//writing & reading datasets
	H5TL::File f("test.h5",H5TL::File::TRUNCATE);
	
	auto a = f.openDataset("data");
	auto x = f.createDataset("data/x",H5TL::DType::UINT8,H5TL::DSpace::SCALAR);
	x.write(15);
	int y = x.read<int>();

	auto z = x.read<blitz::Array<float,2>>();

	cout << boolalpha;
	cout << "equal? " << (x.dtype() == H5TL::DType::UINT8) << endl;
	return 0;
}
