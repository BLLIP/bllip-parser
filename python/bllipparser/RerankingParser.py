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

"""Higher-level python frontend to the BLLIP reranking parser. Wraps the
lower-level (SWIG-generated) CharniakParser and JohnsonReranker modules
so you don't need to interact with them directly."""

from os.path import exists, join
from six import string_types
from . import CharniakParser as parser
from . import JohnsonReranker as reranker
from .Utility import normalize_logprobs

class Tree(object):
    """Represents a single parse (sub)tree in Penn Treebank format. This
    wraps the InputTree structure in the Charniak parser."""
    def __init__(self, input_tree_or_string):
        """These can be constructed from the Penn Treebank string
        representations of trees, e.g.:

            >>> Tree('(S1 (NP (NN tree)))')
            Tree('(S1 (NP (NN tree)))')

        Or from an existing InputTree (internal SWIG object). Users will
        generally want the former."""
        if not isinstance(input_tree_or_string, parser.InputTree):
            if not isinstance(input_tree_or_string, string_types):
                raise TypeError("input_tree_or_string (%r) must be "
                                "an InputTree or string." %
                                input_tree_or_string)
            input_tree_or_string = \
                parser.inputTreeFromString(input_tree_or_string)
        self._tree = input_tree_or_string
        self._sd_tokens = None
    def __getitem__(self, index):
        """Indexes into subtrees for this node. Raises an IndexError if
        there's no such subtree. Slices are supported."""
        # list is necessary since otherwise it doesn't support all the
        # slice stuff
        subtrees = list(self._tree.subTrees())
        try:
            subtree = subtrees[index]
        except IndexError:
            if self.is_preterminal():
                message = 'node is a preterminal'
            else:
                message = 'only %s children for this node' % len(self)
            raise IndexError("list index %r out of range (%s)" %
                             (index, message))
        if isinstance(index, slice):
            return [self.__class__(s) for s in subtree]
        else: # single InputTree object
            return self.__class__(subtree)
    def __iter__(self):
        """Provides an iterator over immediate subtrees in this Tree.
        Each item yielded will be a Tree object rooted at one of the
        children of this tree."""
        for tree in self._tree.subTrees():
            yield self.__class__(tree)
    def all_subtrees(self):
        """Iterates over all nodes in this tree in preorder (this node,
        followed by its first child, etc.)"""
        yield self
        for subtree in self:
            for subsubtree in subtree.all_subtrees():
                yield subsubtree
    def subtrees(self):
        """Returns a list of direct subtrees."""
        return list(iter(self))
    def __len__(self):
        """Returns the number of direct subtrees."""
        return len(self.subtrees())
    def __repr__(self):
        """Provides a representation of this tree which can be used to
        reconstruct it."""
        return '%s(%r)' % (self.__class__.__name__, str(self))
    def __str__(self):
        """Represent the tree in Penn Treebank format on one line."""
        return self._tree.toString()
    def pretty_string(self):
        """Represent the tree in Penn Treebank format with line wrapping."""
        return self._tree.toStringPrettyPrint()
    def format_asciitree(self):
        """Return a string representing this tree using asciitree
        (requires the 'asciitree' package)."""
        import asciitree
        def child_iter(tree):
            return tree.subtrees()
        def text_str(tree):
            return ' %s%s %s' % (tree.label, tree.label_suffix,
                                 tree.token or '')
        return asciitree.draw_tree(self, child_iter=child_iter,
                                   text_str=text_str)
    def as_nltk_tree(self):
        """Returns this tree as an NLTK Tree object."""
        from .Utility import get_nltk_tree_reader_maybe
        read_nltk_tree = get_nltk_tree_reader_maybe()
        if not read_nltk_tree:
            raise ImportError("Unable to import nltk tree reading.")
        nltk_tree = read_nltk_tree(str(self))
        return nltk_tree
    def tokens(self):
        """Return a tuple of the word tokens in this tree."""
        return tuple(self._tree.getWords())
    def tags(self):
        """Return a tuple of the part-of-speech tags in this tree."""
        return tuple(self._tree.getTags())
    def tokens_and_tags(self):
        """Return a list of (word, tag) pairs."""
        return list(zip(self.tokens(), self.tags()))
    def span(self):
        """Returns indices of the span for this tree: (start, end)"""
        return (int(self._tree.start()), int(self._tree.finish()))
    def is_preterminal(self):
        """Returns True iff this node is a preterminal (that is, its
        label is a part of speech tag, it has a non-empty token, and it
        has no child nodes)."""
        return len(self) == 0
    def evaluate(self, gold_tree):
        """Score this tree against a gold tree and return a dictionary with
        PARSEVAL information. Keys:
            gold, test, matched - integers for numbers of brackets
            precision, recall, fscore - floats between 0 and 1

        Note that you must have a parser model loaded in order to
        evaluate parses (otherwise you'll get a ValueError). This is
        because the parser models include information about which tags
        are punctuation."""
        if not RerankingParser._parser_terms_loaded:
            raise ValueError("You need to have loaded a parser model in "
                             "order to evaluate.")
        scorer = parser.ScoreTree()
        stats = scorer.score(self._tree, gold_tree._tree)
        gold = stats.numInGold
        test = stats.numInGuessed
        matched = stats.numCorrect
        return dict(gold=gold, test=test, matched=matched,
                    fscore=stats.fMeasure(), precision=stats.precision(),
                    recall=stats.recall())
    def log_prob(self):
        """Asks the current first-stage parsing model to score an existing
        tree. Returns parser model's log probability. Python equivalent of the
        evalTree command line tool.

        Note that you must have a parser model loaded in order to call
        this parses (otherwise you'll get a ValueError)."""
        if not RerankingParser._parser_model_loaded:
            raise ValueError("You need to have loaded a parser model in "
                             "order to get the log probability.")
        return parser.treeLogProb(self._tree)
    def head(self):
        """Returns the syntactic head of this tree. This will be one of
        the children of this tree (unless this tree is a leaf in which
        case it is its own head). Requires a parsing model to be loaded
        (other you'll get a ValueError) since those include head finding
        information."""
        if not RerankingParser._parser_heads_loaded:
            raise ValueError("You need to have loaded a parser model in "
                             "order to get the log probability.")
        return self.__class__(self._tree.headTree())
    def dependencies(self):
        """Yields pairs of (governor, dependent) subtrees. Requires
        a parsing model to be loaded (see head()).

        See also the sd_tokens() method (which invokes
        Stanford Dependencies to determine richer dependency
        information). dependencies() uses simpler head finding rules
        but doesn't require the PyStanfordDependencies package."""
        tree_to_heads = {}
        for tree in reversed(list(self.all_subtrees())):
            if len(tree):
                head = tree.head()
                assert head.span() in tree_to_heads
                tree_to_heads[tree.span()] = tree_to_heads[head.span()]

                for subtree in tree:
                    subhead = tree_to_heads[subtree.span()]
                    if subhead.span() != head.span():
                        yield (head, subhead)
            else:
                tree_to_heads[tree.span()] = tree
    def visualize(self, method='nltk'):
        """Visualize this tree. The method argument determines the
        method used for visualization. Currently 'nltk' is supported
        (requires NLTK to be installed)."""
        if method == 'nltk':
            self._visualize_nltk()
        else:
            raise ValueError("Unknown visualization method: %r" % method)
    def _visualize_nltk(self):
        """Visualize this tree using NLTK."""
        nltk_tree = self.as_nltk_tree()
        import nltk
        nltk.draw.tree.draw_trees(nltk_tree)

    #
    # properties
    #

    def token():
        doc = """The word for the top node in this subtree. If this
        node is not a preterminal, this will be return None. Setting
        the token on a non-preterminal to anything other than None will
        cause a ValueError. The same goes for setting a preterminal's
        token to None."""
        def fget(self):
            return self._tree.word() or None
        def fset(self, new_word):
            new_word = new_word or None
            if self.is_preterminal():
                if new_word is None:
                    raise ValueError("Can't set a null token on a "
                                     "preterminal Tree.")
            else:
                # not a preterminal
                if new_word is not None:
                    raise ValueError("Can't set the token on a "
                                     "non-preterminal Tree.")
            self._tree.setWord(new_word)
        return locals()
    token = property(**token())

    def label():
        doc = """The label at the top of this subtree as a string. If
        this tree is a preterminal, this will be its part of speech,
        otherwise it will be the phrasal category. This property was
        previously a method."""
        def fget(self):
            return self._tree.term()
        def fset(self, new_label):
            self._tree.setTerm(new_label)
        return locals()
    label = property(**label())

    def label_suffix():
        doc = """Suffix for the label at the top node of this subtree
        (including the hyphen). These include function tags (e.g.,
        "-SBJ" for subject") and coindexing ("-2"). In general, this
        will be the empty string for any trees produced by BLLIP parser
        but this property may be set if you read in gold trees."""
        def fget(self):
            return self._tree.ntInfo()
        def fset(self, new_tag):
            self._tree.setNtInfo(new_tag)
        return locals()
    label_suffix = property(**label_suffix())

    def sd_tokens(self, sd_converter=None, conversion_kwargs=None):
        """Convert this Tree to Stanford Dependencies
        (requires PyStanfordDependencies). Returns a list of
        StanfordDependencies.Token objects. This method caches
        the converted tokens. You may optionally specify a
        StanfordDependencies instance in sd_converter and keyword
        arguments to StanfordDependencies.convert_tree as a dictionary
        in conversion_kwargs.

        See also the dependencies() method which will give you syntactic
        dependencies from the parser's head finder."""
        if not self._sd_tokens:
            try:
                import StanfordDependencies
            except ImportError:
                raise ImportError("For sd_tokens(), you need to install "
                                  "PyStanfordDependencies from PyPI")
            sd_converter = sd_converter or StanfordDependencies.get_instance()
            conversion_kwargs = conversion_kwargs or {}
            self._sd_tokens = sd_converter.convert_tree(str(self),
                                                        **conversion_kwargs)
        return self._sd_tokens

    #
    # readers
    #

    @classmethod
    def trees_from_string(this_class, text):
        """Given text containing multiple Penn Treebank trees, returns
        a list of Tree objects (one for each tree in the text)."""
        # Note: the native method below gives us memory ownership of
        # the InputTree objects in the vector. We acquire their pointers
        # and store them in a Python list (the vector won't stick
        # around). InputTree objects typically contain other InputTree
        # objects and the outer tree will free the inner trees when it is
        # deleted. So, we only need (and want) to acquire the pointer of
        # the outermost InputTree tree.
        trees = list(parser.inputTreesFromString(text))
        for tree in trees:
            tree.this.acquire()
        return list(map(this_class, trees))

    @classmethod
    def trees_from_file(this_class, filename):
        """Given the path to a file containing multiple Penn Treebank
        trees, returns a list of Tree objects (one for each tree in the
        file)."""
        # see trees_from_string for an explanation
        trees = list(parser.inputTreesFromFile(filename))
        for tree in trees:
            tree.this.acquire()
        return list(map(this_class, trees))

