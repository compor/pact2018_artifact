#!/usr/bin/python

import os 
import sys


filename  = sys.argv[1]

fns = filename.split('.')
fn = fns[0]

command1 = 'clang++ -S -emit-llvm ' + filename
print command1
os.system(command1)
command2 = 'opt -load ../build/FuncTypePass/libFuncTypePass.so -ftype < ' + fn +'.ll -myoutput=' + fn + '.frel -o /dev/null'
print command2
os.system(command2) 
command3 = "../CodeGen/codegen " + filename + ' < ' + fn + '.frel -- 2> /dev/null'  + ' | clang-format > ' + fn + '_par.cpp' 
#command3 = '../CodeGen/codegen ' + filename + ' < ' + fn + '.frel -- '
print command3
os.system(command3)
command4 = 'python ../FuncTypePass/plotFRG.py ' + fn + '.frel'
print command4
os.system(command4)
