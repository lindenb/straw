/*
  The MIT License (MIT)
 
  Copyright (c) 2011-2016 Broad Institute, Aiden Lab
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/
#include <cstring>
#include <iostream>
#include "straw.h"
using namespace std;

int main(int argc, char *argv[])
{
  if (argc != 7) {
    cerr << "Not enough arguments" << endl;
    cerr << "Usage: " << argv[0] << " <NONE/VC/VC_SQRT/KR> <hicFile(s)> <chr1>[:x1:x2] <chr2>[:y1:y2] <BP/FRAG> <binsize>" << endl;
    return EXIT_FAILURE;
  }

  string norm=argv[1];
  string fname=argv[2];
  string chr1loc=argv[3];
  string chr2loc=argv[4];
  string unit=argv[5];
  string size=argv[6];
  int binsize=stoi(size);
  vector<XYCount> results;
try {
  if(straw(norm, fname, binsize, chr1loc, chr2loc, unit, results) != EXIT_SUCCESS) {
  	return EXIT_FAILURE;
  	}
  size_t length=results.size();
  for (size_t i=0; i<length; i++) {
    printf("%d\t%d\t%.14g\n", results[i].x, results[i].y, results[i].count);   
  }
  return EXIT_SUCCESS;
  }
catch(exception& err) {
	cerr << "Error:" << err.what() << endl;
	return EXIT_FAILURE;
	}
catch(...) {
	cerr << "An error occured" << endl;
	return EXIT_FAILURE;
	}
}