class ScoredParse:
    """Represents a single parse and its associated parser
    probability and reranker score.

    Properties:
    - ptb_parse: a Tree object representing the parse (str() it to get the
        actual PTB formatted parse)
    - parser_score: The log probability of the parse according to the parser
    - parser_rank: The rank of the parse according to the parser
    - reranker_score: The log probability of the parse according to
      the reranker
    - reranker_rank: The rank of the parse according to the reranker

    The latter two will be None if the reranker isn't being used."""
    def __init__(self, ptb_parse, parser_score=None, reranker_score=None,
                 parser_rank=None, reranker_rank=None):
        self.ptb_parse = ptb_parse
        self.parser_score = parser_score
        self.parser_rank = parser_rank
        self.reranker_score = reranker_score
        self.reranker_rank = reranker_rank
    def __str__(self):
        return "%.12f %.12f %s" % \
            (self.parser_score, self.reranker_score, self.ptb_parse)
    def __repr__(self):
        return "%s(%r, parser_score=%r, reranker_score=%r)" % \
            (self.__class__.__name__, str(self.ptb_parse),
             self.parser_score, self.reranker_score)

class Sentence:
    """Represents a single sentence as input to the parser. Typically,
    you won't need to construct this object directly. This wraps the
    SentRep structure in the Charniak parser."""
    def __init__(self, text_or_tokens):
        if isinstance(text_or_tokens, parser.SentRep):
            # ensure that Python owns the pointer
            text_or_tokens.this.acquire()
            self.sentrep = text_or_tokens
        elif isinstance(text_or_tokens, Sentence):
            self.sentrep = text_or_tokens.sentrep
        elif isinstance(text_or_tokens, string_types):
            self.sentrep = parser.tokenize('<s> ' + text_or_tokens + ' </s>')
        else:
            # text_or_tokens is a sequence -- need to make sure that each
            # element is a string to avoid crashing
            text_or_tokens = [parser.ptbEscape(str(token))
                              for token in text_or_tokens]
            self.sentrep = parser.SentRep(text_or_tokens)
    def __repr__(self):
        """Represent the Sentence as a string."""
        return "%s(%s)" % (self.__class__.__name__, self.tokens())
    def __len__(self):
        """Returns the number of tokens in this sentence."""
        return len(self.sentrep)
    def tokens(self):
        """Returns a list of tokens in this sentence."""
        tokens = []
        for index in range(len(self.sentrep)):
            tokens.append(self.sentrep.getWord(index).lexeme())
        return tokens
    def independent_tags(self):
        """Determine the most likely tags for the words in this sentence,
        considering each word without context. This will not parse the
        sentence but simply use the vocabulary and unknown word model
        to determine the tags."""
        if not RerankingParser._parser_model_loaded:
            raise ValueError("You need to have loaded a parser model in "
                             "order to calculate most likely tags.")
        return Tree(self.sentrep.makeFailureTree('X')).tags()

    @classmethod
    def sentences_from_string(this_class, text):
        """Given text containing SGML(-ish) lines (typical input to
        the command line parser), returns a list of Sentence objects
        (one for each tree in the text). Example usage:

        >>> Sentence.sentences_from_string('<s> Test </s>')
        [bllipparser.RerankingParser.Sentence(['Test'])]
        """
        # Note that the native method below leaks. We work around this
        # by acquiring its pointer in __init__
        sentReps = parser.sentRepsFromString(text)
        return list(map(this_class, sentReps))

    @classmethod
    def sentences_from_file(this_class, filename):
        """Given the path to a filename containing multiple SGML(-ish)
        lines (typical input to the command line parser), returns a list
        of Sentence objects (one for each tree in the text)."""
        # Note that the native method below leaks. We work around this
        # by acquiring its pointer in __init__
        sentReps = parser.sentRepsFromFile(filename)
        return list(map(this_class, sentReps))

