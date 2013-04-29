// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License.  You may obtain
// a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.

// best-parses.cc
//
// Mark Johnson, 30th August 2005, last modified 2nd December 2007

const char usage[] =
  "best-parses version of 2nd December 2007\n"
  "\n"
  "Usage:\n"
  "\n"
  "best-parses [-a] [-l] [-m mode] feat-defs.bz2 feat-weights.bz2 < nbest-parses > best-parses\n"
  "\n"
  "where:\n"
  "\n"
  " -f <f>, use features <f> (must agree with extract-features)\n"
  " -a don't use absolute counts (slower),\n"
  " -d <debuglevel> sets the amount of debugging output,\n"
  " -l maps all words to lower case as trees are read,\n"
  " -m <mode>, where the output depends on <mode>:\n"
  "    0 print 1-best tree,\n"
  "    1 print ranked parses,\n"
  "    2 print feature counts,\n"
  "    3 print 1-best tree with syntactic heads,\n"
  "    4 print 1-best tree with semantic heads,\n"
  "\n"
  " feat-defs.bz2 is a feature definition file produced by extract-spfeatures, and\n"
  " feat-weights.bz2 is a feature weight file\n"
  "\n"
  "The program reads n-best parses from stdin, and writes the best parse to stdout.\n";

#include "custom_allocator.h"       // must be first

// #include <boost/lexical_cast.hpp>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

#include "popen.h"
#include "sp-data.h"
#include "features.h"

int debug_level = 0;
bool absolute_counts = true;
bool collect_correct = false;
bool collect_incorrect = false;

int main(int argc, char **argv) {

  bool lowercase_flag = false;
  int mode = 0;

  std::ios::sync_with_stdio(false);
  const char* fcname = NULL;

  int c;
  while ((c = getopt(argc, argv, "ad:f:lm:")) != -1 )
    switch (c) {
    case 'a':
      absolute_counts = false;
      break;
    case 'd':
      debug_level = atoi(optarg);
      break;
    case 'f':
      fcname = optarg;
      break;
    case 'l':
      lowercase_flag = true;
      break;
    case 'm':
      mode = atoi(optarg);
      break;
    default:
      std::cerr << "## Error: can't interpret argument " << c << " " << optarg << std::endl;
      std::cerr << usage << std::endl;
      exit(EXIT_FAILURE);
    }

  if (argc - optind != 2) {
    std::cerr << "## Error: missing required arguments.\n" << usage << std::endl;
    exit(EXIT_FAILURE);
  }

  if (debug_level > 0)
    std::cerr 
      << "# lowercase_flag (-l) = " << lowercase_flag
      << std::endl;

  // initialize feature classes
  //
  FeatureClassPtrs fcps(fcname);

  izstream fdin(argv[optind]);
  if (!fdin) {
    std::cerr << "## Error: can't open feature definition file " << argv[optind] 
	      << "\n" << usage << std::endl;
    exit(EXIT_FAILURE);
  }
  Id maxid = fcps.read_feature_ids(fdin);
  // std::cout << fcps << std::endl;

  izstream fwin(argv[optind+1]);
  if (!fwin) {
    std::cerr << "## Error: can't open feature weights file " << argv[optind+1]
	      << "\n" << usage << std::endl;
    exit(EXIT_FAILURE);
  }

  std::vector<Float> weights(maxid+1);
  Id id;
  Float weight;
  while (fwin >> id >> "=" >> weight) {
    assert(id <= maxid);
    assert(weights[id] == 0);
    weights[id] = weight;
  }
  
  sp_sentence_type s;
  while (s.read(std::cin, lowercase_flag)) {
    switch (mode) {
    case 0:
      write_tree_noquote_root(std::cout, fcps.best_parse(s, weights));
      std::cout << std::endl;
      break;
    case 1:
      fcps.write_ranked_trees(s, weights, std::cout);
      break;
    case 2:
      fcps.write_features_debug(s, weights, std::cout);
      break;
    case 3:
      write_tree_noquote_root_with_heads(std::cout, fcps.best_parse(s, weights), true);
      std::cout << std::endl;
      break;
    case 4:
      write_tree_noquote_root_with_heads(std::cout, fcps.best_parse(s, weights), false);
      std::cout << std::endl;
      break;
    default:
      std::cerr << "## Error: unknown mode = " << mode << std::endl;
      exit(EXIT_FAILURE);
      break;
    }
  }

} // main()
