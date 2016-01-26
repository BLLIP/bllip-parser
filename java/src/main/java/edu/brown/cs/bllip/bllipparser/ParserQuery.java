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
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import org.apache.commons.lang3.builder.CompareToBuilder;

import edu.brown.cs.bllip.bllipparser.swig.ExtPos;
import edu.brown.cs.bllip.bllipparser.swig.InputTree;
import edu.brown.cs.bllip.bllipparser.swig.SWIGParser;
import edu.brown.cs.bllip.bllipparser.swig.SentRep;
import edu.brown.cs.bllip.bllipparser.swig.StringList;
import edu.brown.cs.bllip.bllipparser.swig.StringVector;
import edu.brown.cs.bllip.bllipparser.Common;

/**
 * Represents a single piece of text (sentence or utterance) to be parsed. When fed to
 * BllipParser.parse(), the results (parses and possibly error messages) from the parser will
 * be populated.  The text can be specified as an untokenized string or as pre-tokenized text.
 */
public class ParserQuery {
  protected String text = null;
  protected String[] tokens = null;
  protected List<Parse> parses = null;
  protected String failureDescription = null;
  protected Map<Integer, List<String>> tagConstraints = null;

  // these are cached and wiped out by some of the setters
  private SentRep sentRep = null;
  private ExtPos extPos = null;

  public ParserQuery(String text) {
    this.text = text;
  }

  public ParserQuery(String[] tokens) {
    this.tokens = tokens;
  }

  @Override
  public String toString() {
    StringBuilder builder = new StringBuilder();
    builder.append("ParserQuery[");
    if (text != null) {
      builder.append("text=\"").append(text).append("\", ");
    }
    if (tokens != null) {
      builder.append("tokens=").append(Arrays.toString(tokens)).append(", ");
    }
    if (getTopPennParse() != null) {
      builder.append("getTopPennParse()=\"").append(getTopPennParse()).append("\", ");
    }
    builder.append("getNumParses()=").append(getNumParses()).append("]");
    return builder.toString();
  }

  /**
   * Outputs the n-best list in the same format as the command-line reranker.
   */
  public String toRerankerOutputFormat() {
    StringBuffer nbestList = new StringBuffer();
    nbestList.append(parses.size()).append(" d\n");
    if (failureDescription != null) {
      nbestList.append("fail ").append(failureDescription).append("\n")
      .append(parses.get(0).getPennParse()).append("\n");
    } else {
      if (parses.size() == 0) { // TODO possibly can be safely removed
        System.out.println("This ParserQuery:");
        System.out.println(toString());
        throw new IllegalStateException("ParserQuery with no parses but no failure message.");
      }
      for (Parse parse : parses) {
        nbestList.append(parse.getRerankerScore()).append(" ").append(parse.getParserProbability())
        .append("\n").append(parse.getPennParse()).append("\n");
      }
    }
    return nbestList.toString();
  }

  public void readParsesFromRerankerOutputFormat(String text) {
    if (text.length() == 0) {
      throw new IllegalArgumentException("Empty text");
    }

    List<Parse> newParses = new LinkedList<Parse>();
    String[] lines = text.split("\n");

    String header = lines[0]; // [num parses] [whitespace] [sentence ID (ignored currently)]
    String[] headerPieces = header.split("\\s+", 2);
    int numParses = Integer.parseInt(headerPieces[0]);

    for (int lineNumber = 1; lineNumber < lines.length; lineNumber += 2) {
      String scoreLine = lines[lineNumber];
      String[] scorePieces = scoreLine.split("\\s+", 2);

      Double rerankerScore = null;
      Double parserProb = null;

      if (scorePieces[0].equals("fail")) {
        failureDescription = scorePieces[1];
      } else {
        try {
          rerankerScore = Double.parseDouble(scorePieces[0]);
          parserProb = Double.parseDouble(scorePieces[1]);
        } catch (NumberFormatException nfe) {
          System.out.println("readParsesFromRerankerOutputFormat: " + text);
          throw new IllegalArgumentException(nfe);
        }
      }

      String pennParseLine = lines[lineNumber + 1];
      Parse currentParse = new Parse(pennParseLine, parserProb, rerankerScore);
      newParses.add(currentParse);
    }

    if (numParses != newParses.size()) {
      throw new IllegalArgumentException("Number of parses declared (" + numParses
              + ") doesn't match actual number given (" + newParses.size() + ")");
    }

    // if all went well, replace our parses with the new ones
    parses = newParses;
  }

  /**
   * Convenience method to get the top parse (first parse in the n-best list) in Penn Treebank
   * format as a String. Returns null if there are no parses available.
   */
  public String getTopPennParse() {
    if (parses == null || parses.size() == 0) {
      return null;
    } else {
      Parse parse = parses.get(0);
      return parse.getPennParse();
    }
  }

  /**
   * Returns the best parse according to the reranker. Ties are broken by the ordering of the parses
   * in the n-best list.
   */
  public Parse getBestParseFromReranker() {
    if (getNumParses() == 0) {
      return null;
    } else {
      Parse bestRerankerParse = parses.get(0);
      for (Parse parse : parses) {
        if (parse.rerankerScore > bestRerankerParse.rerankerScore) {
          bestRerankerParse = parse;
        }
      }

      return bestRerankerParse;
    }
  }

  /**
   * Returns the best parse according to the parser.
   */
  public Parse getBestParseFromParser() {
    if (getNumParses() == 0) {
      return null;
    } else {
      Parse bestParserParse = parses.get(0);
      for (Parse parse : parses) {
        // in the rare case of a tie, we'll stick with the first one we see
        if (parse.parserProbability > bestParserParse.parserProbability) {
          bestParserParse = parse;
        }
      }

      return bestParserParse;
    }
  }