class NBestList(object):
    """Represents an n-best list of parses of the same sentence."""
    def __init__(self, sentrep, parses, sentence_id=None):
        # we keep this around since it's our key to converting our input
        # to the reranker's format (see __str__())
        self._parses = parses
        self._sentrep = sentrep
        self.parses = []
        for index, (score, parse) in enumerate(parses):
            # acquire the InputTree pointers or they'll never be freed
            parse.this.acquire()
            scored_parse = ScoredParse(Tree(parse), score, parser_rank=index)
            self.parses.append(scored_parse)
        self.sentence_id = sentence_id
        # True if we've added reranker scores to our parses
        # (but doesn't necessarily imply that we're sorted by them)
        self._reranked = False

    def __iter__(self):
        """Return an iterator over all the parses in this list."""
        return iter(self.parses)
    def __getitem__(self, index):
        """Get a specific parse by index."""
        return self.parses[index]
    def __len__(self):
        """Return the number of parses in this list."""
        return len(self.parses)
    def __repr__(self):
        """Represent this NBestList as a string."""
        return repr(self.parses)

    def sort_by_reranker_scores(self):
        """Sort the parses by the reranker's score (highest to lowest).
        If the reranker scores tie or there are no reranker scores, parser
        probabilities are used as a secondary key."""
        self.parses.sort(key=lambda parse: (parse.reranker_score,
                                            parse.parser_score),
                         reverse=True)
    def sort_by_parser_scores(self):
        """Sort the parses by the parser's probability (most likely to least
        likely)."""
        self.parses.sort(key=lambda parse: -parse.parser_score)
    def get_parser_best(self):
        """Get the best parse in this n-best list according to the
        parser. If there are no parses in this list, returns None."""
        if len(self.parses):
            return min(self, key=lambda parse: parse.parser_rank)
        else:
            return None
    def get_reranker_best(self):
        """Get the best parse in this n-best list according to the
        reranker. If there are no parses in this list, returns None."""
        if len(self.parses):
            return min(self, key=lambda parse: parse.reranker_rank)
        else:
            return None
    def tokens(self):
        """Get the tokens of this sentence as a sequence of strings."""
        return self._sentrep.tokens()
    def rerank(self, reranker_instance, lowercase=True):
        """Rerank this n-best list according to a reranker model.
        reranker_instance can be a RerankingParser or RerankerModel."""
        assert reranker_instance
        if not self.parses:
            self._reranked = True
            return
        if isinstance(reranker_instance, RerankingParser):
            reranker_instance = reranker_instance.reranker_model
        reranker_input = self.as_reranker_input()
        scores = reranker_instance.scoreNBestList(reranker_input)
        # this could be more efficient if needed
        for (score, nbest_list_item) in zip(scores, self.parses):
            nbest_list_item.reranker_score = score
        self.sort_by_reranker_scores()
        for index, nbest_list_item in enumerate(self.parses):
            nbest_list_item.reranker_rank = index
        self._reranked = True
    def fuse(self, threshold=0.5, exponent=1, num_parses=50,
             use_parser_scores=False):
        """Combine the parses in this n-best list into a single Tree
        using parse fusion. This results in a significant accuracy
        improvement. You may want to tune the parameters for your specific
        parsing model. See Choe, McClosky, and Charniak (EMNLP 2015) for
        more details. This will use the scores from the reranker unless
        the n-best list wasn't reranked or use_parser_scores=True. If
        fusion fails, the top parse from the list will be returned."""
        parses = self.parses[:num_parses]

        if use_parser_scores or not self._reranked:
            scores = [scored_parse.parser_score for scored_parse in parses]
        else:
            scores = [scored_parse.reranker_score for scored_parse in parses]
        norm_scores = normalize_logprobs(scores, exponent=exponent)

        chart = parser.SimpleChart(len(self._sentrep))
        for norm_score, scored_parse in zip(norm_scores, parses):
            chart.populate(scored_parse.ptb_parse._tree, norm_score)
        chart.prune(threshold)

        tree = chart.parse()
        if tree is None:
            # parse failed -- use original 1-best parse
            tree = parses[0].ptb_parse
        else:
            tree = Tree(tree)
        return tree
    def __str__(self):
        """Represent the n-best list in a similar output format to the
        command-line parser and reranker."""
        sentence_id = self.sentence_id or 'x'
        if self._reranked:
            from six.moves import cStringIO
            combined = cStringIO()
            combined.write('%d %s\n' % (len(self.parses), sentence_id))
            for parse in self.parses:
                combined.write('%.12f %.12f\n%s\n' % (parse.reranker_score,
                                                      parse.parser_score,
                                                      parse.ptb_parse))
            combined.seek(0)
            return combined.read()
        else:
            if self._parses:
                return parser.asNBestList(self._parses, str(sentence_id))
            else:
                return '0 %s' % sentence_id
    def as_reranker_input(self, lowercase=True):
        """Convert the n-best list to an internal structure used as input
        to the reranker. You shouldn't typically need to call this."""
        return reranker.readNBestList(str(self), lowercase)

