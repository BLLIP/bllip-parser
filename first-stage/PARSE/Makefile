# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.  You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.

default: parseIt

all: parseIt parseAndEval evalTree fusion

clean:
	rm -f *.o oparseIt parseIt parseAndEval evalTree fusion *~ threads TAGS tags parser_wrapper.C swig/wrapper.C

.PHONY: real-clean
real-clean: clean swig-clean
	rm -f *.d Makefile.dep

# this rule automatically makes our dependency files.
# run "make Makefile.dep" if you add any files or change dependencies.
Makefile.dep:
	$(CC) -MM *.C > Makefile.dep

# include the automatically generated dependency files
-include Makefile.dep

# typical usage -- the ?= sets this only if it hasn't been set previously
# (specifically in the master ../../Makefile)
CFLAGS ?= -Wall -O3 -fPIC
# for debugging
# CFLAGS=-g

.C.o:
	$(CXX) $(CFLAGS) -c $<

# all binaries need these
COMMON_OBJS = \
	Bchart.o \
	BchartSm.o \
	Bst.o \
	FBinaryArray.o \
	CntxArray.o \
	ChartBase.o \
	ClassRule.o \
	ECArgs.o \
	Edge.o \
	EdgeHeap.o \
	ExtPos.o \
	Feat.o \
	Feature.o \
	FeatureTree.o \
	Field.o \
	FullHist.o \
	GotIter.o \
	InputTree.o \
	Item.o \
	Link.o \
	Params.o \
	ParseStats.o \
	SentRep.o \
	ScoreTree.o \
	Term.o \
	TimeIt.o \
	UnitRules.o \
	ValHeap.o \
	edgeSubFns.o \
	ewDciTokStrm.o \
	extraMain.o \
	fhSubFns.o \
	headFinder.o \
	headFinderCh.o \
	utils.o \
	MeChart.o

PARSEANDEVAL_OBJS = $(COMMON_OBJS) parseAndEval.o
PARSE_OBJS = $(COMMON_OBJS) parseIt.o
OPARSE_OBJS = $(COMMON_OBJS) oparseIt.o
EVALTREE_OBJS = $(COMMON_OBJS) SimpleAPI.o evalTree.o
FUSION_OBJS = $(COMMON_OBJS) SimpleAPI.o Fusion.o

parseAndEval: $(PARSEANDEVAL_OBJS)
	$(CXX) $(CFLAGS) ${PARSEANDEVAL_OBJS} -o parseAndEval -D_REENTRANT -D_XOPEN_SOURCE=600 -lpthread

parseIt: $(PARSE_OBJS)
	$(CXX) $(CFLAGS) $(PARSE_OBJS) -o parseIt -D_REENTRANT -D_XOPEN_SOURCE=600 -lpthread

oparseIt: $(OPARSE_OBJS)
	$(CXX) $(CFLAGS) $(OPARSE_OBJS) -o oparseIt 

evalTree: $(EVALTREE_OBJS)
	$(CXX) $(CFLAGS) ${EVALTREE_OBJS} -o evalTree -D_REENTRANT -D_XOPEN_SOURCE=600 -lpthread

fusion: $(FUSION_OBJS)
	$(CXX) $(CFLAGS) $(FUSION_OBJS) -o fusion -D_REENTRANT -D_XOPEN_SOURCE=600

.PHONY: valgrind-parseIt
valgrind-parseIt: CFLAGS += -g -O0
valgrind-parseIt: parseIt
	valgrind --leak-check=full ./parseIt ../DATA/EN/ ../../sample-text/sample-data.txt

.PHONY: clang-leak-parseIt
clang-leak-parseIt: CFLAGS = -fsanitize=leak
clang-leak-parseIt: CXX = clang++
clang-leak-parseIt: parseIt
	ASAN_options="detect_leaks=1" ./parseIt ../DATA/EN/ short.sgml

#
# SWIG wrappers for Java and Python
#

# NOTE: There is now a much more friendly way to install the Python
# bindings with distutils.
# Either run "sudo python setup.py install" from the root of the reranking
# parser distribution or type "sudo pip install bllipparser" if you have
# pip installed.

# These paths are likely not very portable and may need to be edited
# (they also can be overridden by the root ../../Makefile or environment
# variables)

