# FRAP: Function Reconstruction based Automatic Parallelization

This is a compiler tool for automatic parallelization of recurrence loops based on *function reconstruction*. 
Please see our PACT2018 paper "Revealing Parallel Scans and Reductions in Recurrences through Function Reconstruction" for the theories and algorithms. 
The tool basically has two parts: an LLVM IR pass that analyzes and derives the function types in a recurrence loop, and a code generation tool that produces the parallelized code based on the analysis results. 


## Appetizer
Take a look at a simple reduction example:

```cpp
  int s = 0;
  for(int i=1; i<N; i++) {
    s += A[i-1];
  }
```

This loop comptes the sum of the elements in array `A`. We know that the computation can be parallelized by dividing the array into multiple chunks and computing the partial sums of different chunks in parallel. The partial sums are then added up to get the sum of the whole array. The parallelized code looks something like:

```cpp
// compute the partial sums in parallel
  int S_Vs[_N_THREADS]; // stores the partial sum from each thread
  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  for (int tid = 0; tid < _N_THREADS; tid++) {

    workers[tid] = thread([=, &S_Vs] {
      int Vs = 0;
      int start_pos = max(1, tid * BSIZE);
      int end_pos = min(tid * BSIZE + BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
          Vs += A[i - 1]; 
      }
     S_Vs[tid] = vs;
    });
  } 
// add up the partial sums
  int s = 0;
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join();
    int ss0 = s + S_Vs[tid][0];
    s = ss0;
  } 
```

The parallelization of the above reduction loop is quite simple. Now take a look at a less obvious example:

```cpp
float alpha = 0.8;
float s = A[0];
for(int i=1; i<N; i++) {
   s = alpha * A[i] + (1 - alpha) * s;
}
```
This loop computes the running averaging (https://en.wikipedia.org/wiki/Moving_average) of the elements in array `A`. At first glance, this loop cannot be parallelized because there is a carried dependence on `s`. That is, the computation in the next iteration has to wait for the result of `s` in the current iteration. 

However, this loop is a linear recurrence and can be parallelized as a reduction with a careful merge of the partial results:

```cpp
  float S_Vs[_N_THREADS][2];

  int BSIZE = N / _N_THREADS;
  thread *workers = new thread[_N_THREADS];
  
  for (int tid = 0; tid < _N_THREADS; tid++) {
// The array A is divided into chunks, each chunk is assigned to a thread 
// The computation among chunks are independent and conducted in parallel
    workers[tid] = thread([=, &S_Vs] {
      float Vs[2] = {0, 1};
      int start_pos = max(1, tid * BSIZE);
      int end_pos = min(tid * BSIZE + BSIZE, N);

      for (int i = start_pos; i < end_pos; i++) {
        for (int _i_s = 0; _i_s < _N_SAMP; _i_s++) {
          Vs[_i_s] = alpha * x[i] + (1 - alpha) * Vs[_i_s];
        } 
      }
      memcpy(S_Vs[tid], Vs, sizeof(Vs));
    });
  } 
  
// The results of different chunks are merged with little overhead
  float s = x[0];
  for (int tid = 0; tid < _N_THREADS; tid++) {
    workers[tid].join(); 
    float ss0 = (S_Vs[tid][1] - S_Vs[tid][0]) * s + S_Vs[tid][0];
    s = ss0;
  } 
```
You can see that the computation is effective parallelized. But how and why? 

Read our paper:

*Revealing Parallel Scans and Reductions in Recurrences through Function Reconstruction. Peng Jiang, Linchuan Chen, Gagan Agrawal. PACT 2018.* 

And you will find the steps and algorithms to parallelize such loops are actually very straightforward. 
Our tool generates the above parallel codes automatically, and it can parallelize more complicated loops than this simple example.  



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
make
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
`sum.frel` is the output of the LLVM pass -- it stores the function relation graph  as tuples of `[source-node, dest-node, function-type, coefficient-if-available, pass-origin-or-not]`. And `sum.frel.png` is a plot of this graph -- you can open it with any png viewer. (Read the paper for more explanation if you are interested!)

Now you can try to parallelize other loops in this directory following the same step, for example: 

`./par.py mss.cpp` 

will parallelized the loop for computing the maximum segmented sum. The parallelized code is generated in `mss_par.cpp`. The function relation graph for this loop is stored in `mss.frel` and plotted in `mss.frel.png`. 

## Note
The tool is currently a prototype, especially the code genenration part. 
We have tested it with all the loops provided in the `loop_bench` directory. 
More engineering efforts and testing are needed to optimize and stablize it.  
Contact the author (pengjiang030@gmail.com) or open an issue in this repository if you have any questions or suggestions.