class RerankingParser:
    """Wraps the Charniak parser and Johnson reranker into a single
    object. Note that RerankingParser is not thread safe."""
    _parser_model_loaded = False
    _parser_terms_loaded = False
    _parser_heads_loaded = False
    def __init__(self):
        """Create an empty reranking parser. You'll need to call
        load_parser_model() at minimum and load_reranker_model() if
        you're using the reranker. See also the from_unified_model_dir()
        classmethod which will take care of calling both of these
        for you."""
        self.parser_model_dir = None
        self.parser_options = {}
        self.reranker_model = None
        self.unified_model_dir = None

    def __repr__(self):
        if self.unified_model_dir:
            return "%s(unified_model_dir=%r)" % \
                (self.__class__.__name__, self.unified_model_dir)
        else:
            return "%s(parser_model_dir=%r, reranker_model=%r)" % \
                (self.__class__.__name__, self.parser_model_dir,
                 self.reranker_model)

    def load_parser_model(self, model_dir, terms_only=False,
                          heads_only=False, **parser_options):
        """Load the parsing model from model_dir and set parsing
        options. In general, the default options should suffice but see
        the set_parser_options() method for details. Note that the parser
        does not allow loading multiple models within the same process
        (calling this function twice will raise a RuntimeError).

        If terms_only is True, we will not load the full parsing model,
        just part of speech tag information (intended for tools which
        only call things like Tree.evaluate()). If heads_only is True,
        we will only load head finding information (for things like
        Tree.dependencies(). If both are set to True, both of these will
        be loaded but the full parsing model will not."""
        if RerankingParser._parser_model_loaded:
            raise RuntimeError('Parser is already loaded and can only '
                               'be loaded once.')
        self._check_path_or_error(model_dir, 'Parser model directory')
        if not (terms_only or heads_only):
            RerankingParser._parser_model_loaded = True
            RerankingParser._parser_heads_loaded = True
            RerankingParser._parser_terms_loaded = True
            self.parser_model_dir = model_dir
            parser.loadModel(model_dir)
            self.set_parser_options(**parser_options)
        else:
            if terms_only:
                RerankingParser._parser_terms_loaded = True
                parser.loadTermsOnly(model_dir)
            if heads_only:
                RerankingParser._parser_heads_loaded = True
                parser.loadHeadInfoOnly(model_dir)

    def load_reranker_model(self, features_filename, weights_filename,
                            feature_class=None):
        """Load the reranker model from its feature and weights files. A
        feature class may optionally be specified."""
        self._check_path_or_error(features_filename,
                                  'Reranker features filename')
        self._check_path_or_error(weights_filename,
                                  'Reranker weights filename')
        self.reranker_model = reranker.RerankerModel(feature_class,
                                                     features_filename,
                                                     weights_filename)

    def parse(self, sentence, rerank='auto', sentence_id=None):
        """Parse some text or tokens and return an NBestList with the
        results. sentence can be a string or a sequence. If it is a
        string, it will be tokenized. If rerank is True, we will rerank
        the n-best list, if False the reranker will not be used. rerank
        can also be set to 'auto' which will only rerank if a reranker
        model is loaded. If there are no parses or an error occurs,
        this will return an empty NBestList."""
        rerank = self.check_models_loaded_or_error(rerank)

        sentence = Sentence(sentence)
        # max_sentence_length is actually 1 longer than the maximum
        # allowed sentence length
        if len(sentence) >= parser.max_sentence_length - 1:
            raise ValueError("Sentence is too long (%s tokens, must be "
                             "under %s)" %
                             (len(sentence), parser.max_sentence_length - 1))

        try:
            parses = parser.parse(sentence.sentrep)
        except RuntimeError:
            parses = []
        nbest_list = NBestList(sentence, parses, sentence_id)
        if rerank:
            nbest_list.rerank(self)
        return nbest_list

    def parse_tagged(self, tokens, possible_tags, rerank='auto',
                     sentence_id=None):
        """Parse some pre-tagged, pre-tokenized text. tokens must be a
        sequence of strings. possible_tags is map from token indices
        to possible POS tags (strings):

                { index: [tags] }

        Tokens without an entry in possible_tags will be unconstrained by
        POS. If multiple tags are specified, their order will (roughly)
        determine the parsers preference in using them.  POS tags must
        be in the terms.txt file in the parsing model or else you will
        get a ValueError. The rerank flag is the same as in parse()."""
        return self.parse_constrained(tokens, {}, possible_tags,
                                      rerank=rerank, sentence_id=sentence_id)

    def parse_constrained(self, tokens, constraints, possible_tags=None,
                          rerank='auto', sentence_id=None):
        """Parse pre-tokenized text with part of speech and/or phrasal
        constraints. Constraints is a dictionary of

            {(start, end): [terms]}

        which represents the constraint that all spans between [start,end)
        must be one of the terms in that list. start and end are integers
        and terms can be a single string or a list of strings.

        This also allows you to incorporate external POS tags as in
        parse_tagged(). While you can specify a constraint or an external
        POS tag for a word, the semantics are slightly different. Setting
        a tag with possible_tags will allow you to force a word to be a
        POS tag that the parser's tagger would not ordinarily use for
        a tag. Setting a constraint with constraints would only limit
        the set of allowable tags.  Additionally, setting constraints
        doesn't change the probability of the final tree whereas setting
        possible_tags changes the probabilities of words given tags and
        may change the overall probability.

        The rerank flag is the same as in parse()."""
        rerank = self.check_models_loaded_or_error(rerank)
        if isinstance(tokens, string_types):
            raise ValueError("tokens must be a sequence, not a string.")

        if constraints:
            span_constraints = parser.LabeledSpans()
            for (start, end), terms in constraints.items():
                if end <= start:
                    raise ValueError("End must be at least start + 1:"
                                     "(%r, %r) -> %r" % (start, end, terms))
                if isinstance(terms, string_types):
                    terms = [str(terms)]
                for term in terms:
                    span_constraints.addConstraint(int(start), int(end),
                                                   str(term))
        else:
            span_constraints = None

        possible_tags = possible_tags or {}
        ext_pos = self._possible_tags_to_ext_pos(tokens, possible_tags)
        sentence = Sentence(tokens)
        try:
            parses = parser.parse(sentence.sentrep, ext_pos, span_constraints)
            if constraints and not parses:
                raise RuntimeError("Reparsing with relaxed constraints")
        except RuntimeError:
            if span_constraints:
                # we should relax them and retry
                span_constraints.minSizeForParsing = 2
                try:
                    parses = parser.parse(sentence.sentrep, ext_pos,
                                          span_constraints)
                except RuntimeError:
                    parses = []
            else:
                parses = []
        nbest_list = NBestList(sentence, parses, sentence_id)
        if rerank:
            nbest_list.rerank(self)
        return nbest_list

    def simple_parse(self, text_or_tokens):
        """Helper method for just parsing a single sentence and getting
        its Penn Treebank tree.  If you want anything more complicated
        (e.g., the Tree objects, n-best lists, parser or reranker scores,
        etc.), you'll want the parse() or parse_tagged() interfaces.

            >>> rrp.simple_parse('Parse this.')
            '(S1 (S (VP (VB Parse) (NP (DT this))) (. .)))'

        text_or_tokens can be either a string or a sequence of tokens."""
        parses = self.parse(text_or_tokens)
        return str(parses[0].ptb_parse)

    def tag(self, text_or_tokens, allow_failures=False):
        """Helper method for just getting the part-of-speech tags of
        a single sentence. This will parse the sentence and then read
        part-of-speech tags off the tree, so it's not recommended if
        all you need is a fast tagger. Returns a list of (token, tag)
        using Penn Treebank part-of-speech tags.

            >>> rrp.tag('Tag this.')
            [('Tag', 'VB'), ('this', 'DT'), ('.', '.')]

        text_or_tokens can be either a string or a sequence of tokens.
        If allow_failures=True and the parse fails, a ValueError will
        be raised. Otherwise, we will fall back on the most frequent
        POS tag for each word."""
        parses = self.parse(text_or_tokens)
        if parses:
            tokens_and_tags = parses[0].ptb_parse.tokens_and_tags()
        elif not allow_failures:
            sentence = Sentence(text_or_tokens)
            tokens = sentence.tokens()
            tags = sentence.independent_tags()
            tokens_and_tags = list(zip(tokens, tags))
        else:
            raise ValueError('Parse failed while tagging: %r' % text_or_tokens)
        return tokens_and_tags

    def _possible_tags_to_ext_pos(self, tokens, possible_tags):
        ext_pos = parser.ExtPos()
        if not possible_tags:
            return ext_pos
        for index in range(len(tokens)):
            tags = possible_tags.get(index, [])
            if isinstance(tags, string_types):
                tags = [tags]
            tags = list(map(str, tags))
            valid_tags = ext_pos.addTagConstraints(parser.StringVector(tags))
            if not valid_tags:
                # at least one of the tags is bad -- find out which ones
                # and throw a ValueError
                self._find_bad_tag_and_raise_error(tags)
        return ext_pos

    def _find_bad_tag_and_raise_error(self, tags):
        ext_pos = parser.ExtPos()
        bad_tags = set()
        for tag in set(tags):
            good_tag = ext_pos.addTagConstraints(parser.StringVector([tag]))
            if not good_tag:
                bad_tags.add(tag)

        raise ValueError("Invalid POS tags (not present in the parser's "
                         "terms.txt file): %s" % ', '.join(sorted(bad_tags)))

    def check_models_loaded_or_error(self, rerank):
        """Given a reranking mode (True, False, 'auto') determines whether
        we have the appropriately loaded models. Raises a ValueError
        if model(s) are not loaded. Also returns whether the reranker
        should be used (essentially resolves the value of rerank if
        rerank='auto')."""
        if not self._parser_model_loaded:
            raise ValueError("Parser model has not been loaded.")
        if rerank is True and not self.reranker_model:
            raise ValueError("Reranker model has not been loaded.")
        if rerank == 'auto':
            return bool(self.reranker_model)
        else:
            return rerank

    def _check_path_or_error(self, filename, description):
        try:
            filename.encode('ascii')
        except UnicodeEncodeError:
            raise ValueError("%s '%s' must be an ASCII string." %
                             (description, filename))
        if not exists(filename):
            raise ValueError("%s '%s' does not exist." %
                             (description, filename))

    def set_parser_options(self, language='En', case_insensitive=False,
                           nbest=50, small_corpus=True, overparsing=21,
                           debug=0, smooth_pos=0):
        """Set options for the parser. Note that this is called
        automatically by load_parser_model() so you should only need to
        call this to update the parsing options. The method returns a
        dictionary of the new options.

        The options are as follows: language is a string describing
        the language. Currently, it can be one of En (English), Ch
        (Chinese), or Ar (Arabic). case_insensitive will make the parser
        ignore capitalization. nbest is the maximum size of the n-best
        list. small_corpus=True enables additional smoothing (originally
        intended for training from small corpora, but helpful in many
        situations). overparsing determines how much more time the parser
        will spend on a sentence relative to the time it took to find the
        first possible complete parse. This affects the speed/accuracy
        tradeoff. debug takes a non-negative integer. Setting it higher
        than 0 will cause the parser to print debug messages (surprising,
        no?). Setting smooth_pos to a number higher than 0 will cause the
        parser to assign that value as the probability of seeing a known
        word in a new part-of-speech (one never seen in training)."""
        if not RerankingParser._parser_model_loaded:
            raise RuntimeError('Parser must already be loaded (call '
                               'load_parser_model() first)')

        parser.setOptions(language, case_insensitive, nbest, small_corpus,
                          overparsing, debug, smooth_pos)
        self.parser_options = {
            'language': language,
            'case_insensitive': case_insensitive,
            'nbest': nbest,
            'small_corpus': small_corpus,
            'overparsing': overparsing,
            'debug': debug,
            'smooth_pos': smooth_pos
        }
        return self.parser_options

    @classmethod
    def from_unified_model_dir(this_class, model_dir, parsing_options=None,
                               reranker_options=None, parser_only=False):
        """Create a RerankingParser from a unified parsing model on disk.
        A unified parsing model should have the following filesystem
        structure:

        parser/
            Charniak parser model: should contain pSgT.txt, *.g files
            among others
        reranker/
            features.gz or features.bz2 -- features for reranker
            weights.gz or weights.bz2 -- corresponding weights of those
            features

        If one of these subdirectories is missing, the corresponding
        component will not be loaded. The parser_only flag can be used
        to skip loading the reranker even if it available on disk."""
        parsing_options = parsing_options or {}
        reranker_options = reranker_options or {}
        (parser_model_dir, reranker_features_filename,
         reranker_weights_filename) = get_unified_model_parameters(model_dir)
        if parser_only and reranker_options:
            raise ValueError("Can't set reranker_options if "
                             "parser_only is on.")

        rrp = this_class()
        if parser_model_dir:
            rrp.load_parser_model(parser_model_dir, **parsing_options)
        if reranker_features_filename and reranker_weights_filename and \
           not parser_only:
            rrp.load_reranker_model(reranker_features_filename,
                                    reranker_weights_filename,
                                    **reranker_options)

        rrp.unified_model_dir = model_dir
        return rrp

    @classmethod
    def fetch_and_load(this_class, model_name, models_directory=None,
                       verbose=False, extra_loading_options=None):
        """Downloads, installs, and creates a RerankingParser object for
        a specified model using ModelFetcher.download_and_install_model.
        model_name must be one of the defined models in ModelFetcher
        and models_directory is a directory where the model will
        be installed. By default, models will be installed to
        "~/.local/share/bllipparser". Setting verbose=True will
        cause ModelFetcher to be print additional details. Finally,
        extra_loading_options is a dictionary of keyword arguments
        passed to RerankingParser.from_unified_model_dir to customize
        the parsing models."""
        from .ModelFetcher import download_and_install_model
        model_dir = download_and_install_model(model_name,
                                               models_directory,
                                               verbose)

        kwargs = extra_loading_options or {}
        return this_class.from_unified_model_dir(model_dir, **kwargs)

