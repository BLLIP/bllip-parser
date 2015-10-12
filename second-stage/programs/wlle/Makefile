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

SOURCES = avper.cc hlm.cc gavper.cc lm.cc lmdata.c oracle.cc wavper.cc wlle.cc cvlm-owlqn.cc OWLQN.cpp TerminationCriterion.cpp # cvlm-lbfgs.cc cvlm.cc 
TARGETS = avper gavper oracle cvlm-owlqn # cvlm-lbfgs # cvlm lm oracle wavper cvlm-owlqn hlm
OBJECTS = $(patsubst %.cpp,%.o,$(patsubst %.l,%.o,$(patsubst %.c,%.o,$(SOURCES:%.cc=%.o))))

all: $(TARGETS)

lm-owlqn: lm-owlqn.o OWLQN.o TerminationCriterion.o liblmdata.a
	$(CXX) $(LDFLAGS) $^ -o lm-owlqn

cvlm-owlqn: cvlm-owlqn.o OWLQN.o TerminationCriterion.o liblmdata.a
	$(CXX) $(LDFLAGS) $^ -o cvlm-owlqn

cvlm-lbfgs: cvlm-lbfgs.o liblmdata.a cobyla.o
	$(CXX) $(LDFLAGS) $^ -L/usr/local/lib -llbfgs -o cvlm-lbfgs

hlm: hlm.o OWLQN.o TerminationCriterion.o liblmdata.a
	$(CXX) $(LDFLAGS) $^ -o $@

avper: avper.o liblmdata.a
	$(CXX) $(LDFLAGS) $^ -o $@ 

gavper: gavper.o liblmdata.a 
	$(CXX) $(LDFLAGS) $^ -o $@ 

wavper: wavper.o liblmdata.a
	$(CXX) $(LDFLAGS) $^ -o $@ 

oracle: liblmdata.a oracle.o
	$(CXX) $(LDFLAGS) oracle.o liblmdata.a -o oracle

libdata.a: data.o
	ar rcv libdata.a data.o; ranlib libdata.a

liblmdata.a: lmdata.o
	ar rcv liblmdata.a lmdata.o; ranlib liblmdata.a

# CC=gcc

# fast options
# Compilation help: you may need to remove -march=native on older compilers.
# GCCFLAGS=-march=native -mfpmath=sse -msse2 -mmmx
FOPENMP=-fopenmp
# CFLAGS=-MMD -O3 -ffast-math -fstrict-aliasing -Wall -finline-functions $(GCCFLAGS) $(FOPENMP)
LDFLAGS+=$(FOPENMP)
# CXXFLAGS=${CFLAGS} -Wno-deprecated

# debugging options
# GCCFLAGS=
# FOPENMP=
# CFLAGS=-MMD -O0 -g $(GCCFLAGS) $(FOPENMP)
# LDFLAGS=-g $(FOPENMP)
# CXXFLAGS=${CFLAGS} -Wno-deprecated

.PHONY: real-clean
real-clean: clean

.PHONY: clean
clean:
	rm -fr *.a *.d *.o *~ $(TARGETS)

# this command tells GNU make to look for dependencies in *.d files
-include $(patsubst %.cpp,%.d,$(patsubst %.l,%.d,$(patsubst %.c,%.d,$(SOURCES:%.cc=%.d))))
