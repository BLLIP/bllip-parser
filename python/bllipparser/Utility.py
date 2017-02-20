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

import importlib
import math
import re
from . import CharniakParser as parser

whitespace_re = re.compile(r'\s')

def import_maybe(module_name):
    "Import a module and return it if available, otherwise returns None."
    try:
        return importlib.import_module(module_name)
    except ImportError:
        return None

def get_nltk_tree_reader_maybe():
    """Attempts to find the NLTK tree reader for various versions of NLTK.
    Returns False if it fails or a function which takes a string and
    returns an NLTK tree object otherwise."""
    try:
        import nltk.tree
        import nltk.draw.tree
        return nltk.tree.Tree.parse
    except ImportError:
        return False
    except AttributeError: # handle NLTK API changes
        try:
            return nltk.tree.Tree.fromstring
        except AttributeError:
            return False

def normalize_logprobs(logprobs, exponent=1):
    """Sum probs stored as log probs in a (more) numerically stable
    fashion, see:

    blog.smola.org/post/987977550/log-probabilities-semirings-and-floating-point

    We optionally raise all log probs to an exponent."""
    biggest = max(logprobs) * exponent
    exp_diffs = [math.exp((logprob * exponent) - biggest)
                 for logprob in logprobs]
    z = sum(exp_diffs)
    return [exp_diff / z for exp_diff in exp_diffs]

def cleanup_input_token(token):
    """Cleanup and validate an input pre-tokenized token.

    This is here to ensure well-formed input and avoid downstream crashes."""
    # need to make sure that each element is a string to avoid crashing
    if token is None:
        raise ValueError("Token can't be None: %r" % token)
    token = str(token)
    if whitespace_re.search(token):
        raise ValueError("Can't have whitespace within a token: %r" % token)
    if not token:
        raise ValueError("Can't have an empty token: %r" % token)
    token = parser.ptbEscape(token)
    return token
