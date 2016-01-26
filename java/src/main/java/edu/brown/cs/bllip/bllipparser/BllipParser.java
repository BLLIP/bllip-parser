/*
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
package edu.brown.cs.bllip.bllipparser;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.fizzed.jne.JNE;

import org.apache.commons.io.FilenameUtils;

import edu.brown.cs.bllip.bllipparser.swig.ExtPos;
import edu.brown.cs.bllip.bllipparser.swig.InputTree;
import edu.brown.cs.bllip.bllipparser.swig.NBestList;
import edu.brown.cs.bllip.bllipparser.swig.RerankerModel;
import edu.brown.cs.bllip.bllipparser.swig.SWIGParser;
import edu.brown.cs.bllip.bllipparser.swig.SWIGReranker;
import edu.brown.cs.bllip.bllipparser.swig.ScoredTree;
import edu.brown.cs.bllip.bllipparser.swig.SentRep;
import edu.brown.cs.bllip.bllipparser.swig.VectorScoredTree;
import edu.brown.cs.bllip.bllipparser.swig.Weights;

public class BllipParser {
  private static Pattern closeSTag;
  private static Pattern escapedCloseSTag;

  private static boolean alreadyLoadedNativeCode = false;
  // we can only load one parsing model at a time.  if this is null, we haven't loaded any model yet.  otherwise, it is the parsing model directory currently loaded
  private static String alreadyLoadedParsingModel = null;

  private ParserOptions options;

  private static RerankerModel rerankerModel = null;

  static {
    // these pull out the specific casing of the S tags so we can keep it intact
    closeSTag = Pattern.compile("</(S)>", Pattern.CASE_INSENSITIVE);
    escapedCloseSTag = Pattern.compile("CLOSE_(S)_TAG", Pattern.CASE_INSENSITIVE);
  }

  public BllipParser(ParserOptions options) {
    options.sanityCheck();
    this.options = options;

    loadNativeCode(options.usingReranker());

    // set options and load models
    loadParsingModelAndSetOptions();
    if (options.usingReranker()) {
      SWIGReranker.setOptions(options.rerankerDebugLevel, options.relativeCounts);
      // given the thread safety issues in the parser (see below) we only allocate a single version
      // of the reranker model as well.
      if (rerankerModel == null) {
        rerankerModel = new RerankerModel(options.rerankerFeatureClass, options.rerankerFeaturesFilename, options.rerankerWeightsFilename);
      }
    }
  }

  public synchronized void loadParsingModelAndSetOptions() {
    // make sure that we normalize these paths so that we can safely compare them later
    options.parserModelDirectory = FilenameUtils.normalizeNoEndSeparator(options.parserModelDirectory);

    // if we haven't loaded the model yet or a different model is loaded
    if (alreadyLoadedParsingModel == null || !alreadyLoadedParsingModel.equals(options.parserModelDirectory)) {
      SWIGParser.loadModel(options.parserModelDirectory);

      alreadyLoadedParsingModel = options.parserModelDirectory;
    }

    SWIGParser.setOptions(options.parserLanguage, options.parserCaseInsensitive, options.numParses,
            options.parserExtraSmoothing, options.parserOverParsing, options.parserDebugLevel, 0);
  }

  public ParserQuery parse(ParserQuery query) {
    if (!alreadyLoadedParsingModel.equals(options.parserModelDirectory)) {
      throw new RuntimeException("You've constructed multiple BllipParser objects with different parsing model directories. The native code can only handle one parsing model loaded at a time and this BllipParser object no longer has its model loaded.");
    }

    List<Parse> nBestList = new ArrayList<Parse>();
    VectorScoredTree scoredTreeVector;

    // reset these structures in case we've already parsed this query (perhaps with different constraints)
    query.setParses(nBestList);
    query.setFailureDescription(null);

    // convert ParserQuery into appropriate inputs
    SentRep sentRep = query.makeSentRep(options);
    ExtPos extPos = query.makeExtPos();

    // fail fast if sentence is too long
    if (sentRep.length() > options.maxSentenceLengthToParse) {
      if (sentRep.length() > options.maxSentenceLength) {
        // if this is true, we're in bigger trouble and can't easily create the full failure tree
        // we'll assume it's fine to just use the first maxSentenceLength tokens.
        String[] truncatedTokens = new String[options.maxSentenceLength];
        for (int tokenIndex = 0; tokenIndex < options.maxSentenceLength; tokenIndex++) {
          truncatedTokens[tokenIndex] = sentRep.getWord(tokenIndex).lexeme();
        }
        query.setText(null);
        query.setTokens(truncatedTokens);
        query.makeAndSetFailureTree("Sentence is WAY too long", options);
      } else {
        query.makeAndSetFailureTree("Sentence is too long", options);
      }

      return query;
    }

    // the real parsing happens here
    if (extPos == null) { // without external POS constraints
      try {
        scoredTreeVector = SWIGParser.parse(sentRep);
      } catch (RuntimeException re) {
        query.makeAndSetFailureTree("SWIGParser exception: " + re.getMessage(), options);
        return query;
      }
    } else { // with external POS constraints
      if (sentRep.length() != extPos.size()) {
        throw new RuntimeException("ExtPos constraints don't match the length of the sentence (extPos: " + extPos.size() + ", sentence: " + sentRep.length() + ")");
      }

      try {
        scoredTreeVector = SWIGParser.parse(sentRep, extPos, null);
      } catch (RuntimeException re) {
        System.err.println("SWIGParser exception:");
        re.printStackTrace();
        query.makeAndSetFailureTree("SWIGParser exception: " + re.getMessage(), options);
        return query;
      }
    }

    // make a failure tree (which includes most frequent POS tags) in the rare cases where we
    // couldn't parse the whole sentence
    if (scoredTreeVector.size() == 0) {
      query.makeAndSetFailureTree("No parses", options);
      return query;
    }

    // the real reranking happens here (if desired)
    Weights rerankerScores = null;
    if (options.usingReranker()) {
      // TODO: support sentence IDs. for now, they are always dummy.
      String nBestListAsText = SWIGParser.asNBestList(scoredTreeVector, "dummy");
      NBestList rerankerInput = SWIGReranker.readNBestList(nBestListAsText, options.rerankerCaseInsensitive);
      rerankerScores = rerankerModel.scoreNBestList(rerankerInput);
    }

    // convert scoredTreeVector to a (more usable) list of Parse objects
    for (int i = 0; i < scoredTreeVector.size(); i++) {
      ScoredTree scoredParse = scoredTreeVector.get(i);
      double parserScore = scoredParse.getFirst();
      InputTree inputTree = scoredParse.getSecond();
      String parseInPtb = inputTree.toString();
      parseInPtb = unescapeText(parseInPtb);
      Parse parse = new Parse(parseInPtb, parserScore);
      parse.setParserRank(i);
      if (rerankerScores != null) {
        parse.setRerankerScore(rerankerScores.get(i));
      }
      nBestList.add(parse);
    }

    query.setParses(nBestList);

    if (options.sortNBestListByRerankerScores) {
      query.sortByRerankerScores();
    }

    return query;
  }

  /**
   * Escapes characters which confuse the Charniak parser's tokenizer.
   */
  public static String escapeText(String text) {
    text = text.replace("^^", " CARET_CARET ");

    Matcher matcher = closeSTag.matcher(text);
    text = matcher.replaceAll(" CLOSE_$1_TAG ");

    return text;
  }

  /**
   * Unescape text previously escaped by the parser. Note that this is not an exact inverse of
   * escapeText(). This is because escapeText adds spaces to ensure that the characters it escapes
   * create tokenization boundaries. This also unescapes standard PTB escapes (left paren -> -LRB-,
   * e.g.).
   */
  public static String unescapeText(String text) {
    text = text.replace("CARET_CARET", "^^");
    Matcher matcher = escapedCloseSTag.matcher(text);
    text = matcher.replaceAll("</$1>");

    return text;
  }

  public ParserOptions getOptions() {
    return options;
  }

  public void setOptions(ParserOptions options) {
    this.options = options;
  }

  /*
   * Utility methods
   */

  public synchronized static void loadNativeCode(boolean usingReranker) {
    if (alreadyLoadedNativeCode) {
      return;
    }

    JNE.loadLibrary("SWIGParser");
    JNE.loadLibrary("SWIGReranker");
  }

  public static void main(String[] args) {
    // quick testing code
    String base = args[0];

    String parserModel = base + "/parser";
    String rerankerFeatures = base + "/reranker/features.gz";
    String rerankerWeights = base + "/reranker/weights.gz";

    ParserOptions options = new ParserOptions(parserModel, rerankerFeatures, rerankerWeights);
    options.numParses = 50;
    options.maxSentenceLengthToParse = 60;
    BllipParser bllip = new BllipParser(options);
    ParserQuery query = new ParserQuery("Parse this text, please.");
    bllip.parse(query);
    System.out.println(query.getTopPennParse());
  }
}
