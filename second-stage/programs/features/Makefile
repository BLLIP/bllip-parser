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

TARGETS = best-parses best-splhparses best-spmparses extract-spmfeatures best-nmparses extract-nmfeatures extract-spmultifeatures extract-nmultifeatures extract-spfeatures extract-splhfeatures extract-nfeatures oracle-score
SOURCES = best-parses.cc best-splhparses.cc best-spmparses.cc extract-spmultifeatures.cc extract-spmfeatures.cc extract-nmultifeatures.cc best-nmparses.cc extract-nmfeatures.cc extract-nfeatures.cc extract-splhfeatures.cc extract-spfeatures.cc heads.cc read-tree.l sym.cc oracle-score.cc
OBJECTS = $(patsubst %.l,%.o,$(patsubst %.c,%.o,$(SOURCES:%.cc=%.o)))
PARALLEL_TOOLS_TARGETS = count-spfeatures count-nfeatures parallel-extract-nfeatures parallel-extract-spfeatures

FOPENMP?=-fopenmp

top: $(TARGETS)

extract-nmfeatures.o: extract-nmfeatures.cc
	$(CXX) -c $(CXXFLAGS) $(FOPENMP) $< -o $@

extract-nmfeatures: extract-nmfeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $(FOPENMP) $^ -o $@

best-nmparses.o: best-nmparses.cc
	$(CXX) -c $(CXXFLAGS) $(FOPENMP) $< -o $@

best-nmparses: best-nmparses.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $(FOPENMP) $^ -o $@

extract-spmfeatures.o: extract-spmfeatures.cc
	$(CXX) -c $(CXXFLAGS) $(FOPENMP) $< -o $@

extract-spmfeatures: extract-spmfeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $(FOPENMP) $^ -o $@

best-spmparses.o: best-spmparses.cc
	$(CXX) -c $(CXXFLAGS) $(FOPENMP) $< -o $@

best-spmparses: best-spmparses.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $(FOPENMP) $^ -o $@

extract-spmultifeatures: extract-spmultifeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

extract-nmultifeatures: extract-nmultifeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

extract-spfeatures: extract-spfeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

extract-splhfeatures: extract-splhfeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

extract-nfeatures: extract-nfeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

best-parses: best-parses.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

best-splhparses: best-splhparses.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

oracle-score: oracle-score.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

count-spfeatures: count-spfeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

parallel-extract-spfeatures: parallel-extract-spfeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

count-nfeatures: count-nfeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

parallel-extract-nfeatures: parallel-extract-nfeatures.o heads.o read-tree.o sym.o
	$(CXX) $(LDFLAGS) $^ -o $@

parallel-tools: $(PARALLEL_TOOLS_TARGETS)

all: top parallel-tools

read-tree.cc: read-tree.l
	flex -oread-tree.cc read-tree.l

.PHONY: 
clean:
	rm -fr *.o *.d *~ core read-tree.cc reranker_wrapper.C

.PHONY: real-clean
real-clean: clean
	rm -fr $(TARGETS) $(PARALLEL_TOOLS_TARGETS) tags TAGS

#
# SWIG wrappers for Java and Python
#

# NOTE: There is now a much more friendly way to install the Python
# bindings with distutils.
# Either run "sudo python setup.py install" from the root of the reranking
# parser distribution or type "sudo pip install bllipparser" if you have
# pip installed.

# These paths are likely not very portable and may need to be edited
# (they also can be overridden by the root ../../Makefile)

# this should be the path to jni.h
SWIG_JAVA_GCCFLAGS ?= -I/usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include/ \
	-I/usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include/linux/
# this should be the path to Python.h
SWIG_PYTHON_GCCFLAGS ?= -I/usr/include/python2.6/
# -L should have the path to libstdc++.so
SWIG_LINKER_FLAGS ?= -lstdc++ -L/usr/lib/gcc/x86_64-redhat-linux/4.4.4/

# Edit these variables as needed
SWIG_RERANKER_MODULE_NAME ?= SWIGReranker
SWIG_JAVA_PACKAGE ?= ${SWIG_RERANKER_MODULE_NAME}
SWIG_JAVA_FLAGS = ${SWIG_FLAGS}

