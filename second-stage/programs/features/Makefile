TARGETS = best-parses best-splhparses best-spmparses extract-spmfeatures best-nmparses extract-nmfeatures extract-spmultifeatures extract-nmultifeatures extract-spfeatures extract-splhfeatures extract-nfeatures oracle-score
SOURCES = best-parses.cc best-splhparses.cc best-spmparses.cc extract-spmultifeatures.cc extract-spmfeatures.cc extract-nmultifeatures.cc best-nmparses.cc extract-nmfeatures.cc extract-nfeatures.cc extract-splhfeatures.cc extract-spfeatures.cc heads.cc read-tree.l sym.cc oracle-score.cc
OBJECTS = $(patsubst %.l,%.o,$(patsubst %.c,%.o,$(SOURCES:%.cc=%.o)))

FOPENMP=-fopenmp

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

parallel-tools: count-spfeatures count-nfeatures parallel-extract-nfeatures parallel-extract-spfeatures

all: top parallel-tools

read-tree.cc: read-tree.l
	flex -oread-tree.cc read-tree.l

.PHONY: 
clean: 
	rm -fr *.o *.d *~ core read-tree.cc 

.PHONY: real-clean
real-clean: clean 
	rm -fr $(TARGETS)

# this command tells GNU make to look for dependencies in *.d files
-include $(patsubst %.l,%.d,$(patsubst %.c,%.d,$(SOURCES:%.cc=%.d)))
