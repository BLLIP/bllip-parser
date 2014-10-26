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

# Top-level makefile for reranking parser
# Mark Johnson, 28th July 2008 -- added EXEC prefix to major commands

########################################################################
#                                                                      #
#                                Summary                               #
#                                                                      #
########################################################################
#
# To build just the reranking parser run-time, execute:
#
# make                 # builds reranking parser programs
#
# To retrain the reranking parser, run the following steps:
#
# make reranker        # builds reranking parser and training programs
# make nbesttrain      # builds 20 folds of n-best training parses
# make eval-reranker   # extracts features, estimates weights, and evaluates
#
# The following high-level goals may also be useful:
#
# make nbesttrain-clean # removes temporary files used in nbesttrain
# make nbest-oracle    # oracle evaluation of n-best results 
# make features        # extracts features from 20-fold parses
# make train-reranker  # trains reranker model
# make train-clean     # removes all temporary files used in training
# make sparseval       # download and install SParseval
#
# I typically run nbesttrain to produce the n-best parses 

# To run 2 jobs in parallel (e.g. on a multiprocessor) run, e.g.,
#
# make -j 2 nbesttrain
#
# This really only helps with nbesttrain, since the other time consuming
# step (reranker feature weight estimation) isn't yet parallelized.

# The environment variable GCCFLAGS can be used to specify
# machine-dependent optimization flags, e.g.
#
# setenv GCCFLAGS "-march=pentium4"
#
# or
#
# setenv GCCFLAGS "-march=opteron -m64"
#
# The top-level make goal builds the reranking parser using a pre-trained
# model.  To build this parser, just run
#
# make 
#
# You may need to tweak the following variables to suit your environment

# GCCFLAGS is not set here, so we use the shell environment
# variable's value.  But you can set it here if you want.
# Version 4.1 and later gcc permit -march=native, but older
# versions will need -march=pentium4 or -march=opteron
#
# GCCFLAGS ?= -march=native -mfpmath=sse -msse2 -mmmx -m32

# CFLAGS is used for all C and C++ compilation
#
CFLAGS = -MMD -O3 -Wall -ffast-math -finline-functions -fomit-frame-pointer -fstrict-aliasing $(GCCFLAGS)

EXEC = time

# for SWIG wrappers, use these flags instead
#
# CFLAGS = -MMD -O3 -Wall -ffast-math -finline-functions -fomit-frame-pointer -fno-strict-aliasing -fPIC $(GCCFLAGS)
# LDFLAGS = $(GCCLDFLAGS)
# EXEC = time

# for debugging, uncomment the following CFLAGS, LDFLAGS and EXEC
#
# CFLAGS = -g -O -MMD -Wall -ffast-math -fstrict-aliasing $(GCCFLAGS)
# LDFLAGS = -g -Wall $(GCCLDFLAGS)
# EXEC = valgrind

CXXFLAGS ?= $(CFLAGS) -Wno-deprecated
export CFLAGS
export CXXFLAGS
export LDFLAGS

CC ?= gcc
CXX ?= g++
export CC
export CXX

# Building the 20-fold training data with nbesttrain 
# --------------------------------------------------

# For training the parser and reranker you will need your own copy of the
# Penn WSJ Treebank.
#
# PENNWSJTREEBANK must be set to the base directory of the Penn WSJ Treebank
#
PENNWSJTREEBANK=/usr/local/data/Penn3/parsed/mrg/wsj/

# NPARSES is the number of alternative parses to consider for each sentence
#
NPARSES=50

# NFOLDS is the number of folds to use, and FOLDS is a list of the numbers
# from 00 to NFOLDS-1 (I couldn't see how to program this in make).
#
NFOLDS=20
FOLDS=00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19

# SECTIONS is a list of sections from the treebank to n-best parse
# using Eugene's standard n-best parser (in addition to the folds).
#
SECTIONS=22 23 24

# NBESTPARSERBASEDIR is the base directory in which the n-best parser is
# located.  If you change this, change NBESTPARSERNICKNAME as well.
#
NBESTPARSERBASEDIR=first-stage