ifneq ($(strip $(SWIG_JAVA_PACKAGE)),)
	SWIG_JAVA_FLAGS += -package ${SWIG_JAVA_PACKAGE}
endif

SWIG_OBJS = simple-api.o heads.o read-tree.o sym.o

# for some reason, optimization flags make some functions disappear
# (sp_sentence_type::nparses() for example) so we need to turn off
# optimization (highly unfortunate -- would be nice to work this out)
simple-api.o: CXXFLAGS += -O0
simple-api.o: simple-api.cc
	$(CXX) -c $(CXXFLAGS) $(FOPENMP) $< -o $@

.PHONY: swig-java
swig-java: swig/java/lib/lib$(SWIG_RERANKER_MODULE_NAME).so

swig/java/lib/lib$(SWIG_RERANKER_MODULE_NAME).so: CXXFLAGS += -fPIC -fno-strict-aliasing -Wno-deprecated
swig/java/lib/lib$(SWIG_RERANKER_MODULE_NAME).so: $(SWIG_OBJS)
	mkdir -p swig/build
	mkdir -p swig/java/lib
	swig $(SWIG_JAVA_FLAGS) -module $(SWIG_RERANKER_MODULE_NAME) -Wall -c++ -java -outdir swig/java/lib \
		-o swig/build/java_wrapper.cxx swig/wrapper.i
	gcc -O2 $(CXXFLAGS) -c $(SWIG_JAVA_GCCFLAGS) -iquote . \
		swig/build/java_wrapper.cxx -o swig/build/java_wrapper.o
	gcc $(SWIG_LINKER_FLAGS) -shared -o \
		swig/java/lib/lib$(SWIG_RERANKER_MODULE_NAME).so \
		$(SWIG_OBJS) swig/build/java_wrapper.o

.PHONY: swig-java-test
swig-java-test: swig-java
	javac swig/java/lib/*.java
	javac -cp swig/java/lib/ swig/java/test/*.java
	java -cp swig/java/lib:swig/java/test -Djava.library.path=swig/java/lib test

.PHONY: swig-python
swig-python: swig/python/lib/_$(SWIG_RERANKER_MODULE_NAME).so

swig/python/lib/_$(SWIG_RERANKER_MODULE_NAME).so: CXXFLAGS += -fPIC -fno-strict-aliasing -Wno-deprecated
swig/python/lib/_$(SWIG_RERANKER_MODULE_NAME).so: $(SWIG_OBJS)
	mkdir -p swig/build
	mkdir -p swig/python/lib
	swig $(SWIG_FLAGS) -module $(SWIG_RERANKER_MODULE_NAME) -Wall -c++ -python -classic -outdir \
		swig/python/lib -o swig/build/python_wrapper.cxx swig/wrapper.i
	g++ -fno-strict-aliasing $(CXXFLAGS) -Wfatal-errors \
		-c -iquote . $(SWIG_PYTHON_GCCFLAGS) \
		swig/build/python_wrapper.cxx -o swig/build/python_wrapper.o
	gcc $(SWIG_LINKER_FLAGS) -shared $(SWIG_OBJS) \
		swig/build/python_wrapper.o -o swig/python/lib/_$(SWIG_RERANKER_MODULE_NAME).so

.PHONY: swig-python-test
swig-python-test: swig-python
	PYTHONPATH=swig/python/lib/ python swig/python/test/test.py

.PHONY: swig-python-valgrind
swig-python-valgrind: CXXFLAGS += -g
swig-python-valgrind: swig-python
	PYTHONPATH=swig/python/lib/ valgrind --leak-check=full --suppressions=/usr/share/doc/python-devel-2.6.6/valgrind-python.supp python swig/python/test/test.py

swig-clean:
	rm -rf swig/build swig/python/lib swig/java/lib swig/java/test/*.class swig/python/test/*.py[co] simple-api.o

# this command tells GNU make to look for dependencies in *.d files
-include $(patsubst %.l,%.d,$(patsubst %.c,%.d,$(SOURCES:%.cc=%.d)))
