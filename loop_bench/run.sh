#opt -load ../build/FuncTypePass/libFuncTypePass.so -ftype -myoutput=$1".frel" < $1 #2> err_log 1> $2 
opt -load ../build/FuncTypePass/libFuncTypePass.so -ftype < $1 -debug-only=$2 -myoutput=$1".frel" 
#../CodeGen/codegen "mps.cpp" < $1".frel" -- #| clang-format
#python ../FuncTypePass/plotFRG.py $1".frel"
#opt -load ../build/FuncTypePass/libFuncTypePass.so -ftype < $1 
