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

import math
import importlib

# now part of waterworks.Tools
class DeprecatedGetter:
    """Used when a getter method has been converted to a property. All
    attributes will be dispatched to the property's value and a warning
    will be raised if this is called. This doesn't work if the property
    being deprecated has its own __call__ method, since that will be
    unreachable as a DeprecatedGetter."""
    def __init__(self, name, value):
        """name is the attribute name of the property. value is its
        value."""
        self.__name = name
        self.__value = value
    def __getattr__(self, attr):
        """All attributes except __call__ are dispatched to the property's
        value."""
        return getattr(self.__value, attr)
    def __call__(self, *args, **kwargs):
        """Shouldn't be called except by deprecated code. Issues a warning
        about the deprecation then returns the value so that deprecated
        code will continue to work."""
        from warnings import warn
        warn("%r is no longer a method. It's now a property." % self.__name,
             DeprecationWarning, stacklevel=2)
        return self.__value

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
