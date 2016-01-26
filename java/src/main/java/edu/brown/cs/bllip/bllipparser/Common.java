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

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

public class Common {
  private static Map<String, String> unescapingMapping = new HashMap<String, String>();
  static {
    unescapingMapping.put("-LRB-", "(");
    unescapingMapping.put("-RRB-", ")");
    unescapingMapping.put("-LCB-", "{");
    unescapingMapping.put("-RCB-", "}");
    unescapingMapping.put("-LSB-", "[");
    unescapingMapping.put("-RSB-", "]");
  }

  public static String ptbUnescape(String text) {
    for (Entry<String, String> entry : unescapingMapping.entrySet()) {
      text = text.replace(entry.getKey(), entry.getValue());
    }
    return text;
  }

  public static String ptbEscape(String text) {
    for (Entry<String, String> entry : unescapingMapping.entrySet()) {
      text = text.replace(entry.getValue(), entry.getKey());
    }
    return text;
  }

  public static void checkReadableOrThrowError(String description,
      String pathAsString, boolean expectedDirectory) {
    Path path = Paths.get(pathAsString);
    if (expectedDirectory) {
      if (!Files.isDirectory(path)) {
        throw new RuntimeException(pathAsString + " (" + description
            + ") is not a directory");
      }
    } else {
      if (!Files.isRegularFile(path)) {
        throw new RuntimeException(pathAsString + " (" + description
            + ") is not a regular file");
      }
    }

    if (!Files.isReadable(path)) {
      throw new RuntimeException(pathAsString + " (" + description
          + ") exists but is not readable");
    }
  }
}