# this should be the path to jni.h
SWIG_JAVA_GCCFLAGS ?= -I/usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include/ \
	-I/usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include/linux/
# this should be the path to Python.h
SWIG_PYTHON_GCCFLAGS ?= -I/usr/include/python2.6/
# -L should have the path to libstdc++.so
SWIG_LINKER_FLAGS ?= -lstdc++ -L/usr/lib/gcc/x86_64-redhat-linux/4.4.4/

# Edit or set these variables as needed
SWIG_PARSER_MODULE_NAME ?= SWIGParser
# SWIG_JAVA_PACKAGE ?= your.package.name.for.the.swig.wrapper

SWIG_JAVA_FLAGS = ${SWIG_FLAGS}
ifneq ($(strip $(SWIG_JAVA_PACKAGE)),)
	SWIG_JAVA_FLAGS += -package ${SWIG_JAVA_PACKAGE}
endif

# 
# main SWIG targets
#
swig-java: CFLAGS += -fPIC -fno-strict-aliasing
swig-java: swig/java/lib/lib$(SWIG_PARSER_MODULE_NAME).so

swig-java-test: swig-java
	javac swig/java/lib/*.java
	javac -cp swig/java/lib/ swig/java/test/*.java
	java -cp swig/java/lib:swig/java/test -Djava.library.path=swig/java/lib test

swig-python: CFLAGS += -fPIC -fno-strict-aliasing
swig-python: swig/python/lib/_$(SWIG_PARSER_MODULE_NAME).so

swig-python-test: swig-python
	PYTHONPATH=$(PYTHONPATH):swig/python/lib python swig/python/test/test.py

.PHONY: swig-python-valgrind
swig-python-valgrind: CFLAGS += -g -O0
swig-python-valgrind: swig-python
	PYTHONPATH=$(PYTHONPATH):swig/python/lib valgrind --leak-check=full --suppressions=/usr/share/doc/python-devel-2.6.6/valgrind-python.supp python swig/python/test/test.py

swig-clean:
	rm -rf swig/build swig/python/lib swig/java/lib swig/java/test/*.class swig/python/test/*.py[co]

#
# Java SWIG helpers
#
swig/build/java_wrapper.cxx: swig/wrapper.i
	mkdir -p swig/build
	mkdir -p swig/java/lib
	swig $(SWIG_JAVA_FLAGS) -module $(SWIG_PARSER_MODULE_NAME) -Wall -c++ -java -outdir swig/java/lib \
		-o swig/build/java_wrapper.cxx swig/wrapper.i

swig/build/java_wrapper.o: swig/build/java_wrapper.cxx
	$(CC) -O3 $(CFLAGS) -c $(SWIG_JAVA_GCCFLAGS) -I. \
		swig/build/java_wrapper.cxx -o swig/build/java_wrapper.o

swig/java/lib/lib$(SWIG_PARSER_MODULE_NAME).so: swig/build/java_wrapper.o $(COMMON_OBJS)
	$(CC) $(SWIG_LINKER_FLAGS) -shared \
		-o swig/java/lib/lib$(SWIG_PARSER_MODULE_NAME).so \
		$(COMMON_OBJS) swig/build/java_wrapper.o

#
# Python SWIG helpers
#
swig/build/python_wrapper.cxx: swig/wrapper.i
	mkdir -p swig/build
	mkdir -p swig/python/lib
	swig $(SWIG_FLAGS) -module $(SWIG_PARSER_MODULE_NAME) -Wall -c++ -python -classic \
		-outdir swig/python/lib -o swig/build/python_wrapper.cxx swig/wrapper.i

swig/build/python_wrapper.o: swig/build/python_wrapper.cxx
	$(CC) -O3 -fno-strict-aliasing -fPIC -c $(SWIG_PYTHON_GCCFLAGS) \
		-I. swig/build/python_wrapper.cxx -o swig/build/python_wrapper.o

swig/python/lib/_$(SWIG_PARSER_MODULE_NAME).so: swig/build/python_wrapper.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(SWIG_LINKER_FLAGS) -shared \
		-o swig/python/lib/_$(SWIG_PARSER_MODULE_NAME).so \
		$(COMMON_OBJS) swig/build/python_wrapper.o
