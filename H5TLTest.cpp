// H5TLTest.cpp : Defines the entry point for the console application.
//


//#include "H5TL.hpp"
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
	vector<int> x(10,1);
	out(x);
	partial_sum(begin(x), end(x), begin(x));
	out(x);

	vector<float> z(10,1.1);
	partial_sum(begin(z), end(z), begin(z));
	out(z);

	set<int> y(x.begin(), x.end());
	//out(x);
	//cout << H5TL::rank(5) << endl;
	//cout << H5TL::rank(x) << endl;
	//cout << H5TL::rank(y) << endl;
	return 0;
}

