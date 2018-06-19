opt -load ../build/FuncTypePass/libFuncTypePass.so -ftype -myoutput=$1".frel" < $1".ll"
./codegen $1".cpp" < $1".frel" -- 
