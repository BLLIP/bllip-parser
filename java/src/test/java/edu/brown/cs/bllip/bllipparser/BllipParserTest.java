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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.commons.lang3.ArrayUtils;
import org.junit.BeforeClass;
import org.junit.Test;

import edu.brown.cs.bllip.bllipparser.Common;

public class BllipParserTest {

  private static BllipParser parser;
  private static ParserOptions options;

  /*
   * These are some basic tests but should cover the most common code paths.
   * This currently doesn't run out of the box since the model path must be specified.
   * NOTE: The BllipParser API has shifted, so these tests are currently disabled.
   *
   * TODO:
   * - testing parser alone -- we can't actually do this currently based on the
   *   BllipParser architecture which only lets us load a single model per JVM process.
   * - testing external POS tag constraints
   * - should test on way more sentences (maybe outside of this unit test)
   */

  @BeforeClass
  public static void setUp() throws Exception {
    String base = "/path/to/wsj-no-AUX"; // TODO will need to figure out how to generalize this path and set it from properties

    String parserModel = base + "/parser";
    String rerankerFeatures = base + "/reranker/features.gz";
    String rerankerWeights = base + "/reranker/weights.gz";

    options = new ParserOptions(parserModel, rerankerFeatures, rerankerWeights);
    options.numParses = 10;

    parser = new BllipParser(options);
  }

  /**
   * Basic tokenization tests.
   */
  @Test
  public void testTokenize() {
    assertTokenizes("It's not an especially long test sentence.",
            "It 's not an especially long test sentence .".split(" "));
    assertTokenizes("These aren't the droids you're looking for.",
            "These are n't the droids you 're looking for .".split(" "));
    assertTokenizes("What about question marks?",
            "What about question marks ?".split(" "));
    assertTokenizes("Are parentheticals (like this) okay?",
            "Are parentheticals ( like this ) okay ?".split(" "));
    assertTokenizes("Lots of brackets: ( ) [ ] { }",
            "Lots of brackets : ( ) [ ] { }".split(" "));
    assertTokenizes("Lots of brackets but no spaces: ()[]{}",
            "Lots of brackets but no spaces : ( ) [ ] { }".split(" "));
    assertTokenizes("Punctuation after (parens).",
            "Punctuation after ( parens ) .".split(" "));
  }

  /**
   * Test escaping, tokenization, and unescaping of some messier input (</S> tag)
   */
  @Test
  public void testEscapingAndTokenization1() {
    assertTokenizes("This sentence has a </s> which needs to be escaped.",
            "This sentence has a </s> which needs to be escaped .".split(" "));
    assertTokenizes("This sentence has a </S> which also needs to be escaped.",
            "This sentence has a </S> which also needs to be escaped .".split(" "));
    assertTokenizes("This sentence has a</s>tag which needs to be escaped but no spaces.",
            "This sentence has a </s> tag which needs to be escaped but no spaces .".split(" "));
  }

  /**
   * Test escaping, tokenization, and unescaping of some messier input ("^^")
   */
  @Test
  public void testEscapingAndTokenization2() {
    assertTokenizes("This sentence has the infamous double caret: ^^ (that was it).",
            "This sentence has the infamous double caret : ^^ ( that was it ) .".split(" "));
  }

  public void assertTokenizes(String input, String[] expectedTokens) {
    ParserQuery query = new ParserQuery(input);
    String[] actualTokens = query.tokenize(options);
    assertNotNull(actualTokens);
    for (int i = 0; i < actualTokens.length; i++) {
      String token = actualTokens[i];
      actualTokens[i] = BllipParser.unescapeText(token);
      actualTokens[i] = Common.ptbUnescape(actualTokens[i]);
    }
    if (!Arrays.equals(actualTokens, expectedTokens)) {
      System.out.println("Tokens (input):    " + input);
      System.out.println("Tokens (actual):   " + ArrayUtils.toString(actualTokens));
      System.out.println("Tokens (expected): " + ArrayUtils.toString(expectedTokens));
      System.out.println();
    }
    assertArrayEquals(expectedTokens, actualTokens);
  }