# NBESTPARSER is the n-best parser.  If you change this, please
# change NBESTPARSERNICKNAME below as well.
#
NBESTPARSER=$(NBESTPARSERBASEDIR)/PARSE/parseIt

# NBESTTRAINER is the program (probably a shell script) for training
# the n-best parser.  If you change this, please change
# NBESTPARSERNICKNAME below as well.
#
NBESTTRAINER=$(NBESTPARSERBASEDIR)/TRAIN/trainParser

# NBESTPARSERNICKNAME is a nickname for the n-best parser.  If you 
# experiment with several n-best parsers, give each one a different
# nickname.
#
NBESTPARSERNICKNAME=ec

# TMP specifies a temporary directory used while constructing the
# folds while producing the training parses for the reranker.  You can
# delete this directory after nbesttrain has finished.  On an NFS
# system you may want to change this to a local directory.
#
TMP=tmp

# Extracting features from 20-fold n-best parses
# ----------------------------------------------

# VERSION should be either "final" or "nonfinal".  If VERSION is
# "nonfinal" then we train on folds 00-19, folds 20-21 are used as
# dev, and sections 22 and 24 are used as test1 and test2
# respectively.  If VERSION is "final" then we train on folds 00-21,
# section 24 is used as dev and sections 22 and 23 are used as test1
# and test2 respectively.
#
# VERSION=nonfinal
VERSION=final

# FEATUREEXTRACTOR is the program that used to extract features from
# the 20-fold n-best parses.  If you change this, please pick a new 
# FEATURESNICKNAME below.
#
FEATUREEXTRACTOR=second-stage/programs/features/extract-spfeatures

# FEATUREEXTRACTORFLAGS are flags you want to give to the feature extractor
#
FEATUREEXTRACTORFLAGS=-l -c -i -s 5

# FEATURESNICKNAME is an arbitrary string used to identify a
# particular set of extracted features for training the reranker.  You
# can keep several different sets of feature counts and corresponding
# models around by giving each a unique FEATURESNICKNAME.  If you
# develop a new set of features, give them a new FEATURESNICKNAME so
# they doesn't over-write the existing features.
#
FEATURESNICKNAME=sp

# Estimating weights for features
# -------------------------------

# ESTIMATOR is the program used to estimate feature weights from the
# feature counts. This is the feature weight estimator that gives best
# performance.  There are others in the same directory (e.g., weighted
# perceptron).  If you decide to use a different feature weight
# estimator you should also change ESTIMATORNICKNAME below.  Other sets
# of these variable values can be found in the train-eval-reranker.sh
# script.
#
ESTIMATOR=second-stage/programs/wlle/cvlm-lbfgs

# ESTIMATORFLAGS are flags given to the estimator
#
ESTIMATORFLAGS=-l 1 -c 10 -F 1 -n -1 -p 2

# ESTIMATORNICKNAME is used to name the feature weights file
#
ESTIMATORNICKNAME=lbfgs-l1c10F1n1p2

# ESTIMATORSTACKSIZE is the size (in KB) of the per-thread stacks
# used during estimation
#
# ESTIMATORSTACKSIZE=40960

# ESTIMATORENV is a prefix to the estimator command that can be
# used to set environment variables, etc.
#
# ESTIMATORENV=ulimit -s $(ESTIMATORSTACKSIZE) && OMP_STACKSIZE=`ulimit -s` &&
ESTIMATORENV=

########################################################################
#
# You probably shouldn't need to change anything below here.

# TARGETS is the list of targets built when make is called
# without arguments
#
TARGETS = PARSE reranker-runtime

.PHONY: top
top: $(TARGETS)

# zcat on OS X behaves differently in some cases
ZCAT = gunzip -c

# PARSE builds the n-best first-stage parser (i.e., Eugene's parser).
#
.PHONY: PARSE
PARSE:
	$(MAKE) -C $(NBESTPARSERBASEDIR)/PARSE parseIt

