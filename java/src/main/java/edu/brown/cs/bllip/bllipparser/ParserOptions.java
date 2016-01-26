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

import java.io.File;
import java.io.IOException;

import org.apache.commons.lang3.builder.ToStringBuilder;

/**
 * Represents all the options that BllipParser accepts. Different constructors use different
 * components and set the components flag accordingly. These constructors take the required options
 * for those components, all others can be set as fields.
 *
 * Potentially confusing is that there are two parameters involving sentence length.
 * maxSentenceLength is the largest number of tokens we ever expect to see in a sentence. Most parts
 * of BllipParser expect this to be 399 and changing it is not recommended. If a sentence is longer
 * than this, it will be cropped at maxSentenceLength tokens.
 *
 * maxSentenceLengthToParse is the longest sentence that we will attempt to parse. If a sentence is
 * longer than this, we will create a failed parse tree instead of parsing the sentence.
 */
public class ParserOptions {
  //
  // General options
  //

  // sets whether you're using the parser or reranking parser
  public Components components = Components.RERANKING_PARSER;
  // whether n-best list should be sorted by parser scores (false) or reranker scores (true)
  public boolean sortNBestListByRerankerScores = false;
  public int maxSentenceLengthToParse = 60; // see main Javadocs for a description of this field

  //
  // Parser options
  //

  // the only required parser parameter -- defaults will be used in all other cases
  public String parserModelDirectory = null;

  // Run mode
  public int numParses = 50; // -N
  public int parserDebugLevel = 0; // -d

  // Performance/quality
  public boolean parserExtraSmoothing = true; // -s
  public Double parserOverParsing = 21.0; // -T
  public Double parserExtraPosSmoothing = 0.0; // -P

  // Input
  public boolean parserCaseInsensitive = false; // -C
  public String parserLanguage = "En"; // -L
  public int maxSentenceLength = 399; // -l

  //
  // Reranker options
  //

  // required reranker options
  public String rerankerFeaturesFilename = null;
  public String rerankerWeightsFilename = null;

  public String rerankerFeatureClass = null; // -f
  // oops, this probably should have been set to true. doesn't affect rankings, just scores.
  public boolean relativeCounts = false; // -a
  public int rerankerDebugLevel = 0; // -d

  // this option is intentionally independent of parserCaseInsensitive since typical usage is to
  // have the parser case sensitive and the reranker lowercase everything
  public boolean rerankerCaseInsensitive = true; // -l

  /**
   * Constructs options for using the parser only.
   */
  public ParserOptions(String parserModel) {
    parserModelDirectory = parserModel;
    components = Components.PARSER_ONLY;
  }

  /**
   * Constructs options for using the reranking parser.
   */
  public ParserOptions(String parserModel, String rerankerFeatures, String rerankerWeights) {
    parserModelDirectory = parserModel;
    rerankerFeaturesFilename = rerankerFeatures;
    rerankerWeightsFilename = rerankerWeights;
    components = Components.RERANKING_PARSER;
    sortNBestListByRerankerScores = true;
  }

  /**
   * Convenience function to tell you if the reranker is enabled.
   */
  public boolean usingReranker() {
    return components == Components.RERANKING_PARSER;
  }

  /**
   * Makes sure that options are consistent and that required model files exist.
   *
   * @throws RuntimeException if anything is insane.
   */
  public void sanityCheck() {
    if (numParses < 1) {
      throw new RuntimeException("numParses must be at least 1 (currently " + numParses + ")");
    }

    Common.checkReadableOrThrowError("Parser model", parserModelDirectory, true);

    if (usingReranker()) {
      if (numParses <= 1) {
        throw new RuntimeException("Can't use reranker without multiple parses (set numParses > 1)");
      }
      Common.checkReadableOrThrowError("Reranker features", rerankerFeaturesFilename, false);
      Common.checkReadableOrThrowError("Reranker weights", rerankerWeightsFilename, false);
    } else {
      if (sortNBestListByRerankerScores) {
        throw new RuntimeException("Can't sort by reranker scores if not using the reranker.");
      }
    }

    if (maxSentenceLength < 1 || maxSentenceLength > 399) {
      throw new RuntimeException("maxSentenceLength must be >=1 and <= 399 (currently " + maxSentenceLength + ")");
    }

    if (maxSentenceLengthToParse > maxSentenceLength) {
      throw new RuntimeException("maxSentenceLengthToParse can't be greater than maxSentenceLength");
    }
  }

  @Override
  public String toString() {
    return ToStringBuilder.reflectionToString(this);
  }

  public static ParserOptions fromUnifiedModel(String unifiedModelPath) {
    try {
      File featuresFile = new File(unifiedModelPath, "reranker/features.gz");
      File weightsFile = new File(unifiedModelPath, "reranker/weights.gz");
      if (!featuresFile.exists()) {
        featuresFile = new File(unifiedModelPath, "reranker/features.bz2");
      }
      if (!weightsFile.exists()) {
        weightsFile = new File(unifiedModelPath, "reranker/weights.bz2");
      }
      return new ParserOptions(new File(unifiedModelPath, "parser").getCanonicalPath(),
              featuresFile.getCanonicalPath(),
              weightsFile.getCanonicalPath());
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }

  //
  // enumeration(s)
  //

  public enum Components {
    PARSER_ONLY, RERANKING_PARSER,
  }
}