  /**
   * Test basic parsing mechanics with a simple sentence.
   */
  @Test
  public void testSimpleParse() {
    ParserQuery query = new ParserQuery("A short sentence.");
    parser.parse(query);
    assertEquals(query.getNumParses(), 10);
    assertNull(query.getFailureDescription());
    List<Parse> parses = query.getParses();

    Parse topParse = parses.get(0);
    Parse expectedTopParse = new Parse("(S1 (NP (DT A) (JJ short) (NN sentence) (. .)))", -38.709401951732616, 2.606311166, 0, 0);
    assertSame(topParse, query.getBestParseFromParser());
    assertSame(topParse, query.getBestParseFromReranker());
    assertEquals(expectedTopParse.getPennParse(), query.getTopPennParse());
    assertSameParse(expectedTopParse, topParse);

    Parse secondParse = parses.get(1);
    Parse expectedSecondParse = new Parse("(S1 (NP (NP (DT A) (JJ short) (NN sentence)) (. .)))", -43.96170810550869, 0.0593699607798413, 2, 1);
    assertSameParse(expectedSecondParse, secondParse);
  }

  /**
   * Test basic parsing mechanics with a complex sentence.
   */
  @Test
  public void testComplexParse() {
    ParserQuery query = new ParserQuery("Her history of chest pain was typical of crescendo angina pectoris on exertion, without symptoms that were prolonged or that occurred at rest.");
    parser.parse(query);
    assertEquals(query.getNumParses(), 10);
    assertNull(query.getFailureDescription());
    List<Parse> parses = query.getParses();

    Parse parse0 = new Parse("(S1 (S (NP (NP (PRP$ Her) (NN history)) (PP (IN of) (NP (NN chest) (NN pain)))) (VP (VBD was) (ADJP (JJ typical) (PP (IN of) (NP (NN crescendo) (NN angina) (NNS pectoris)))) (PP (IN on) (NP (NN exertion))) (, ,) (PP (IN without) (NP (NP (NNS symptoms)) (SBAR (SBAR (WHNP (WDT that)) (S (VP (VBD were) (VP (VBN prolonged))))) (CC or) (SBAR (WHNP (WDT that)) (S (VP (VBD occurred) (PP (IN at) (NP (NN rest)))))))))) (. .)))",
            -301.566011900728, 2.1626921912852115, 2, 0);
    Parse parse1 = new Parse("(S1 (S (NP (NP (PRP$ Her) (NN history)) (PP (IN of) (NP (NN chest) (NN pain)))) (VP (VBD was) (ADJP (JJ typical) (PP (IN of) (NP (NN crescendo) (NN angina) (NNS pectoris)))) (PP (IN on) (NP (NN exertion))) (, ,) (PP (IN without) (NP (NP (NNS symptoms)) (SBAR (SBAR (WHNP (WDT that)) (S (VP (VBD were) (ADJP (JJ prolonged))))) (CC or) (SBAR (WHNP (WDT that)) (S (VP (VBD occurred) (PP (IN at) (NP (NN rest)))))))))) (. .)))",
            -301.0627045581689, 1.6178072195000002, 0, null);
    Parse parse2 = new Parse("(S1 (S (NP (NP (PRP$ Her) (NN history)) (PP (IN of) (NP (NN chest) (NN pain)))) (VP (VBD was) (ADJP (JJ typical) (PP (IN of) (NP (NN crescendo) (NN angina) (NN pectoris)))) (PP (IN on) (NP (NN exertion))) (, ,) (PP (IN without) (NP (NP (NNS symptoms)) (SBAR (SBAR (WHNP (WDT that)) (S (VP (VBD were) (ADJP (JJ prolonged))))) (CC or) (SBAR (WHNP (WDT that)) (S (VP (VBD occurred) (PP (IN at) (NP (NN rest)))))))))) (. .)))",
            -301.39896735188256, 0.8225177143672042, 1, null);

    assertSameParse(parse0, parses.get(0));
    assertSameParse(parse1, parses.get(1));
    assertSameParse(parse2, parses.get(2));

    query.sortByRerankerScores();

    parses = query.getParses();
    parse0.setRerankerRank(1);
    parse1.setRerankerRank(5);
    parse2.setRerankerRank(0);
    assertSameParse(parse0, parses.get(1));
    assertSameParse(parse1, parses.get(5));
    assertSameParse(parse2, parses.get(0));
  }

