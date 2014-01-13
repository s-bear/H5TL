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

struct foo_return {
	int x;
	foo_return(int _x) : x(_x) {}
	operator int() {
		return x;
	}
	operator float() {
		return float(x) + 0.5;
	}
};

foo_return foo() {
	return foo_return(5);
}

int main(int argc, char* argv[])
{
	int a;
	H5TL::rank(0);
	H5TL::rank("abc");
	cout << H5TL::dtype(&a).size() << endl;
	a = foo();
	float b = foo();
	cout << a << ", " << b << endl;
	//set<int> y;
	//H5TL::rank(y);
	return 0;
}

