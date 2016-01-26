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

/**
 * A specific parse of some text, potentially including its parser probability and reranker score.
 * You typically obtain these via a ParserQuery.
 */
public class Parse {
  protected String pennParse;
  protected Double parserProbability;
  protected Double rerankerScore;
  protected Integer parserRank;
  protected Integer rerankerRank;

  public Parse(String pennParse) {
    this(pennParse, null, null, null, null);
  }

  public Parse(String pennParse, Double parserProbability) {
    this(pennParse, parserProbability, null, null, null);
  }

  public Parse(String pennParse, Double parserProbability, Double rerankerScore) {
    this(pennParse, parserProbability, rerankerScore, null, null);
  }

  public Parse(String pennParse, Double parserProbability, Double rerankerScore, Integer parserRank, Integer rerankerRank) {
    this.pennParse = pennParse;
    this.parserProbability = parserProbability;
    this.rerankerScore = rerankerScore;
    this.parserRank = parserRank;
    this.rerankerRank = rerankerRank;
  }

  @Override
  public String toString() {
    StringBuilder builder = new StringBuilder();
    builder.append("Parse[");
    if (pennParse != null) {
      builder.append("pennParse=\"").append(pennParse).append("\", ");
    }
    if (parserProbability != null) {
      builder.append("parserProbability=").append(parserProbability).append(", ");
    }
    if (rerankerScore != null) {
      builder.append("rerankerScore=").append(rerankerScore);
    }
    builder.append("]");
    return builder.toString();
  }

  @Override
  public int hashCode() {
    final int prime = 31;
    int result = 1;
    result = prime * result + (parserProbability == null ? 0 : parserProbability.hashCode());
    result = prime * result + (parserRank == null ? 0 : parserRank.hashCode());
    result = prime * result + (pennParse == null ? 0 : pennParse.hashCode());
    result = prime * result + (rerankerRank == null ? 0 : rerankerRank.hashCode());
    result = prime * result + (rerankerScore == null ? 0 : rerankerScore.hashCode());
    return result;
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }
    if (obj == null) {
      return false;
    }
    if (getClass() != obj.getClass()) {
      return false;
    }
    Parse other = (Parse) obj;
    if (parserProbability == null) {
      if (other.parserProbability != null) {
        return false;
      }
    } else if (!parserProbability.equals(other.parserProbability)) {
      return false;
    }
    if (parserRank == null) {
      if (other.parserRank != null) {
        return false;
      }
    } else if (!parserRank.equals(other.parserRank)) {
      return false;
    }
    if (pennParse == null) {
      if (other.pennParse != null) {
        return false;
      }
    } else if (!pennParse.equals(other.pennParse)) {
      return false;
    }
    if (rerankerRank == null) {
      if (other.rerankerRank != null) {
        return false;
      }
    } else if (!rerankerRank.equals(other.rerankerRank)) {
      return false;
    }
    if (rerankerScore == null) {
      if (other.rerankerScore != null) {
        return false;
      }
    } else if (!rerankerScore.equals(other.rerankerScore)) {
      return false;
    }
    return true;
  }

  //
  // Getters and setters
  //

  public String getPennParse() {
    return pennParse;
  }

  public void setPennParse(String pennParse) {
    this.pennParse = pennParse;
  }

  public Double getParserProbability() {
    return parserProbability;
  }

  public void setParserProbability(Double parserProbability) {
    this.parserProbability = parserProbability;
  }

  public Double getRerankerScore() {
    return rerankerScore;
  }

  public void setRerankerScore(Double rerankerScore) {
    this.rerankerScore = rerankerScore;
  }

  public Integer getParserRank() {
    return parserRank;
  }

  public void setParserRank(Integer parserRank) {
    this.parserRank = parserRank;
  }

  /**
   * Returns the rank of this parse according to the reranker. This is only set if someone has
   * called the ParserQuery object's sortByRerankerScores() method.
   */
  public Integer getRerankerRank() {
    return rerankerRank;
  }

  public void setRerankerRank(Integer rerankerRank) {
    this.rerankerRank = rerankerRank;
  }
}
