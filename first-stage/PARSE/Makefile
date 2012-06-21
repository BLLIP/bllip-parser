default: parseIt

all: parseIt parseAndEval evalTree

clean:
	rm -f *.o parseIt parseAndEval rParse *~ threads evalTree

# this rule automatically makes our dependency files.
# run "make Makefile.dep" if you add any files or change dependencies.
Makefile.dep:
	$(CC) -MM *.C > Makefile.dep

# include the automatically generated dependency files
-include Makefile.dep

CFLAGS=-Wall -O3 -fPIC
# CFLAGS=-g
.C.o:
	$(CXX) $(CFLAGS) -c $<

PARSEANDEVAL_OBJS = \
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
	MeChart.o \
	parseAndEval.o 

parseAndEval: $(PARSEANDEVAL_OBJS)
	$(CXX) $(CFLAGS) ${PARSEANDEVAL_OBJS} -o parseAndEval  -D_REENTRANT -D_XOPEN_SOURCE=600 -lpthread


PARSE_OBJS = \
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
	MeChart.o \
	parseIt.o 

parseIt: $(PARSE_OBJS)
	$(CXX) $(CFLAGS) $(PARSE_OBJS) -o parseIt   -D_REENTRANT -D_XOPEN_SOURCE=600 -lpthread


OPARSE_OBJS = \
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
	MeChart.o \
	oparseIt.o 

oparseIt: $(OPARSE_OBJS)
	$(CXX) $(CFLAGS) $(PARSE_OBJS) -o oparseIt 


FPT_OBJS = \
	ECArgs.o \
	utils.o \
	ParseStats.o \
	finPTest.o

finPTest: $(FPT_OBJS)
	$(CXX) $(CFLAGS) $(FPT_OBJS) -o finPTest



EVALTREE_OBJS = \
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
	MeChart.o \
	evalTree.o 

evalTree: $(EVALTREE_OBJS)
	$(CXX) $(CFLAGS) ${EVALTREE_OBJS} -o evalTree  -D_REENTRANT -D_XOPEN_SOURCE=600 -lpthread