def tokenize(text):
    """Helper method to tokenize a string. Note that most methods accept
    untokenized text so you shouldn't need to run this if you intend
    to parse this text. Returns a list of string tokens."""
    sentence = Sentence(text)
    return sentence.tokens()

def get_unified_model_parameters(model_dir):
    """Determine the actual parser and reranker model filesystem entries
    for a unified parsing model. Returns a triple:

    (parser_model_dir, reranker_features_filename,
     reranker_weights_filename)

    Any of these can be None if that part of the model is not present
    on disk (though, if you have only one of the reranker model files,
    the reranker will not be loaded).

    A unified parsing model should have the following filesystem structure:

    parser/
        Charniak parser model: should contain pSgT.txt, *.g files
        among others
    reranker/
        features.gz or features.bz2 -- features for reranker
        weights.gz or weights.bz2 -- corresponding weights of those
        features
    """
    if not exists(model_dir):
        raise IOError("Model directory '%s' does not exist" % model_dir)

    parser_model_dir = join(model_dir, 'parser')
    if not exists(parser_model_dir):
        parser_model_dir = None
    reranker_model_dir = join(model_dir, 'reranker')

    def get_reranker_model_filename(name):
        filename = join(reranker_model_dir, '%s.gz' % name)
        if not exists(filename):
            # try bz2 version
            filename = join(reranker_model_dir, '%s.bz2' % name)
        if not exists(filename):
            filename = None
        return filename

    features_filename = get_reranker_model_filename('features')
    weights_filename = get_reranker_model_filename('weights')
    return (parser_model_dir, features_filename, weights_filename)
