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
	return os;
}

int main(int argc, char* argv[])
{
	//Testing:
	//creating & opening files
	//creating & opening groups
	//creating & opening datasets
	//writing & reading datasets
	H5TL::File f("test.h5",H5TL::File::TRUNCATE);
	f.write("note","This file was created using H5TL");

	vector<int> a(10,1);
	cout << "a: " << a;
	partial_sum(a.begin(),a.end(),a.begin());
	f.write("data/a",a);

	vector<float> b = f.read<vector<float>>("data/a");
	cout << "b: " << b;
	return 0;
}
