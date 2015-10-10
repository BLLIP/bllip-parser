"""
cvlm corpus read/transform support.

Data structures and utilities to read the format that cvlm (reranker
optimizer) takes as input (sparse feature values associated with each
candidate).

This needs the waterworks utility library.

Example:

>>> corpus = RerankerFeatureCorpus('path/to/filename.gz')
>>> for sentence in corpus:
...     print 'index', sentence.index, 'num parses', len(sentence.parses)
...     print 'num parse0 features', len(sentence.parses[0].features)

"""
from waterworks.Strings import try_parse_float, try_parse_int
from waterworks.Files import possibly_compressed_file
from waterworks.Tools import initialize, generic_repr

def parse_kv_list(text):
    """Parse cvlm key-value pairs from text. Returns a dictionary."""
    pieces = text.split()
    results = {}
    for piece in pieces:
        if '=' in piece:
            key, value = piece.split('=')
            if value.endswith(','):
                value = value[:-1]
            value = try_parse_float(value, value)
        else:
            key = piece
            value = 1
        if key.endswith(','):
            key = key[:-1]
        key = try_parse_int(key, key)
        results[key] = value
    return results

def generate_kv_list(features):
    """Render cvlm key-value pairs to text from a dictionary."""
    pieces = []
    for k, v in sorted(features.items()):
        if v == 1:
            pieces.append(str(k))
        else:
            pieces.append('%s=%s' % (k, v))
    return ' '.join(pieces)

class RerankerParse:
    """A single parse of a RerankerSentence. Each parse includes
    the number of proposed and matched brackets (which, combined with
    gold_brackets will tell you its f-score) and a dictionary of features
    to values."""
    def __init__(self, proposed_brackets, matched_brackets, features):
        initialize(self, locals())
    __repr__ = generic_repr
    def cvlm_format(self):
        """Render this parse in cvlm's sparse feature vector format."""
        meta = 'P=%s W=%s' % (self.proposed_brackets, self.matched_brackets)
        if self.features:
            return '%s %s' % (meta, generate_kv_list(self.features))
        else:
            return meta

class RerankerSentence:
    """A single sentence for input to cvlm. Each sentence includes the
    number of gold brackets, its index in the corpus, and a list of all
    candidate parses (RerankerParse objects)."""
    def __init__(self, gold_brackets, parses, index):
        initialize(self, locals())
    __repr__ = generic_repr

    def cvlm_format(self):
        """Render this sentence in cvlm's sparse feature vector format."""
        return 'G=%s N=%s %s' % (self.gold_brackets, len(self.parses),
                                 ', '.join(parse.cvlm_format()
                                           for parse in self.parses)) + ','
    def __iter__(self):
        return iter(self.parses)

    @classmethod
    def from_string(this_class, text, index):
        parses_text = text.split(', ')
        gold_brackets = None
        parses = []
        for parse_index, parse_text in enumerate(parses_text):
            features = parse_kv_list(parse_text)
            if parse_index == 0:
                gold_brackets = features.pop('G')
                features.pop('N')
            proposed_brackets = features.pop('P')
            matched_brackets = features.pop('W')

            parses.append(RerankerParse(proposed_brackets, matched_brackets,
                                        features))
        assert gold_brackets is not None
        return this_class(gold_brackets, parses, index)

class RerankerFeatureCorpus:
    """Made up of a series of sentences. Because these files are huge
    and the Python wrappers around these structures cannot typically be
    stored in memory, this only lets you iterate over the corpus. Note
    that if you're generating a new reranker input file, you'll need
    to create the S=<number of sentences> header as well. The number
    of sentences in the RerankerFeatureCorpus corpus is available under
    the num_sentences property."""
    def __init__(self, filename):
        initialize(self, locals())

        self.reader = iter(possibly_compressed_file(filename))
        self.header = parse_kv_list(next(self.reader))
        assert 'S' in self.header
        self.num_sentences = self.header['S']

    __repr__ = generic_repr

    def __iter__(self):
        for i, line in enumerate(self.reader):
            sentence = RerankerSentence.from_string(line, i)
            sentence.header = self.header
            yield sentence

    def transform(self, transformer):
        """Iterate over every sentence in this corpus, applying a
        transformation function to each. The transformer will be called
        on each RerankerSentence instance in order."""
        for sentence in self:
            yield transformer(sentence)