  /**
   * Test parsing with POS tag constraints.
   */
  @Test
  public void testParseWithPOSConstraints() {
    ParserQuery query = new ParserQuery("British left waffles on Falklands .".split(" "));
    parser.parse(query);
    assertEquals(10, query.getNumParses());
    assertNull(query.getFailureDescription());

    assertSameParse(new Parse("(S1 (S (NP (JJ British) (NN left)) (VP (VBZ waffles) (PP (IN on) (NP (NNP Falklands)))) (. .)))",
            -89.59751308276466, 1.4919366154119993, 0, 0),
            query.getParses().get(0));

    Map<Integer, List<String>> tagConstraints = new HashMap<Integer, List<String>>();
    ArrayList<String> tags = new ArrayList<String>();
    tags.add("NNS");
    tagConstraints.put(2, tags);
    query.setTagConstraints(tagConstraints);
    parser.parse(query);
    assertEquals(10, query.getNumParses());
    assertSameParse(new Parse("(S1 (S (NP (NNPS British)) (VP (VBD left) (NP (NNS waffles) (PP (IN on) (NP (NNP Falklands))))) (. .)))", -93.09178742278908, 1.0114073994999997, 0, null),
            query.getParses().get(0));
    assertNull(query.getFailureDescription());

    tags.set(0, "VBZ");
    // since it doesn't expect these to change, we need to reset this to clear cached ExtPos objects
    query.setTagConstraints(tagConstraints);
    parser.parse(query);
    assertEquals(10, query.getNumParses());
    assertNull(query.getFailureDescription());

    assertSameParse(new Parse("(S1 (S (NP (JJ British) (NN left)) (VP (VBZ waffles) (PP (IN on) (NP (NNP Falklands)))) (. .)))", -89.59751308276466, 0.8652405399999998, 0, null),
            query.getParses().get(0));

    tags.set(0, "TO");
    query.setTagConstraints(tagConstraints);
    parser.parse(query);
    assertEquals(1, query.getNumParses());
    assertNotNull(query.getFailureDescription());

    // make sure we can sort an empty n-best list
    query.sortByRerankerScores();
  }

  public void assertSameParse(Parse expectedParse, Parse actualParse) {
    assertEquals("pennParse doesn't match", expectedParse.getPennParse(), actualParse.getPennParse());
    assertEquals("parserProbability doesn't match", expectedParse.getParserProbability(), actualParse.getParserProbability());
    assertEquals("rerankerScore doesn't match", expectedParse.getRerankerScore(), actualParse.getRerankerScore());
    assertEquals("parserRank doesn't match", expectedParse.getParserRank(), actualParse.getParserRank());
    assertEquals("rerankerRank doesn't match", expectedParse.getRerankerRank(), actualParse.getRerankerRank());
  }

  /**
   * Test ParserQuery getText() fails when it doesn't know the text.
   */
  @Test(expected=RuntimeException.class)
  public void testParserQueryGetTextWhenEmpty() {
    ParserQuery query = new ParserQuery("Already tokenized input .".split(" "));
    query.getText();
  }

  /**
   * Test ParserQuery getText() when it does know the text.
   */
  @Test
  public void testParserQueryGetTextWhenNonEmpty() {
    String text = "Not already tokenized input.";
    ParserQuery query = new ParserQuery(text);
    assertEquals(text, query.getText());
  }

  /**
   * Test ParserQuery setText() and setTokens() in various patterns.
   */
  @Test
  public void testParserQuerySetTextAndTokens() {
    String text = "Not already tokenized input.";
    ParserQuery query = new ParserQuery(text);
    assertEquals(text, query.getText());
    query.setText(null);
    // TODO assert getText throws an exception
    String[] tokens = new String[] { "Some", "tokens", "." };
    query.setTokens(tokens);
    assertArrayEquals(tokens, query.getTokens());
    query.setTokens(null);
    // TODO assert getTokens throws an exception
  }

  /**
   * Test constructors for ParserQuery objects.
   */
  @Test
  public void testParserQueryConstruction() {
    // build the same sentence from a String and a String[] and make sure they have the same tokens
    ParserQuery query1 = new ParserQuery("Abdominal examination revealed mild tenderness in the right upper quadrant and a fluid wave without hepatosplenomegaly.");
    ParserQuery query2 = new ParserQuery("Abdominal examination revealed mild tenderness in the right upper quadrant and a fluid wave without hepatosplenomegaly .".split(" "));

    query1.tokenize(options);
    query2.tokenize(options);

    assertNotNull(query1.getTokens());

    assertArrayEquals(query1.getTokens(), query2.getTokens());
    assertNull(query1.getTagConstraints());
    assertNull(query2.getTagConstraints());

    assertNull(query1.getParses());
    assertNull(query1.getBestParseFromParser());
    assertNull(query1.getBestParseFromReranker());
    assertNull(query1.getTopPennParse());
    assertEquals(0, query1.getNumParses());

    assertNull(query2.getParses());
    assertNull(query2.getBestParseFromParser());
    assertNull(query2.getBestParseFromReranker());
    assertNull(query2.getTopPennParse());
    assertEquals(0, query2.getNumParses());
  }
}
