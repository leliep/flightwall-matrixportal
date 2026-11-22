#!/usr/bin/env python3
"""Exercise real HTML numeric formatting, especially zero and single-digit values."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
code=r'''
#include "NumericForm.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
int main(){
 char text[48];
 for(int n=-500;n<=9000;++n){assert(numericInputText(text,sizeof(text),n,0));assert(text[0]!=' ');assert(std::strtol(text,nullptr,10)==n);}
 for(int n=0;n<360;++n){assert(numericInputText(text,sizeof(text),n,0));assert(text[0]!=' ');assert(std::strtol(text,nullptr,10)==n);}
 assert(numericInputText(text,sizeof(text),-0.0,0)&&!std::strcmp(text,"0"));
 assert(numericInputText(text,sizeof(text),51.601362,6)&&!std::strcmp(text,"51.601362"));
 assert(numericInputText(text,sizeof(text),6.141228,6)&&!std::strcmp(text,"6.141228"));
 assert(numericInputText(text,sizeof(text),30,1)&&!std::strcmp(text,"30.0"));
 assert(!numericInputText(text,1,10,0));assert(!numericInputText(text,sizeof(text),NAN,0));assert(!numericInputText(text,sizeof(text),INFINITY,0));
 std::cout<<"Numeric form: zero, headings 0-359, heights -500 to 9000, precision, negative zero and invalid/bounded values passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-numeric-form-') as folder:
 p=Path(folder);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-I'+str(root/'include'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
