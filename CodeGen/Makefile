LLVM_SRC_PATH := /home/pact2018/llvm-6.0.0.src

LLVM_BUILD_PATH := /usr/local/llvm_install
LLVM_BIN_PATH   := $(LLVM_BUILD_PATH)/bin



$(info -----------------------------------------------)
$(info Using LLVM_SRC_PATH = $(LLVM_SRC_PATH))
$(info Using LLVM_BUILD_PATH = $(LLVM_BUILD_PATH))
$(info Using LLVM_BIN_PATH = $(LLVM_BIN_PATH))
$(info -----------------------------------------------)

CXX := clang++ 
CXXFLAGS := -fno-rtti -O0 -g
PLUGIN_CXXFLAGS := -fpic

LLVM_CXXFLAGS := `$(LLVM_BIN_PATH)/llvm-config --cxxflags`
LLVM_LDFLAGS := `$(LLVM_BIN_PATH)/llvm-config --ldflags --libs --system-libs`

CLANG_INCLUDES := \
        -I$(LLVM_SRC_PATH)/tools/clang/include \
	-I$(LLVM_BUILD_PATH)/tools/clang/include

CLANG_LIBS := \
	      -Wl,--start-group \
	      -lclangAST \
	      -lclangASTMatchers \
	      -lclangAnalysis \
	      -lclangBasic \
	      -lclangDriver \
	      -lclangEdit \
	      -lclangFrontend \
	      -lclangFrontendTool \
	      -lclangLex \
	      -lclangParse \
	      -lclangSema \
	      -lclangEdit \
	      -lclangRewrite \
	      -lclangRewriteFrontend \
	      -lclangStaticAnalyzerFrontend \
	      -lclangStaticAnalyzerCheckers \
	      -lclangStaticAnalyzerCore \
	      -lclangSerialization \
	      -lclangToolingCore \
	      -lclangTooling \
	      -lclangFormat \
	      -Wl,--end-group

codegen: codegen.cpp generate-samples.cc 
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ $(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

clean: 
	rm codegen
