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

"""Simple interactive shell for viewing parses. To run:

    python -mbllipparser.ParsingShell /path/to/model/

Optional dependencies:
If you have NLTK installed, you'll be able to use the 'visual' command
which shows constituency trees.
If you have PyStanfordDependencies installed, you'll be able to use the
'sdparse' command. Dependencies are shown in CoNLL-X format, though ASCII
trees will be shown if you have the asciitree package."""
import sys
from cmd import Cmd
import importlib
def import_maybe(module_name):
    "Import a module and return it if available, otherwise returns None."
    try:
        return importlib.import_module(module_name)
    except ImportError:
        return None

try:
    import nltk.tree
    import nltk.draw.tree
    read_nltk_tree = nltk.tree.Tree.parse
    have_nltk_tree_drawing = True
except ImportError:
    have_nltk_tree_drawing = False
except AttributeError: # handle NLTK API changes
    try:
        read_nltk_tree = nltk.tree.Tree.fromstring
        have_nltk_tree_drawing = True
    except AttributeError:
        have_nltk_tree_drawing = False

StanfordDependencies = import_maybe('StanfordDependencies')
asciitree = import_maybe('asciitree')

from bllipparser.RerankingParser import RerankingParser

# TODO should integrate with bllipparser.ModelFetcher

class ParsingShell(Cmd):
    def __init__(self, model):
        Cmd.__init__(self)
        self.prompt = 'bllip> '
        if model is None:
            print("Warning: no parsing model to load.")
            print("Specify with: python -mbllipparser.ParsingShell " + \
                  "/path/to/model/")
            self.rrp = None
        else:
            sys.stdout.write("Loading models... ")
            sys.stdout.flush()
            self.rrp = RerankingParser.from_unified_model_dir(model)
            print("done!")
        print("Enter a sentence to see its parse or 'help' for more options.")
        self.last_nbest_list = []
        self.options = {}
        self.sd = None

    def do_visual(self, text):
        """Use reranking parser to parse text.  Visualize top parses from
        parser and reranker."""
        if not have_nltk_tree_drawing:
            print("Can't visualize without NLTK installation.")
            return

        nbest_list = self.parse(text)
        parser_top_parse = str(nbest_list.get_parser_best().ptb_parse)
        parser_top_parse = parser_top_parse.replace('S1', 'parser')
        reranker_top_parse = str(nbest_list[0].ptb_parse)
        reranker_top_parse = reranker_top_parse.replace('S1', 'reranker')

        nltk_trees = [read_nltk_tree(parser_top_parse)]
        if nbest_list[0].parser_rank != 0:
            print("Parser:")
            print(parser_top_parse)
            print()
            print("Reranker's parse: (parser index %d)" % \
                nbest_list[0].parser_rank)
            print(reranker_top_parse)
            nltk_trees.insert(0, read_nltk_tree(reranker_top_parse))

        nltk.draw.tree.draw_trees(*nltk_trees)

    def do_parse(self, text):
        """Use reranking parser to parse text.  Show top parses from
        parser and reranker."""
        self.parse(text)
        self.print_parses()

    def do_nbest(self, text):
        """Use reranking parser to parse text.  Show complete n-best list."""
        nbest_list = self.parse(text)
        for i, item in enumerate(nbest_list):
            print('reranker rank: ', i)
            print('reranker score:', item.reranker_score)
            print('parser rank:   ', item.parser_rank)
            print('parser score:  ', item.parser_score)
            print(item.ptb_parse.pretty_string())
            print()
        print()

    def do_visualnbest(self, text):
        """Usage: visualnbest [start] stop
        Visualizes all parses from start-stop in the n-best list.
        Sentence must already be parsed."""
        if not have_nltk_tree_drawing:
            print("Can't visualize without NLTK installation.")
            return

        pieces = list(map(int, text.split()))
        start = 0
        if len(pieces) == 2:
            start = pieces[0]
            end = pieces[1]
        elif len(pieces) == 1:
            end = pieces[0]
        else:
            print("Should only have 1 or 2 arguments.")
            return
        end += 1 # to make this inclusive of both end points

        nbest_list = self.last_nbest_list
        nltk_trees = []
        for item in nbest_list[start:end]:
            i = item.reranker_rank
            print('reranker rank: ', i)
            print('reranker score:', item.reranker_score)
            print('parser rank:   ', item.parser_rank)
            print('parser score:  ', item.parser_score)
            print(item.ptb_parse.pretty_string())
            tree = str(item.ptb_parse)
            tree = tree.replace('S1', 'S1-r%d-p%d' % (i, item.parser_rank))
            nltk_trees.append(read_nltk_tree(tree))
            print()
        print()
        nltk.draw.tree.draw_trees(*nltk_trees)

    def do_tagged(self, text):
        """Use reranking parser to parse pre-tagged, pre-tokenized text.
        Show top parses from parser and reranker.  Example usage:

        rrp> tagged word1 word2:TAG1 word3:TAG2 word4:TAG2|TAG3

        will require word2 to be tagged with TAG1, word3 to be tagged
        with TAG2 and word4 to be tagged with TAG2 or TAG3."""
        tokens_and_tags = text.split()
        tokens = []
        possible_tags = {}
        for index, token_and_tag in enumerate(tokens_and_tags):
            if ':' in token_and_tag and len(token_and_tag) > 3:
                token, tags = token_and_tag.split(':')
                tokens.append(token)
                possible_tags[index] = tags.split('|')
            else:
                tokens.append(token_and_tag)

        nbest_list = self.rrp.parse_tagged(tokens, possible_tags)
        self.got_nbest_list(nbest_list)
        self.print_parses()

    def do_set(self, text):
        """Set an option. Syntax: set option-name option-value
        Current options:

        sdversion - version of Stanford CoreNLP to use for Stanford
                    Dependencies
        sdbackend - backend to use for Stanford Dependencies conversion
        sdvis     - how to draw Stanford Dependencies (can be conll or
                    asciitree). Default is to use asciitree if available."""
        pieces = text.split()
        if len(pieces) != 2:
            print("Syntax: set option-name option-value")
            return
        key, value = pieces
        self.options[key] = value
        print("set %r = %r" % (key, value))
        if key.startswith('sd'):
            # reset it so it can be reloaded
            self.sd = None

    def do_sdparse(self, text):
        """Use reranking parser to parse text, then show the
        output as Stanford Dependencies in CoNLL format. Requires
        PyStanfordDependencies. You may want to use the 'set' command
        to set the sdversion, sdbackend, and sdvis options
        (see 'help set')."""
        if not StanfordDependencies:
            print("Can't show dependencies without " \
                  "PyStanfordDependencies installation.")
            return

        try:
            # load SD on demand so user can set version options before
            self.load_stanford_dependencies()
            nbest_list = self.parse(text)

            parser_tree = nbest_list.get_parser_best().ptb_parse
            reranker_tree = nbest_list.get_reranker_best().ptb_parse

            parser_tokens = parser_tree.sd_tokens(sd_converter=self.sd)
            reranker_tokens = reranker_tree.sd_tokens(sd_converter=self.sd)
        except StanfordDependencies.JavaRuntimeVersionError as jrve:
            # load_stanford_dependencies and sd_tokens potentially throw
            # this
            print('JavaRuntimeVersionError:', jrve)
            print()
            print("Try running: 'set sdversion 3.4.1'")
            if self.options.get('sdbackend', 'jpype') == 'jpype':
                print("Also, since you're using the jpype backend, " + \
                      "you'll need to restart first.")
            return

        if parser_tokens == reranker_tokens:
            print('Parser and reranker:')
        else:
            print('Parser:')

        self.visualize_sd_tokens(parser_tokens)

        if parser_tokens != reranker_tokens:
            print()
            print('Reranker:')
            self.visualize_sd_tokens(reranker_tokens)

    def default(self, text):
        if text == 'EOF':
            raise SystemExit
        else:
            return self.do_parse(text)

    def print_parses(self):
        nbest_list = self.last_nbest_list
        parser_top_parse = nbest_list.get_parser_best()
        reranker_top_parse = nbest_list[0]

        if reranker_top_parse.parser_rank == 0:
            print(parser_top_parse.ptb_parse.pretty_string())
        else:
            print("Parser's parse:")
            print(parser_top_parse.ptb_parse.pretty_string())
            print()
            print("Reranker's parse: (parser index %d)" % \
                reranker_top_parse.parser_rank)
            print(reranker_top_parse.ptb_parse.pretty_string())
        print()

    def visualize_sd_tokens(self, tokens):
        # eventually, we may add dot/xdot support
        sdvis = self.options.get('sdvis', 'asciitree').lower()
        use_asciitree = sdvis == 'asciitree'
        if use_asciitree and asciitree:
            print(tokens.as_asciitree())
        else:
            for token in tokens:
                print(token.as_conll())

    def got_nbest_list(self, nbest_list):
        nbest_list.sort_by_reranker_scores()
        self.last_nbest_list = nbest_list

    def parse(self, text):
        if text.strip(): # if no text, return the last nbest list
            nbest_list = self.rrp.parse(text)
            print('Tokens:', ' '.join(nbest_list.tokens()))
            print()
            self.got_nbest_list(nbest_list)

        return self.last_nbest_list

    def load_stanford_dependencies(self):
        if self.sd:
            return
        kwargs = dict(version=self.options.get('sdversion'))
        if 'sdbackend' in self.options:
            kwargs['backend'] = self.options['sdbackend']
        self.sd = StanfordDependencies.get_instance(**kwargs)

def main(shell_class=ParsingShell):
    if len(sys.argv) > 1:
        model = sys.argv[-1]
    else:
        model = None
    shell = shell_class(model)
    shell.cmdloop()

if __name__ == "__main__":
    main()