# TRAIN builds the programs needed to train the first-stage parser.
#
.PHONY: TRAIN
TRAIN:
	$(MAKE) -C $(NBESTPARSERBASEDIR)/TRAIN all

# reranker-runtime builds the run-time components of the reranker.
# These include best-parses, which reranks the n-best parses produced
# by the first-stage parser, and ptb, which is a program that converts
# Penn Treebank trees into the various formats needed by Eugene's
# parser, the reranker training programs, sparseval, etc.  
#
.PHONY: reranker-runtime
reranker-runtime:
	$(MAKE) -C second-stage/programs/features best-parses
	$(MAKE) -C second-stage/programs/prepare-data ptb

# reranker builds the training and run-time components of the reranker.
# These include:
#  ptb, which converts the Penn Treebank parse trees into
#       the various formats needed by Eugene's parser, the reranker training
#       program, sparseval, etc., 
#  extract-spfeatures, which produces feature-count files used to train 
#       the reranker, 
#  cvlm-lbfgs/avper, which estimates the feature weights.
#
.PHONY: reranker
reranker: top TRAIN
	$(MAKE) -C second-stage

# EVALB has been replaced with sparseval (nearly the same features with fewer bugs)
#
sparseval: SParseval/src/sparseval

SParseval:
	wget http://www.clsp.jhu.edu/vfsrv/ws2005/groups/eventdetect/files/SParseval.tgz
	tar xvzf SParseval.tgz
	rm SParseval.tgz

