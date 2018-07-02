# FRAP: Function Reconstruction based Automatic Parallelization

This is a compiler tool for automatic parallelization of recurrence loops based on *function reconstruction*. 
Please see our PACT2018 paper "Revealing Parallel Scans and Reductions in Recurrence through Function Reconstruction" for the theories and algorithms. 
The tool basically has two parts: an LLVM IR pass that analyzes and derives the function types in a recurrence loop, and a code generation tool that produces the parallelized code based on the analysis results.

## Install
The tool is built on LLVM 6.0.0 and Clang 6.0.0, so the first step is to install LLVM and Clang:

```bash
 wget http://releases.llvm.org/6.0.0/llvm-6.0.0.src.tar.xz
 tar xf llvm-6.0.0.src.tar.xz
 cd llvm-6.0.0.src.tar.xz/tools
 wget http://releases.llvm.org/6.0.0/cfe-6.0.0.src.tar.xz
 tar xf cfe-6.0.0.src.tar.xz
 mv cfe-6.0.0.src clang
 cd ..
 mkdir -p build/Debug
 cd build/Debug
 cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/the/path/you/want/llvm/installed/ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=On ../..
 make -j 4 && make install
 ```
If you encounter any problem, make sure you have cmake, make, gcc, g++ available on your machine. You may also need a machine with large memory (we tested on a 16GB machine and it fails to build LLVM for source due to lack of memory, and 32GB machine works for us...)

After the successful installation of LLVM (you may need to add your install directory into your PATH variable), test it with:

`clang -v`

Then, download our tool:

`git clone https://github.com/pengjiang030/pact2018_artifact.git`

Compile the LLVM IR pass:

```bash
cd pact2018_artifact
mkdir build
cd build
cmake ..
cd ..
```

To compile the code generation tool, first open to the source code directory:

`cd CodeGen`

Open the Makefile with any editor, and change `LLVM_SRC_PATH` and `LLVM_BUILD_PATH` on the top to the proper directories. `LLVM_SRC_PATH` is the directory of your LLVM source code, and `LLVM_BUILD_PATH` is the path where you have your LLVM installed.
Then, compile it with:

`make`

Now, everything is ready!


## Usage
You can try our tool with the `par.py` script provided in `loop_bench` directory. 

Let us start from the simplest example, you can take a look at `sum.cpp', which computes the sum of an array sequentially. 
Parallelize the sequential loop with our tool:

`./par.py sum.cpp`

The `par.py` script calls our tool and generates the parallelized loop in `sum_par.cpp`. 
Take a loop at that file, you can see that the sequential loop is parallelized as two parts: a parallel sampling part and a sequential propagation part. 
You can also see two more files are generated in the directory `sum.frel` and `sum.frel.png`. 
`sum.frel` is the output of the LLVM pass -- it stores the function relation graph  as tuples of `[source-node, dest-node, function-type, coefficient-if-available, pass-origin-or-not]`. And `sum.frel.png` is a plot of this graph -- you can open it with any png viewer. 

Now you can try to parallelize other loops in this directory following the same step, for example: 

`./par.py mss.cpp` 

will parallelized the loop for computing the maximum segmented sum. The parallelized code is generated in `mss_par.cpp`. The function relation graph for this loop is stored in `mss.frel` and plotted in `mss.frel.png`. 

## Note
The tool is currently a prototype, especially the code genenration part. 
We have tested it with all the loops provided in the `loop_bench` directory. 
More engineering efforts and testing are needed to optimize and stablize it.  
Contact the author (pengjiang030@gmail.com) or open an issue in this repository if you have any questions or suggestions.