  public int getNumParses() {
    if (parses == null) {
      return 0;
    } else {
      return parses.size();
    }
  }

  /**
   * Sorts the parses in-place by reranker score. Uses the parser rank to break ties. This method
   * assigns reranker ranks to Parse objects (which would otherwise be null).
   */
  public void sortByRerankerScores() {
    Collections.sort(parses, new Comparator<Parse>() {
      public int compare(Parse o1, Parse o2) {
        return -1 * new CompareToBuilder().append(o1.getRerankerScore(), o2.getRerankerScore())
                .append(o1.getParserRank(), o2.getParserRank())
                .toComparison();
      }
    });

    int rerankerRank = 0;
    for (Parse parse : parses) {
      parse.setRerankerRank(rerankerRank);
      rerankerRank++;
    }
  }

  /**
   * Sorts the parses in-place by parser probabilities. Uses the reranker score to break ties.
   */
  public void sortByParserProbabilities() {
    Collections.sort(parses, new Comparator<Parse>() {
      public int compare(Parse o1, Parse o2) {
        return -1 * new CompareToBuilder().append(o1.getParserProbability(), o2.getParserProbability())
                .append(o1.getRerankerScore(), o2.getRerankerScore())
                .toComparison();
      }
    });
  }

  /**
   * Returns whether the text is represented as a String (untokenized) or an String[] (pre-tokenized).
   */
  public boolean alreadyTokenized() {
    return tokens != null;
  }

  /**
   * Escapes the input text and tokenizes it with the Charniak tokenizer.
   */
  public String[] tokenize(ParserOptions options) {
    if (tokens == null) {
      SentRep sentRep = makeSentRep(options);
      int numTokens = sentRep.length();
      tokens = new String[numTokens];
      for (int index = 0; index < numTokens; index++) {
        tokens[index] = sentRep.getWord(index).lexeme();
      }
    }

    return tokens;
  }

  protected SentRep makeSentRep(ParserOptions options) {
    if (sentRep == null) {
      if (tokens != null) {
        StringList stringList = new StringList();
        for (String token : tokens) {
          token = Common.ptbEscape(token);
          token = BllipParser.escapeText(token);
          stringList.add(token);
        }
        sentRep = new SentRep(stringList);
      } else if (text != null) {
        String escapedText = BllipParser.escapeText(text);
        sentRep = SWIGParser.tokenize("<s> " + escapedText + " </s>", options.maxSentenceLength);
      } else {
        throw new RuntimeException("Parser query is missing both token and text representations.");
      }
    }

    return sentRep;
  }

  protected ExtPos makeExtPos() {
    if (extPos == null) {
      if (tagConstraints == null) {
        return null;
      }

      extPos = new ExtPos();
      for (int index = 0; index < tokens.length; index++) {
        List<String> possibleTags = tagConstraints.get(index);
        StringVector tagVector = new StringVector();
        if (possibleTags != null) {
          for (String possibleTag : possibleTags) {
            tagVector.add(possibleTag);
          }
        }
        extPos.addTagConstraints(tagVector);
      }
    }

    return extPos;
  }

  protected void makeAndSetFailureTree(String failureDescription, ParserOptions options) {
    SentRep sentRep = makeSentRep(options);
    InputTree failTree = sentRep.makeFailureTree("FRAG");
    Parse failParse = new Parse(failTree.toString());
    parses = new ArrayList<Parse>();
    parses.add(failParse);
    this.failureDescription = failureDescription;
  }

  //
  // Getters and setters follow
  //

  public String getText() {
    if (text == null) {
      if (tokens == null) {
        throw new RuntimeException("Somehow lost both representations of the text to parse.");
      } else {
        throw new RuntimeException("Don't know the original text for this since it was constructed from tokens.");
      }
    } else {
      return text;
    }
  }

  public void setText(String text) {
    if (tokens != null && text != null) {
      throw new RuntimeException("Can't have both text and tokens set in a ParserQuery.");
    }
    this.text = text;

    clearCachedSWIGObjects();
  }

  public String[] getTokens() {
    if (sentRep != null) {
      int numTokens = sentRep.length();
      tokens = new String[numTokens];
      for (int index = 0; index < numTokens; index++) {
        tokens[index] = sentRep.getWord(index).lexeme();
      }
    }

    if (tokens == null) {
      throw new RuntimeException("Tokens unavailable for this query (call tokenize() method first).");
    }

    return tokens;
  }

  public void setTokens(String[] tokens) {
    if (text != null && tokens != null) {
      throw new RuntimeException("Can't have both text and tokens set in a ParserQuery.");
    }
    this.tokens = tokens;

    clearCachedSWIGObjects();
  }

  public List<Parse> getParses() {
    return parses;
  }

  public void setParses(List<Parse> parses) {
    this.parses = parses;
  }

  public String getFailureDescription() {
    return failureDescription;
  }

  public void setFailureDescription(String failureDescription) {
    this.failureDescription = failureDescription;
  }

  public Map<Integer, List<String>> getTagConstraints() {
    return tagConstraints;
  }

  public void setTagConstraints(Map<Integer, List<String>> tagConstraints) {
    if (!alreadyTokenized()) {
      throw new RuntimeException("Can't use tag constraints unless input is already tokenized.");
    }
    this.tagConstraints = tagConstraints;

    clearCachedSWIGObjects();
  }

  private void clearCachedSWIGObjects() {
    sentRep = null;
    extPos = null;
  }
}
