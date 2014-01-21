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

int main(int argc, char* argv[]) {
	try {
		H5TL::File f("test.h5",H5TL::File::TRUNCATE);
		f.write("note","This file was created using H5TL");

		vector<int> a(10,1);
		partial_sum(a.begin(),a.end(),a.begin());
		cout << "a: " << a;
		f.write("data/a",a);

		vector<float> b = f.read<vector<float>>("data/a");
		cout << "b: " << b;

		vector<double> c(10,1.1);
		partial_sum(c.begin(),c.end(),c.begin());
		cout << "c: " << c;
		hsize_t dims[] = {10}, maxdims[] = {H5TL::DSpace::UNL};
		H5TL::DSpace cs(dims,maxdims);
		H5TL::Dataset cds = f.write("data/c",c,cs);
		cds.append(25);
		return 0;
	} catch(H5TL::h5tl_error &e) {
		cerr << e.what();
	}
}