SParseval/src/sparseval: SParseval
	rm -f SParseval/src/*.o
	$(MAKE) -C SParseval/src sparseval

# clean removes object files.
#
.PHONY: clean
clean:
	(cd $(NBESTPARSERBASEDIR); rm -f PARSE/*.o; rm -f TRAIN/*.o)
	$(MAKE) -C $(NBESTPARSERBASEDIR)/TRAIN clean
	$(MAKE) -C $(NBESTPARSERBASEDIR)/PARSE clean
	$(MAKE) -C second-stage clean
	rm -f python/bllipparser/CharniakParser.py* python/bllipparser/JohnsonReranker.py*

# nbesttrain-clean removes temporary files used in constructing the 20
# folds of n-best training data.
#
nbesttrain-clean:
	rm -fr $(TMP)

# train-clean gets rid of all data not essential for the runtime reranking parser. 
#
.PHONY: train-clean
train-clean: nbesttrain-clean
	rm -fr results
	$(MAKE) -C second-stage train-clean

# real-clean tries to get rid of all object and binary files to
# produce a version for distribution.  But Eugene writes new programs
# faster than I can make real-clean clean them up!
# NOTE: This erases the reranker model directory!
#
.PHONY: real-clean
real-clean: clean train-clean swig-clean
	$(MAKE) -C $(NBESTPARSERBASEDIR)/TRAIN real-clean
	$(MAKE) -C $(NBESTPARSERBASEDIR)/PARSE real-clean
	$(MAKE) -C second-stage real-clean

########################################################################
#                                                                      #
# nbesttrain -- Preparing the N-best training data for the reranker    #
#                                                                      #
########################################################################

# To build the 20-fold n-best data in second-stage/train 
# for training the ranker, run
#
# make nbesttrain 
#
# or
# 
# make -j 2 nbesttrain
#
# on a multiprocessor machine

# TRAIN specifies the location of the trees to be divided into NFOLDS
# This is defined here to use sections 2-21 of the Penn WSJ treebank.
#
TRAIN=$(PENNWSJTREEBANK)/0[2-9]/*mrg $(PENNWSJTREEBANK)/1[0-9]/*mrg $(PENNWSJTREEBANK)/2[0-1]/*mrg

# NBESTDIR is the directory that holds the n-best parses for training
# the reranker.
#
NBESTDIR=second-stage/nbest/$(NBESTPARSERNICKNAME)$(NPARSES)

# NBESTFILES are all of the files in the n-best folds, plus dev and test sections
#
NBESTFILES= $(foreach fold,$(FOLDS),$(NBESTDIR)/fold$(fold).gz) $(foreach section,$(SECTIONS),$(NBESTDIR)/section$(section).gz)

.PHONY: nbesttrain
nbesttrain: $(NBESTFILES) PARSE TRAIN second-stage/programs/prepare-data/ptb

# This goal copies and gzips the output of the n-best parser
# into the appropriate directory for training the reranker.
#
# .PRECIOUS: $(NBESTDIR)/fold%.gz
.INTERMEDIATE: $(NBESTDIR)/fold%.gz
$(NBESTDIR)/fold%.gz: $(TMP)/fold%/$(NBESTPARSERNICKNAME)$(NPARSES)best
	mkdir -p $(NBESTDIR)
	gzip -c $+ > $@

# The remaining goals in this section are for training and parsing
# with the n-best parser to produce the folds for training the
# reranker.

.INTERMEDIATE: $(TMP)/fold%/$(NBESTPARSERNICKNAME)$(NPARSES)best
$(TMP)/fold%/$(NBESTPARSERNICKNAME)$(NPARSES)best: $(TMP)/fold%/DATA $(TMP)/fold%/yield $(NBESTPARSER)
	$(EXEC) $(NBESTPARSER) -l400 -K -N$(NPARSES) $(@D)/DATA/ $(@D)/yield > $@

.INTERMEDIATE: $(TMP)/fold%/DATA
$(TMP)/fold%/DATA: $(TMP)/fold%/train $(TMP)/fold%/dev $(NBESTTRAINER)
	mkdir -p $@
	LC_COLLATE=C; cp $(NBESTPARSERBASEDIR)/DATA/EN/[a-z]* $@
	$(EXEC) $(NBESTTRAINER) $@ $(@D)/train $(@D)/dev

.INTERMEDIATE: $(TMP)/fold%/train
$(TMP)/fold%/train: second-stage/programs/prepare-data/ptb
	mkdir -p $(@D)
	$(EXEC) second-stage/programs/prepare-data/ptb -n $(NFOLDS) -x $(patsubst $(TMP)/fold%,%,$(@D)) -e $(TRAIN)  > $@

.INTERMEDIATE: $(TMP)/fold%/dev
$(TMP)/fold%/dev: second-stage/programs/prepare-data/ptb
	mkdir -p $(@D)
	second-stage/programs/prepare-data/ptb -n $(NFOLDS) -i $(patsubst $(TMP)/fold%,%,$(@D)) -e $(TRAIN)  > $@

# $(TMP)/fold%/DATA: $(TMP)/%/train $(TMP)/%/dev
# 	mkdir -p $@
# 	LC_COLLATE=C; cp $(NBESTPARSERBASEDIR)/DATA/EN/[a-z]* $@
# 	$(NBESTPARSERBASEDIR)/TRAIN/trainParser $@ $(@D)/train $(@D)/dev

.INTERMEDIATE: $(TMP)/fold%/yield
$(TMP)/fold%/yield: second-stage/programs/prepare-data/ptb
	mkdir -p $(@D)
	$(EXEC) second-stage/programs/prepare-data/ptb -n $(NFOLDS) -i $(patsubst $(TMP)/fold%,%,$(@D)) -c $(TRAIN) > $@

# .PRECIOUS: $(NBESTDIR)/section%.gz
.INTERMEDIATE: $(NBESTDIR)/section%.gz
$(NBESTDIR)/section%.gz: $(TMP)/section%/$(NBESTPARSERNICKNAME)$(NPARSES)best
	mkdir -p $(NBESTDIR)
	gzip -c $+ > $@

.INTERMEDIATE: $(TMP)/section%/$(NBESTPARSERNICKNAME)$(NPARSES)best
$(TMP)/section%/$(NBESTPARSERNICKNAME)$(NPARSES)best: $(TMP)/section%/yield $(NBESTPARSER)
	$(EXEC) $(NBESTPARSER) -l400 -K -N$(NPARSES) $(NBESTPARSERBASEDIR)/DATA/EN/ $(@D)/yield > $@

.INTERMEDIATE: $(TMP)/section%/yield
$(TMP)/section%/yield: second-stage/programs/prepare-data/ptb
	mkdir -p $(@D)
	$(EXEC) second-stage/programs/prepare-data/ptb -c $(PENNWSJTREEBANK)/$(patsubst $(TMP)/section%,%,$(@D))/wsj*.mrg  > $@

########################################################################
#                                                                      #
# touch-nbest marks the n-best parses as up-to-date                    #
#                                                                      #
########################################################################

.PHONY: touch-nbest
touch-nbest:
	touch $(NBESTDIR) $(NBESTDIR)/*

########################################################################
#                                                                      #
# nbest oracle evaluation                                              #
#                                                                      #
########################################################################

.PHONY: nbest-oracle
nbest-oracle: second-stage/programs/features/oracle-score second-stage/programs/prepare-data/ptb $(NBESTFILES)
	$(EXEC) second-stage/programs/features/oracle-score "$(ZCAT) $(NBESTDIR)/fold[0-1][0-9].gz" "second-stage/programs/prepare-data/ptb -g $(TRAIN)"
	$(EXEC) second-stage/programs/features/oracle-score "$(ZCAT) $(NBESTDIR)/section22.gz" "second-stage/programs/prepare-data/ptb -g $(PENNWSJTREEBANK)/22/wsj*.mrg"
	$(EXEC) second-stage/programs/features/oracle-score "$(ZCAT) $(NBESTDIR)/section24.gz" "second-stage/programs/prepare-data/ptb -g $(PENNWSJTREEBANK)/24/wsj*.mrg"

.PHONY: nbest-oracle-detailed
nbest-oracle-detailed: second-stage/programs/eval-beam/main second-stage/programs/prepare-data/ptb $(NBESTFILES)
	$(EXEC) second-stage/programs/eval-beam/main "$(ZCAT) $(NBESTDIR)/fold[0-1][0-9].gz" "second-stage/programs/prepare-data/ptb -g $(TRAIN)"
	$(EXEC) second-stage/programs/eval-beam/main "$(ZCAT) $(NBESTDIR)/section22.gz" "second-stage/programs/prepare-data/ptb -g $(PENNWSJTREEBANK)/22/wsj*.mrg"
	$(EXEC) second-stage/programs/eval-beam/main "$(ZCAT) $(NBESTDIR)/section24.gz" "second-stage/programs/prepare-data/ptb -g $(PENNWSJTREEBANK)/24/wsj*.mrg"

########################################################################
#                                                                      #
# extract-features extracts feature counts for training reranker       #
#                                                                      #
########################################################################

# FEATBASEDIR is the directory in which the feature counts will be saved,
# minus the $(VERSION) flag.
#
FEATBASEDIR=second-stage/features/$(NBESTPARSERNICKNAME)$(NPARSES)$(FEATURESNICKNAME)

# FEATDIR is the directory in which the feature counts will be saved.
#
FEATDIR=$(FEATBASEDIR)$(VERSION)

# MODELBASEDIR is the directory in which the features and feature
# weights are saved, minus the version.
#
MODELBASEDIR=second-stage/models/$(NBESTPARSERNICKNAME)$(NPARSES)$(FEATURESNICKNAME)

# MODELDIR is the directory in which the features and feature weights
# are saved.
#
MODELDIR=$(MODELBASEDIR)$(VERSION)

.PHONY: features
features: $(MODELDIR)/features.gz $(FEATDIR)/train.gz $(FEATDIR)/dev.gz $(FEATDIR)/test1.gz $(FEATDIR)/test2.gz

# This goal does feature extraction for reranker training for the
# nonfinal case (i.e., train is folds 0-17, dev is folds 18-19, test1
# is section 22 and test2 is section 24).
#
$(MODELBASEDIR)nonfinal/features.gz $(FEATBASEDIR)nonfinal/train.gz $(FEATBASEDIR)nonfinal/dev.gz $(FEATBASEDIR)nonfinal/test1.gz $(FEATBASEDIR)nonfinal/test2.gz: second-stage/programs/prepare-data/ptb $(FEATUREEXTRACTOR) $(NBESTFILES)
	mkdir -p $(FEATBASEDIR)nonfinal
	mkdir -p $(MODELBASEDIR)nonfinal
	$(EXEC) $(FEATUREEXTRACTOR) $(FEATUREEXTRACTORFLAGS) \
		"$(ZCAT) $(NBESTDIR)/fold0[0-9].gz $(NBESTDIR)/fold1[0-7].gz" \
		"second-stage/programs/prepare-data/ptb -g -n 10 -x 9 $(TRAIN)" \
		$(FEATBASEDIR)nonfinal/train.gz \
		"$(ZCAT) $(NBESTDIR)/fold1[8-9].gz" \
		"second-stage/programs/prepare-data/ptb -g -n 10 -i 9 $(TRAIN)" \
		$(FEATBASEDIR)nonfinal/dev.gz \
		"$(ZCAT) $(NBESTDIR)/section22.gz" \
		"second-stage/programs/prepare-data/ptb -g $(PENNWSJTREEBANK)/22/*mrg" \
		$(FEATBASEDIR)nonfinal/test1.gz \
		"$(ZCAT) $(NBESTDIR)/section24.gz" \
		"second-stage/programs/prepare-data/ptb -g $(PENNWSJTREEBANK)/24/*mrg" \
		$(FEATBASEDIR)nonfinal/test2.gz \
		| gzip > $(MODELBASEDIR)nonfinal/features.gz

# This goal does feature extraction for reranker training for the
# final case (i.e., train is folds 0-19, dev is section 24, test1
# is section 22 and test2 is section 23).
#
$(MODELBASEDIR)final/features.gz $(FEATBASEDIR)final/train.gz $(FEATBASEDIR)final/dev.gz $(FEATBASEDIR)final/test1.gz $(FEATBASEDIR)final/test2.gz: second-stage/programs/prepare-data/ptb $(FEATUREEXTRACTOR) $(NBESTFILES)
	mkdir -p $(FEATBASEDIR)final
	mkdir -p $(MODELBASEDIR)final
	$(EXEC) $(FEATUREEXTRACTOR) $(FEATUREEXTRACTORFLAGS) \
		"$(ZCAT) $(NBESTDIR)/fold*.gz" \
		"second-stage/programs/prepare-data/ptb -g $(TRAIN)" \
		$(FEATBASEDIR)final/train.gz \
		"$(ZCAT) $(NBESTDIR)/section22.gz" \
		"second-stage/programs/prepare-data/ptb -g $(PENNWSJTREEBANK)/22/*mrg" \
		$(FEATBASEDIR)final/test1.gz \
		"$(ZCAT) $(NBESTDIR)/section23.gz" \
		"second-stage/programs/prepare-data/ptb -g $(PENNWSJTREEBANK)/23/*mrg" \
		$(FEATBASEDIR)final/test2.gz \
		"$(ZCAT) $(NBESTDIR)/section24.gz" \
		"second-stage/programs/prepare-data/ptb -g $(PENNWSJTREEBANK)/24/*mrg" \
		$(FEATBASEDIR)final/dev.gz \
		| gzip > $(MODELBASEDIR)final/features.gz


########################################################################
#                                                                      #
# train-reranker estimates the reranker feature weights                #
#                                                                      #
########################################################################

WEIGHTSFILE=$(MODELDIR)/$(ESTIMATORNICKNAME)-weights
WEIGHTSFILEGZ=$(WEIGHTSFILE).gz

.PHONY: train-reranker
train-reranker: $(WEIGHTSFILEGZ)

# This goal estimates the reranker feature weights (i.e., trains the
# reranker).
#
# Don't use auto-renaming as in "gzip foo" because it fails if there is
# more than one hardlink on the file (I'm looking at you Time Machine!).
#
# $(WEIGHTSFILEGZ): $(ESTIMATOR)
$(WEIGHTSFILEGZ): $(ESTIMATOR) $(MODELDIR)/features.gz $(FEATDIR)/train.gz $(FEATDIR)/dev.gz $(FEATDIR)/test1.gz
	$(ESTIMATORENV) $(ZCAT) $(FEATDIR)/train.gz | $(EXEC) $(ESTIMATOR) $(ESTIMATORFLAGS) -e $(FEATDIR)/dev.gz -f $(MODELDIR)/features.gz -o $(WEIGHTSFILE) -x $(FEATDIR)/test1.gz
	gzip -c $(WEIGHTSFILE) >$(WEIGHTSFILEGZ)
	rm -f $(WEIGHTSFILE)

########################################################################
#                                                                      #
# eval-reranker evaluates the reranker on the two test data sets       #
#                                                                      #
########################################################################

EVALDIR=second-stage/eval/$(NBESTPARSERNICKNAME)$(NPARSES)$(FEATURESNICKNAME)$(VERSION)-$(ESTIMATORNICKNAME)

.PHONY: eval-reranker
eval-reranker: $(EVALDIR)/weights-eval # $(EVALDIR)/dev-parsediffs.gz

$(EVALDIR)/weights-eval: $(WEIGHTSFILEGZ) $(MODELDIR)/features.gz $(FEATDIR)/dev.gz $(FEATDIR)/test1.gz $(FEATDIR)/test2.gz second-stage/programs/eval-weights/eval-weights
	mkdir -p $(EVALDIR)
	$(ZCAT) $(WEIGHTSFILEGZ) | second-stage/programs/eval-weights/eval-weights $(EVALWEIGHTSARGS) $(MODELDIR)/features.gz $(FEATDIR)/dev.gz > $(EVALDIR)/weights-eval
	$(ZCAT) $(WEIGHTSFILEGZ) | second-stage/programs/eval-weights/eval-weights $(EVALWEIGHTSARGS) $(MODELDIR)/features.gz $(FEATDIR)/test1.gz >> $(EVALDIR)/weights-eval
	$(ZCAT) $(WEIGHTSFILEGZ) | second-stage/programs/eval-weights/eval-weights $(EVALWEIGHTSARGS) $(MODELDIR)/features.gz $(FEATDIR)/test2.gz >> $(EVALDIR)/weights-eval

$(EVALDIR)/dev-parsediffs.gz: $(WEIGHTSFILEGZ) $(FEATDIR)/test1.gz $(NBESTDIR)/section24.gz second-stage/programs/eval-weights/best-indices second-stage/programs/eval-weights/best-parses second-stage/programs/eval-weights/pretty-print
	$(ZCAT) $(WEIGHTSFILEGZ) \
	 | second-stage/programs/eval-weights/best-indices $(FEATDIR)/test1.gz \
	 | second-stage/programs/eval-weights/best-parses $(NBESTDIR)/section24.gz \
	 | second-stage/programs/eval-weights/pretty-print -d \
	 | gzip > $(EVALDIR)/dev-parsediffs.gz

######################################################
#                                                    #
# swig-java builds SWIG wrapper extensions for Java  #
#                                                    #
######################################################

# These paths are likely not very portable and may need to be edited

# this should be the path to jni.h
SWIG_JAVA_GCCFLAGS ?= -I/usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include/ \
	-I/usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include/linux/
# -L should have the path to libstdc++.so
SWIG_LINKER_FLAGS ?= -lstdc++ -L/usr/lib/gcc/x86_64-redhat-linux/4.4.4/
export SWIG_JAVA_GCCFLAGS
export SWIG_LINKER_FLAGS

swig-java: CXXFLAGS += -fPIC -fno-strict-aliasing -Wno-deprecated
swig-java: PARSE reranker-runtime
	$(MAKE) -C $(NBESTPARSERBASEDIR)/PARSE swig-java
	$(MAKE) -C second-stage/programs/features swig-java

swig-clean:
	$(MAKE) -C $(NBESTPARSERBASEDIR)/PARSE swig-clean
	$(MAKE) -C second-stage/programs/features swig-clean
