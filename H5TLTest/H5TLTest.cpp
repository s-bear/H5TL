/**********
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014 Samuel Bear Powell
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
\**********/

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

//#include "blitz/Array.h"

template<typename T, size_t N>
ostream& operator<<(ostream& os, const array<T,N>& vec) {
	os << "array{";
	bool first = true;
	for(auto t : vec) {
		if(first) {os << t; first = false;}
		else {os << ", " << t;}
	}
	os << "}" << endl;
	return os;
}


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
		H5TL::DProps cp = H5TL::DProps().chunked().deflate(3).fill(1);
		H5TL::Dataset cds = f.write("data/c",c,cs,cp);
		int x[] = {25,26,27};
		cds.append(x);
		c.resize(13);
		cds.read(c);
		cout << "c: " << c;
		
		//vector<bool> doesn't work because the standard is weird
		array<bool,10> d; 
		for(size_t i = 0; i < d.size(); ++i)
			d[i] = (i % 2 == 0);
		cout << boolalpha << "d: " << d;
		f.write("d",d);
		array<bool,10> e = f.read<array<bool,10>>("d");
		cout << "e: " << e;
		
		return 0;
	} catch(H5TL::h5tl_error &e) {
		cerr << e.what();
	}
}
