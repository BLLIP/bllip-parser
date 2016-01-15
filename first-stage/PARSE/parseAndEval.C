/*
 * Copyright 1999, 2005 Brown University, Providence, RI.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.  You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include <pthread.h>
#include <time.h>
#include <sys/resource.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <math.h>
#include "Field.h"
#include "GotIter.h"
#include "Wrd.h"
#include "InputTree.h"
#include "Bchart.h"
#include "ECArgs.h"
#include "MeChart.h"
#include "headFinder.h"
#include "Params.h"
#include "AnsHeap.h"
#include "TimeIt.h"
#include "extraMain.h"
#include "Link.h"
#include "ClassRule.h"
#include "ScoreTree.h"
 
/* I need to syncronize three times for each sentence,
one to read it in from a common file, one to write it out
to a common file, and once to store how well I did on
the sentence in running total variable.  One a single
processer, each sentence takes about .7 seconds.
*/
pthread_mutex_t readlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writelock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t scorelock = PTHREAD_MUTEX_INITIALIZER;

Params 		    params;
int numThreads=2;
int sentenceCount = 0;
int printCount =0;
double  totGram  = 0.0;
double  totMix  = 0.0;
double  totTri  = 0.0;
int  totWords = 0;
bool histPoints[1000];
ParseStats  totPst[1000];

extern LeftRightGotIter* globalGi[MAXNUMTHREADS];

/* In order to print out the data in the correct order each
thread has it's own PrintStack which stores the output data
(printStrict) until it is time to print it out in order.
*/
typedef struct printStruct{
  int                sentenceCount;
  int                numDiff;
  vector<InputTree*> trees;
  vector<double>     probs;
} printStruct;

typedef list<printStruct> PrintStack;

/* the arguments to the thread function are stored in this struct */
typedef struct loopArg{
  int      id;
  istream* inpt;
  ostream* outpt;
} loopArg;

void
outputExtPos(list<ECString>& wtList,vector<ECString>& poslist)
{
  list<ECString>::iterator wtlI=wtList.begin();
  vector<ECString>::iterator plI=poslist.begin();
  for(;wtlI!=wtList.end();wtlI++){
    cout<<*wtlI<<" "<<*plI<<"\n";
    plI++;
  }
  cout<<"---\n";
}


