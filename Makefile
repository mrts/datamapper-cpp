# Dependency on dbc-cpp

DBCCPPDIR    = lib/dbccpp
DBCCPPLIB    = $(DBCCPPDIR)/lib/libdbccpp.a
DBCPPINCPATH = -I$(DBCCPPDIR)/include
DBCCPPLIBS   = -L$(DBCCPPDIR)/lib -ldbccpp -lsqlite3

# For building with clang++ 3.1 in Ubuntu 12.04, install system clang and
# add -I/usr/include/clang/3.0/include to compile flags

# For `clang++ --coverage` support:
#    ln -s sudo ln -s /usr/lib/llvm-3.0/lib/libprofile_rt.a /usr/lib

OPTIMIZE = -O2 # -g -std=c++0x | -std=c++11
COMPILER = clang++ # g++

CXX      = $(COMPILER)
CXXFLAGS = -pipe $(OPTIMIZE) -fPIC -Wall -Wextra -Werror -D_REENTRANT
INCPATH  = -Iinclude $(DBCPPINCPATH)

TEST        = datamappercpp-test
TESTCPPDIR  = test/testcpp
TESTCPPLIB  = $(TESTCPPDIR)/lib/libtestcpp.a
TESTINCPATH = $(INCPATH) -I$(TESTCPPDIR)/include
TESTCPPLIBS = -L$(TESTCPPDIR)/lib -ltestcpp

LINK     = $(COMPILER)
LFLAGS   = -Wl,-O1
LIBS     = $(TESTCPPLIBS) $(DBCCPPLIBS)

DEP      = Makefile.dep

TESTSRC  = $(wildcard test/src/*.cpp)
TESTOBJS = $(patsubst test/src/%.cpp, test/obj/%.o, $(TESTSRC))

# Targets

test/obj/%.o: test/src/%.cpp
	mkdir -p test/obj
	$(CXX) -c $(CXXFLAGS) $(TESTINCPATH) -o $@ $<

$(TESTCPPLIB): $(TESTCPPDIR)/Makefile
	cd $(TESTCPPDIR); make -j 4

$(DBCCPPLIB): $(DBCCPPDIR)/Makefile
	cd $(DBCCPPDIR); make -j 4

$(TEST): $(TESTOBJS) $(TESTCPPLIB) $(DBCCPPLIB)
	$(LINK) $(LFLAGS) -o $@ $(TESTOBJS) $(LIBS)

test: $(TEST)
	./$(TEST)

dbg: $(TEST)
	cgdb ./$(TEST)

clean:
	rm -f $(OBJS) $(TESTOBJS) $(TEST) $(DBCCPPLIB) $(TESTCPPLIB)

# Automatic dependency handling

dep: $(SRC) $(TESTSRC)
	$(CXX) $(TESTINCPATH) -MM $(SRC) $(TESTSRC) \
		| sed -r 's#^[^[:space:]]+: (test/)?(src)/([^[:space:]]+).cpp#\1obj/\3.o: \1\2/\3.cpp#' \
		> $(DEP)

include $(DEP)
