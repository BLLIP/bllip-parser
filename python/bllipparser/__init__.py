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

"""
Python frontend to the BLLIP natural language parser.

The BLLIP parser (also known as the Charniak-Johnson parser or
Brown Reranking Parser) is described in the paper `Charniak
and Johnson (Association of Computational Linguistics, 2005)
<http://aclweb.org/anthology/P/P05/P05-1022.pdf>`_. This package
provides the BLLIP parser runtime along with a Python interface. Note
that it does not come with any parsing models but includes a model
downloader. The primary maintenance for the parser takes place at
`GitHub <http://github.com/BLLIP/bllip-parser>`_.

We request acknowledgement in any publications that make use of this
software and any code derived from this software. Please report the
release date of the software that you are using, as this will enable
others to compare their results to yours.

Quickstart
----------

Install ``bllipparser`` with `pip
<https://pip.pypa.io/en/stable/installing.html#install-pip>`_::

    shell% pip install --user bllipparser

or (if you have ``sudo`` access)::

    shell% sudo pip install bllipparser

To fetch a parsing model and start parsing::

    >>> from bllipparser import RerankingParser
    >>> rrp = RerankingParser.fetch_and_load('WSJ-PTB3', verbose=True)
    [downloads, installs, and loads the model]
    >>> rrp.simple_parse("It's that easy.")
    "(S1 (S (NP (PRP It)) (VP (VBZ 's) (ADJP (RB that) (JJ easy))) (. .)))"

The first time this is called, this will download and install a parsing
model trained from Wall Street Journal in ``~/.local/share/bllipparser``
(it will only be loaded on subsequent calls).

For a list of installable parsing models, run::

    shell% python -mbllipparser.ModelFetcher -l

See `BLLIP Parser models
<https://github.com/BLLIP/bllip-parser/blob/master/MODELS.rst>`_ for
information about picking the best parsing model for your text.

Basic usage
-----------

The main class in ``bllipparser`` is the ``RerankingParser`` parser class
which provides an interface to the first stage parser and the second stage
reranker. The easiest way to construct a ``RerankingParser`` object is
with the ``fetch_and_load`` (see above) or ``from_unified_model_dir``
class methods. A unified model is a directory that contains two
subdirectories: ``parser/`` and ``reranker/``, each with the respective
model files::

    >>> from bllipparser import RerankingParser
    >>> rrp = RerankingParser.from_unified_model_dir('/path/to/model/')

If you only want the most likely parse of a sentence in Penn Treebank
format, use the ``simple_parse()`` method::

    >>> rrp.simple_parse('This is simple.')
    '(S1 (S (NP (DT This)) (VP (VBZ is) (ADJP (JJ simple))) (. .)))'

If you want more information about the parse, you'll want to use the
``parse()`` method which returns an ``NBestList`` object. The parser
produces an *n-best list* of the *n* most likely parses of the sentence
(default: *n=50*). Typically you only want the top parse, but the others
are available as well::

    >>> nbest_list = rrp.parse('This is a sentence.')

To get information about the top parse (note that the ``ptb_parse``
property is a ``Tree`` object, described in more detail later)::

    >>> print(repr(nbest_list[0]))
    ScoredParse('(S1 (S (NP (DT This)) (VP (VBZ is) (NP (DT a) (NN sentence))) (. .)))', parser_score=-29.620656470412328, reranker_score=-7.13760513405013)
    >>> print(nbest_list[0].ptb_parse)
    (S1 (S (NP (DT This)) (VP (VBZ is) (NP (DT a) (NN sentence))) (. .)))
    >>> print(nbest_list[0].parser_score)
    -29.6206564704
    >>> print(nbest_list[0].reranker_score)
    -7.13760513405
    >>> print(len(nbest_list))
    50

You can perform syntactic fusion with the ``fuse()`` method. This
combines the parses in the n-best list into a single ``Tree`` (which
may be a parse already present in the n-best list or a novel one)::

    >>> print(nbest_list.fuse())
    (S1 (S (NP (DT This)) (VP (VBZ is) (NP (DT a) (NN sentence))) (. .)))

If you have the `PyStanfordDependencies
<https://pypi.python.org/pypi/PyStanfordDependencies/>`_ package,
you can parse straight to `Stanford Dependencies
<http://nlp.stanford.edu/software/stanford-dependencies.shtml>`_::

    >>> tokens = nbest_list[0].ptb_parse.sd_tokens()
    >>> for token in tokens:
    ...     print(token)
    ...
    Token(index=1, form=u'This', cpos=u'DT', pos=u'DT', head=4, deprel=u'nsubj')
    Token(index=2, form=u'is', cpos=u'VBZ', pos=u'VBZ', head=4, deprel=u'cop')
    Token(index=3, form=u'a', cpos=u'DT', pos=u'DT', head=4, deprel=u'det')
    Token(index=4, form=u'sentence', cpos=u'NN', pos=u'NN', head=0, deprel=u'root')
    Token(index=5, form=u'.', cpos=u'.', pos=u'.', head=4, deprel=u'punct')

This will attempt to use a default converter but see docs for how to
customize dependency conversion (or if you run into Java version issues).

If you have an existing tokenizer, tokenization can also be specified
by passing a list of strings::

    >>> nbest_list = rrp.parse(['This', 'is', 'a', 'pretokenized', 'sentence', '.'])

If you'd like to disable the reranker (lowers accuracy, so not normally
done), set ``rerank=False``::

    >>> nbest_list = rrp.parse('Parser only!', rerank=False)

You can also parse text with existing POS tags (these act as soft
constraints). In this example, token 0 ('Time') should have tag VB and
token 1 ('flies') should have tag NNS::

    >>> rrp.parse_tagged(['Time', 'flies'], possible_tags={0 : 'VB', 1 : 'NNS'})[0]
    ScoredParse('(S1 (NP (VB Time) (NNS flies)))', parser_score=-54.05083561918019, reranker_score=-15.079632500107973)

You don't need to specify a tag for all words: Here, token 0 ('Time') should
have tag VB and token 1 ('flies') is unconstrained::

    >>> rrp.parse_tagged(['Time', 'flies'], possible_tags={0 : 'VB'})[0]
    ScoredParse('(S1 (S (VP (VB Time) (NP (VBZ flies)))))', parser_score=-54.3497715 5750189, reranker_score=-16.681734375725263)

You can specify multiple tags for each token. When you do this, the
tags for a token will be used in decreasing priority. token 0 ('Time')
should have tag VB, JJ, or NN and token 1 ('flies') is unconstrained::

    >>> rrp.parse_tagged(['Time', 'flies'], possible_tags={0 : ['VB', 'JJ', 'NN']})[0]
    ScoredParse('(S1 (NP (NN Time) (VBZ flies)))', parser_score=-42.9961920777843, reranker_score=-12.57069545767032)

If you have labeled span constraints, you can require that all parses follow them with ``parse_constrained``. The following requires that the parse contain
a VP covering ``left`` to ``Falklands``::

    >>> rrp.parse_constrained('British left waffles on Falklands .'.split(),
    ...                       constraints={(1, 5) : ['VP']})[0]
    ScoredParse('(S1 (S (NP (NNPS British)) (VP (VBD left) (NP (NNS waffles)) (PP (IN on) (NP (NNP Falklands)))) (. .)))', parser_score=-93.73622897543436, reranker_score=-25.60347808581542)

To force ``British left`` to be a noun phrase::

    >> rrp.parse_constrained('British left waffles on Falklands .'.split(),
    ...                      constraints={(0, 2): ['NP']})[0]
    ScoredParse('(S1 (S (NP (JJ British) (NN left)) (VP (VBZ waffles) (PP (IN on) (NP (NNP Falklands)))) (. .)))', parser_score=-89.59447837562135, reranker_score=-25.480236524298025)

There are many parser options which can be adjusted (though the defaults
should work well for most cases) with ``set_parser_options``. This
will change the size of the n-best list and pick the defaults for all
other options. It returns a dictionary of the current options::

    >>> rrp.set_parser_options(nbest=10)
    {'language': 'En', 'case_insensitive': False, 'debug': 0, 'small_corpus': True, 'overparsing': 21, 'smooth_pos': 0, 'nbest': 10}
    >>> nbest_list = rrp.parse('The list is smaller now.', rerank=False)
    >>> len(nbest_list)
    10

The parser can also be used as a tagger::

    >>> rrp.tag("Time flies while you're having fun.")
    [('Time', 'NNP'), ('flies', 'VBZ'), ('while', 'IN'), ('you', 'PRP'), ("'re", 'VBP'), ('having', 'VBG'), ('fun', 'NN'), ('.', '.')]

Use this if all you want is a Penn Treebank-style tokenizer::

    >>> from bllipparser import tokenize
    >>> tokenize("Tokenize this sentence, please.")
    ['Tokenize', 'this', 'sentence', ',', 'please', '.']

Parsing shell
-------------

BLLIP Parser includes an interactive shell for visualizing parses::

    shell% python -mbllipparser model

(for Python 2.6, you'll need to run: ``python -mbllipparser.ParsingShell model``)

Model can be a unified parsing model or first-stage parsing model on
disk or the name of a model known by ModelFetcher, in which case it will
be downloaded and installed if it hasn't been already. If no model is
specified, it will list installable parsing models.

Once in the shell, type a sentence to have the parser parse it::

    bllip> I saw the astronomer with the telescope.
    Tokens: I saw the astronomer with the telescope .

    Parser's parse:
    (S1 (S (NP (PRP I))
         (VP (VBD saw)
          (NP (NP (DT the) (NN astronomer))
           (PP (IN with) (NP (DT the) (NN telescope)))))
         (. .)))

    Reranker's parse: (parser index 2)
    (S1 (S (NP (PRP I))
         (VP (VBD saw)
          (NP (DT the) (NN astronomer))
          (PP (IN with) (NP (DT the) (NN telescope))))
         (. .)))

If you have ``nltk`` installed, you can use its tree visualization to
see the output::

    bllip> visual Show me this parse.
    Tokens: Show me this parse .

    [graphical display of the parse appears]

If you have ``PyStanfordDependencies`` installed, you can parse straight
to Stanford Dependencies::

    bllip> sdparse Now with Stanford Dependencies integration!
    Tokens: Now with Stanford Dependencies integration !

    Parser and reranker:
     Now [root]
      +-- with [prep]
      |  +-- integration [pobj]
      |     +-- Stanford [nn]
      |     +-- Dependencies [nn]
      +-- ! [punct]

The ``asciitree`` package is required to visualize Stanford Dependencies
as a tree. If it is not available, the dependencies will be shown in
CoNLL-X format.

There is more detailed help inside the shell under the ``help`` command.

The Tree class
--------------

The parser provides a simple Tree class which provides information about
Penn Treebank-style trees::

    >>> tree = bllipparser.Tree('(S1 (S (NP (DT This)) (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree))) (. .)))')
    >>> print(tree)
    (S1 (S (NP (DT This)) (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree))) (. .)))

``pretty_string()`` provides a line-wrapped stringification::

    >>> print(tree.pretty_string())
    (S1 (S (NP (DT This))
         (VP (VBZ is)
          (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree)))
         (. .)))

You can obtain the tokens and tags of the tree::

    >>> print(tree.tokens())
    ('This', 'is', 'a', 'fairly', 'simple', 'parse', 'tree', '.')
    >>> print(tree.tags())
    ('DT', 'VBZ', 'DT', 'RB', 'JJ', 'NN', 'NN', '.')
    >>> print(tree.tokens_and_tags())
    [('This', 'DT'), ('is', 'VBZ'), ('a', 'DT'), ('fairly', 'RB'), ('simple', 'JJ'), ('parse', 'NN'), ('tree', 'NN'), ('.', '.')]

Or get information about the labeled spans in the tree::

    >>> print(tree.span())
    (0, 8)
    >>> print(tree.label)
    S1

You can navigate within the trees and more::

    >>> tree.subtrees()
    [Tree('(S (NP (DT This)) (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree))) (. .))')]
    >>> tree[0] # first subtree
    Tree('(S (NP (DT This)) (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree))) (. .))')
    >>> tree[0].label
    'S'
    >>> tree[0][0] # first subtree of first subtree
    Tree('(NP (DT This))')
    >>> tree[0][0].label
    'NP'
    >>> tree[0][0].span() # [start, end) indices for the span
    (0, 1)
    >>> tree[0][0].tags() # tags within this span
    ('DT',)
    >>> tree[0][0].tokens() # tuple of all tokens in this span
    ('This',)
    >>> tree[0][0][0]
    Tree('(DT This)')
    >>> tree[0][0][0].token
    'This'
    >>> tree[0][0][0].label
    'DT'
    >>> tree[0][0][0].is_preterminal()
    True
    >>> len(tree[0]) # number of subtrees
    3
    >>> for subtree in tree[0]: # iteration works
    ...    print(subtree)
    ... 
    (NP (DT This))
    (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree)))
    (. .)
    >>> for subtree in tree.all_subtrees(): # all subtrees (recursive)
    ...     print('%s %s' % (subtree.is_preterminal(), subtree))
    ...
    False (S1 (S (NP (DT This)) (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree))) (. .)))
    False (S (NP (DT This)) (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree))) (. .))
    False (NP (DT This))
    True (DT This)
    False (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree)))
    True (VBZ is)
    False (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree))
    True (DT a)
    False (ADJP (RB fairly) (JJ simple))
    True (RB fairly)
    True (JJ simple)
    True (NN parse)
    True (NN tree)
    True (. .)

More examples and advanced features
-----------------------------------

See the documentation and the `examples
<https://github.com/BLLIP/bllip-parser/tree/master/python/examples>`_
directory in the repository.

References
----------

Parser and reranker:

* Eugene Charniak and Mark Johnson. "`Coarse-to-fine n-best parsing and
  MaxEnt discriminative reranking
  <http://aclweb.org/anthology/P/P05/P05-1022.pdf>`_."  Proceedings of
  the 43rd Annual Meeting on Association for Computational Linguistics.
  `Association for Computational Linguistics, 2005
  <http://bllip.cs.brown.edu/publications/index_bib.shtml#charniak-johnson:2005:ACL>`_.

* Eugene Charniak. "`A maximum-entropy-inspired parser
  <http://aclweb.org/anthology//A/A00/A00-2018.pdf>`_." Proceedings of
  the 1st North American chapter of the Association for Computational
  Linguistics conference. `Association for Computational Linguistics, 2000
  <http://bllip.cs.brown.edu/publications/index_bib.shtml#Charniak:2000:NAACL>`_.

Self-trained parsing models:

* David McClosky, Eugene Charniak, and Mark Johnson.
  "`Effective Self-Training for Parsing
  <http://www.aclweb.org/anthology/N/N06/N06-1020.pdf>`_."
  Proceedings of the Conference on Human Language Technology
  and North American chapter of the `Association for
  Computational Linguistics (HLT-NAACL 2006), 2006
  <http://www.aclweb.org/anthology/N/N06/N06-1020.bib>`_.

Syntactic fusion:

* Do Kook Choe, David McClosky, and Eugene Charniak.
  "`Syntactic Parse Fusion
  <http://nlp.stanford.edu/~mcclosky/papers/choe-emnlp-2015.pdf>`_."
  Proceedings of the Conference on `Empirical Methods in Natural Language
  Processing (EMNLP 2015), 2015
  <http://nlp.stanford.edu/~mcclosky/papers/choe-emnlp-2015.bib>`_.

Release highlights
------------------
- 2016.9.11: Python 3.5 support
- 2015.12.3: Python 3 support, ``Tree`` visualization methods, better test coverage, bugfixes
- 2015.08.18: New APIs for easier use, integrated ``ModelFetcher`` with ``ParsingShell``, automatically organize models
- 2015.08.15: Add syntactic fusion, ``sigeval``, and new self-trained model
- 2015.07.23: Fix build error, other build system improvements
- 2015.07.08: Constrained parsing, reranker can now be built with optimization (30% faster), other API additions
- 2015.01.11: Improved ``PyStanfordDependencies`` support, memory leak fixed, API additions, bugfixes
- 2014.08.29: Add ``Tree`` class, ``RerankerFeatureCorpus`` module, other API updates
- 2014.02.09: Add ``ModelFetcher``, ``RerankingParser`` improvements
- 2013.10.16: ``distutils`` support, initial PyPI release
"""

from .RerankingParser import RerankingParser, Tree, Sentence, tokenize

__authors__ = 'Eugene Charniak, Mark Johnson, David McClosky, many others'
__license__ = 'Apache 2.0'
__version__ = '2016.9.11'
__maintainer__ = 'David McClosky'
__email__ = 'notsoweird+pybllipparser@gmail.com'