/* the function called by each thread is "mainLoop" */
void*
mainLoop(void* arg)
{
  loopArg *loopA = (loopArg*)arg;
  istream* testSStream = loopA->inpt;
  ostream* pstatStream = loopA->outpt;
  int id = loopA->id;
  double log600 = log2(600.0);
  PrintStack printStack;
  for( ;  ; )
    {
      InputTree     correct;  
      InputTree*    cuse;

      /* first lock to read in the material */
      pthread_mutex_lock(&readlock);
      if( !*testSStream ) {
	pthread_mutex_unlock(&readlock);
	break;
      }
      *testSStream >> correct;
      if( !*testSStream ){
	pthread_mutex_unlock(&readlock);
	break;
      }
      totWords += correct.length()+1;
      int locCount = sentenceCount++;
      list<ECString>  wtList;
      correct.make(wtList);
      SentRep sr( wtList );  // used in precision calc

      ExtPos extPos;
      if(params.extPosIfstream)
	extPos.read(params.extPosIfstream,sr);
      pthread_mutex_unlock(&readlock);

      cuse = &correct;
      int len = correct.length();
      if(len > params.maxSentLen) continue;
      //cerr << "Len = " << len << endl;
      /*
	if( !params.field().in(sentenceCount) )
	{
	sentenceCount++;
	continue;
	}
	if(sentenceCount < -1)
	{
	sentenceCount++;
	continue;
	}
	sentenceCount++;
      */
      vector<ECString> poslist;
      correct.makePosList(poslist);
      ScoreTree sc;
      sc.setEquivInts(poslist);
      MeChart*	chart = new MeChart( sr,extPos,id );
       
      chart->parse( );
      Item* topS = chart->topS();
      if(!topS)
	{
	  cerr << "Parse failed" << endl;
	  cerr << correct << endl;
	  error(" could not parse "); 
	  delete chart;
	  continue;
	}
       
      // compute the outside probabilities on the items so that we can
      // skip doing detailed computations on the really bad ones 

      chart->set_Alphas();

      Bst& bst = chart->findMapParse();
      if( bst.empty()) error( "mapProbs did not return answer");
      float bestF = -1;
      int i;
      int numVersions = 0;
      Link diffs(0);
      //cerr << "Need num diff: " << Bchart::Nth << endl;
      printStruct printS;
      printS.sentenceCount = locCount;
      printS.numDiff = 0;
      for(numVersions = 0 ; ; numVersions++)
	{
	  short pos = 0;
	  Val* val = bst.next(numVersions);
	  if(!val)
	    {
	      //cerr << "Breaking" << endl;
	      break;
	    }
	  InputTree*  mapparse = inputTreeFromBsts(val,pos,sr);
	  bool isU;
	  int dummy = 0;
	  diffs.is_unique(mapparse, isU, dummy);
	  // cerr << "V " << isU << " " << numVersions << *mapparse << endl;
	  if(isU)
	    {
	      printS.probs.push_back(val->prob());
	      printS.trees.push_back(mapparse);
	      printS.numDiff++;
	    }
	  else
	    {
	      delete mapparse;
	    }
	  if(printS.numDiff >= Bchart::Nth) break;
	  if(numVersions > 20000) break;
	}

      ParseStats* locPst = new ParseStats[Bchart::Nth];
      ParseStats bestPs;
      for(i = 0 ; i <printS.numDiff ; i++)
	{
	  InputTree *mapparse = printS.trees[i];
	  assert(mapparse);
	  sc.trips.clear();
	  ParseStats pSt;
	  sc.recordGold(cuse,pSt);
	  sc.precisionRecall(mapparse,pSt);
	  float newF = pSt.fMeasure();
	  cerr << printS.sentenceCount << "\t" << newF << endl;
	  if(newF > bestF)
	    {
	      bestF = newF;
	      bestPs = pSt;
	    }
	  if(histPoints[i])
	    {
	      locPst[i] += bestPs;
	    }
	}
      if(printS.numDiff < Bchart::Nth)
	{
	  for(i = printS.numDiff ; i < Bchart::Nth ; i++)
	    {
	      if(histPoints[i]) locPst[i] += bestPs;
	    }
	}

      pthread_mutex_lock(&scorelock);
      for(i = 0 ; i < Bchart::Nth ; i++) totPst[i]+=locPst[i];
      pthread_mutex_unlock(&scorelock);

      int numPrinted;

      /* put the sentence with which we just finished at the end of the printStack*/
      printStack.push_back(printS);
      PrintStack::iterator psi = printStack.begin();
      /* now look at each item from the front of the print stack
	 to see if it should be printed now */
      pthread_mutex_lock(&writelock);
      for( numPrinted =0; psi != printStack.end(); numPrinted++ )
	{
	  printStruct& pstr=(*psi);
	  if(pstr.sentenceCount != printCount) break;
	  *pstatStream << pstr.sentenceCount << "\t" << pstr.numDiff << "\n";
	  printCount++;
	  for(i = 0 ; i < pstr.numDiff ; i++)
	    {
	      InputTree*  mapparse = pstr.trees[i];
	      assert(mapparse);
	      double logP =log2(pstr.probs[i]);
	      logP -= (sr.length()*log600);
	      *pstatStream <<  logP << "\n";
	      if(Bchart::prettyPrint) *pstatStream << *mapparse << "\n\n";
	      else
		{
		  mapparse->printproper(*pstatStream);
		  *pstatStream << "\n";
		}
	      delete mapparse;
	    }
	  *pstatStream << endl;
	  psi++;
	}
      pthread_mutex_unlock(&writelock);
      for(i = 0 ; i < numPrinted ; i++) printStack.pop_front();
      if(Feature::isLM)
	{
	  double lgram = log2(bst.sum());
	  lgram -= (sr.length()*log600);
	  double pgram = pow(2,lgram);
	  double iptri = chart->triGram();;
	  double ltri = (log2(iptri)-sr.length()*log600);
	  double ptri = pow(2.0,ltri);
	  double pcomb1 = (0.667 * pgram)+(0.333 * ptri);
	  double lcom1 = log2(pcomb1);
	  totGram -= lgram;
	  totTri -= ltri;
	  totMix -= lcom1;
	  if(locCount%10 == 9)
	    {
	      cerr << locCount << "\t";
	      cerr << pow(2.0,totGram/(double)totWords);
	      cerr <<"\t" <<  pow(2.0,totTri/(double)totWords);
	      cerr << "\t" << pow(2.0,totMix/(double)(totWords));
	      cerr << endl;
	    }
	}
      if(locCount%50 == 1)
	{
	  cerr << sentenceCount << "\t";
	  for(int i = 0 ; i < Bchart::Nth ; i++)
	    if(histPoints[i])
	      {
		cerr << i << " " << totPst[i].fMeasure() << "\t";
	      }
	  cerr << endl;
	}

      delete chart;
      delete [] locPst;
    }
  return 0;
}

int
main(int argc, char *argv[])
{
   ECArgs args( argc, argv );
   //AnsTreeHeap::print = true;
   /* o = basic, but not debugging, output.
      l = length of sentence to be proceeds 0-40 is default
      n = work on each #'th line.
      d = print out debugging info at level #
      t = report timings (requires o)
   */

   int i;
   params.init( args );
   //cerr << "Starting wwBCTest " << Feature::sLM << endl;

   if( args.nargs() > 2 || args.nargs() < 2 )	// require path name 
     error( "Need exactly two args.");
   ECString  path( args.arg( 0 ) );
   generalInit(path);
   for(int i = 0 ; i < 500 ; i++) histPoints[i] = false;
   histPoints[0] = true;
   if (Bchart::Nth == 50 || Bchart::Nth == 500 || Bchart::Nth == 1000)
     histPoints[1] = histPoints[9] = histPoints[24] = histPoints[49] = true;
   if (Bchart::Nth == 500 || Bchart::Nth == 1000)
     histPoints[99] = histPoints[249] = histPoints[499] = true;
   if(Bchart::Nth == 1000)
     histPoints[749] = histPoints[999] = true;

   TimeIt timeIt;

   ECString testSString = args.arg(1);
   ifstream testSStream(testSString.c_str());
   if( !testSStream )
     {
       cerr << "Could not find " << testSString << endl;
       error( "No testSstream");
     }

    ECString      pstatStreamName( params.fileString());
    pstatStreamName  += "PStatInfo/pStat";
    pstatStreamName += params.numString();
    pstatStreamName += ".txt";
    ofstream    pstatStream( pstatStreamName.c_str(), ios::out);
    if( !pstatStream )
      {
	cerr << "Looking to output to " << pstatStreamName << endl;
	error( "unable to open pstat stream");
      }

   for(i = 0 ; i < MAXNUMTHREADS ; i++) globalGi[i] =NULL;

   pthread_t thread[MAXNUMTHREADS];
   loopArg lA[MAXNUMTHREADS];
   for(i = 0 ; i < numThreads  ; i++){
     lA[i].id = i;
     lA[i].inpt=&testSStream;
     lA[i].outpt=&pstatStream;
     pthread_create(&thread[i],0,mainLoop, (void*)&lA[i]);
   }
  for(i=0; i<numThreads; i++){
    pthread_join(thread[i],0);
  }

   for(int i = 0 ; i < Bchart::Nth ; i++)
     if(histPoints[i])
       {
	 cerr << i << " " << totPst[i].fMeasure() << "\t";
       }
   cerr << endl;
   if(Feature::isLM)
     {
       cerr << pow(2.0,totGram/(double)totWords);
       cerr <<"\t" <<  pow(2.0,totTri/(double)totWords);
       cerr << "\t" << pow(2.0,totMix/(double)(totWords));
       cerr << endl;
     }
   if( args.isset('t') ) timeIt.finish(sentenceCount);
   pthread_exit(0);

   return 0;
}
