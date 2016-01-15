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

import unittest
from bllipparser import Tree

sample_tree = '(S1 (S (NP (DT This)) (VP (VBZ is) (NP (DT a) (ADJP ' \
              '(RB fairly) (JJ simple)) (NN parse) (NN tree))) (. .)))'
sample_tree_pretty = '''(S1 (S (NP (DT This))
     (VP (VBZ is)
      (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree)))
     (. .)))'''

class TreeTests(unittest.TestCase):
    def test_tree_basics(self):
        tree = Tree(sample_tree)
        assert str(tree) == sample_tree
        assert tree.pretty_string() == sample_tree_pretty
        assert tree.tokens() == ('This', 'is', 'a', 'fairly', 'simple',
                                 'parse', 'tree', '.')
        assert tree.tags() == ('DT', 'VBZ', 'DT', 'RB', 'JJ', 'NN',
                               'NN', '.')
        assert tree.tokens_and_tags() == \
            [('This', 'DT'), ('is', 'VBZ'), ('a', 'DT'), ('fairly', 'RB'),
             ('simple', 'JJ'), ('parse', 'NN'), ('tree', 'NN'), ('.', '.')]
        assert tree.span() == (0, 8)
        assert tree.label == 'S1'

        subtrees = tree.subtrees()
        assert len(subtrees) == 1
        assert str(subtrees[0]) == '(S (NP (DT This)) (VP (VBZ is) (NP ' \
                                   '(DT a) (ADJP (RB fairly) (JJ simple)) ' \
                                   '(NN parse) (NN tree))) (. .))'
        assert subtrees[0].label == 'S'
        assert str(subtrees[0][0]) == '(NP (DT This))'
        assert subtrees[0][0].label == 'NP'
        assert subtrees[0][0].span() == (0, 1)
        assert subtrees[0][0].tags() == ('DT',)
        assert subtrees[0][0].tokens() == ('This',)
        assert str(subtrees[0][0][0]) == '(DT This)'
        assert subtrees[0][0][0].token == 'This'
        assert subtrees[0][0][0].label == 'DT'
        assert tree[0][0][0].is_preterminal()
        assert len(tree[0]) == 3

        subtrees = iter(tree[0])
        assert str(next(subtrees)) == '(NP (DT This))'
        assert str(next(subtrees)) == '(VP (VBZ is) (NP (DT a) (ADJP ' \
                                      '(RB fairly) (JJ simple)) (NN parse) ' \
                                      '(NN tree)))'
        assert str(next(subtrees)) == '(. .)'

        pairs = [(False, sample_tree),
                 (False, '(S (NP (DT This)) (VP (VBZ is) (NP (DT a) (ADJP '
                         '(RB fairly) (JJ simple)) (NN parse) (NN tree))) '
                         '(. .))'),
                 (False, '(NP (DT This))'),
                 (True, '(DT This)'),
                 (False, '(VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ '
                         'simple)) (NN parse) (NN tree)))'),
                 (True, '(VBZ is)'),
                 (False, '(NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN '
                         'parse) (NN tree))'),
                 (True, '(DT a)'),
                 (False, '(ADJP (RB fairly) (JJ simple))'),
                 (True, '(RB fairly)'),
                 (True, '(JJ simple)'),
                 (True, '(NN parse)'),
                 (True, '(NN tree)'),
                 (True, '(. .)')]
        actual_pairs = [(subtree.is_preterminal(), str(subtree))
                        for subtree in tree.all_subtrees()]
        assert pairs == actual_pairs

        # index into a preterminal
        self.assertRaises(IndexError, lambda: tree[0][0][0][0])
        # index a child that doesn't exist
        self.assertRaises(IndexError, lambda: tree[500])
        self.assertRaises(IndexError, lambda: tree[0][0][7777])
        self.assertRaises(IndexError, lambda: tree[-30])

        # repr shouldn't crash, but we don't check (or rely on) its form
        repr(tree)
        repr(tree[0])
        repr(tree[0][1])
        repr(tree[0][1][0])

    def test_tree_asciitree(self):
        tree = Tree('(S1 (NP (NN asciitree)) (VP (VBZ is) (NP (DT a) (NN dependency))))')
        self.assertEqual(tree.format_asciitree().strip(), '''S1 
  +-- NP 
  |  +-- NN asciitree
  +-- VP 
     +-- VBZ is
     +-- NP 
        +-- DT a
        +-- NN dependency'''.strip())

    def test_tree_nltk(self):
        tree = Tree('(S1 (NP (NN NLTK)) (VP (VBZ is) (NP (DT a) (NN dependency))))')
        nltk_tree = tree.as_nltk_tree()
        self.assertEqual(len(nltk_tree), 2)
        self.assertEqual(len(nltk_tree.leaves()), 4)
        self.assertEqual(str(nltk_tree), str(tree))
        self.assertEqual(nltk_tree.label(), 'S1')
        self.assertRaises(ValueError, tree.visualize, 'bad vis method')

    def test_tree_errors(self):
        # test issue #33
        self.assertRaises(RuntimeError, Tree, '(())')
        # make sure we can still load good trees after an error
        Tree(sample_tree)
        Tree(sample_tree)
        self.assertRaises(RuntimeError, Tree, '(BADTOPTAG hi)')
        self.assertRaises(RuntimeError, Tree,
                          'Does not start with a paren')
        self.assertRaises(RuntimeError, Tree, '(S1 eh)')
        self.assertRaises(RuntimeError, Tree, '(S1')
        self.assertRaises(RuntimeError, Tree, '(S1 ((')
        self.assertRaises(RuntimeError, Tree, '(S1 (NP')
        Tree(sample_tree)

        self.assertRaises(TypeError, Tree, 1)
        self.assertRaises(TypeError, Tree, None)
        self.assertRaises(TypeError, Tree, {})
        self.assertRaises(TypeError, Tree, len)

    def test_tree_trees_from_string(self):
        trees = Tree.trees_from_string('(S1(X    (NN junk)))')
        assert len(trees) == 1
        assert str(trees[0]) == '(S1 (X (NN junk)))'

        trees2 = Tree.trees_from_string('''(S1 (S (NP (DT This))
    (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree))) (. .)))
(S1 (X    (NN junk)))
(S1(X(NN nospace)))

(S1 (NP (DT another)
                                    (JJ junk)

        (NN tree)))
''')
        assert len(trees2) == 4
        assert str(trees2[0]) == sample_tree
        assert str(trees2[1]) == '(S1 (X (NN junk)))'
        assert str(trees2[2]) == '(S1 (X (NN nospace)))'
        assert str(trees2[3]) == '(S1 (NP (DT another) (JJ junk) (NN tree)))'

        trees3 = Tree.trees_from_string('')
        assert len(trees3) == 0

        trees4 = Tree.trees_from_string(sample_tree)
        assert len(trees4) == 1
        assert str(trees4[0]) == sample_tree

    def test_tree_trees_from_file(self):
        import tempfile
        tree_file = tempfile.NamedTemporaryFile('w+t', delete=False)
        print(tree_file)
        print(tree_file.name)
        tree_file.write('''(S1 (S (NP (DT This))
    (VP (VBZ is) (NP (DT a) (ADJP (RB fairly) (JJ simple)) (NN parse) (NN tree))) (. .)))
(S1 (X    (NN junk)))
(S1(X(NN nospace)))

(S1 (NP (DT another)
                                    (JJ junk)

        (NN tree)))
''')
        tree_file.flush()

        trees2 = Tree.trees_from_file(tree_file.name)
        assert len(trees2) == 4
        assert str(trees2[0]) == sample_tree
        assert str(trees2[1]) == '(S1 (X (NN junk)))'
        assert str(trees2[2]) == '(S1 (X (NN nospace)))'
        assert str(trees2[3]) == '(S1 (NP (DT another) (JJ junk) (NN tree)))'

    def test_tree_modify_tree(self):
        tree = Tree(sample_tree)
        assert str(tree) == sample_tree
        assert str(tree[0][1][0]) == '(VBZ is)'

        tree[0][1][0].label = 'ZZZ'
        assert str(tree[0][1][0]) == '(ZZZ is)'

        tree[0][1][0].token = 'displays'
        assert str(tree[0][1][0]) == '(ZZZ displays)'
        assert str(tree) == '(S1 (S (NP (DT This)) (VP (ZZZ displays) (NP ' \
                            '(DT a) (ADJP (RB fairly) (JJ simple)) (NN ' \
                            'parse) (NN tree))) (. .)))'

        tree[0][1].label_suffix = '-SUFFIX'
        assert tree[0][1].label_suffix == '-SUFFIX'
        assert str(tree[0][1]) == '(VP-SUFFIX (ZZZ displays) (NP (DT a) ' \
                                  '(ADJP (RB fairly) (JJ simple)) (NN ' \
                                  'parse) (NN tree)))'

        self.assertRaises(ValueError, setattr, tree[0], 'token', 'anything')
        self.assertRaises(ValueError, setattr, tree[0][-1], 'token', None)

        tree[0][-1].token = '!'
        assert str(tree) == '(S1 (S (NP (DT This)) (VP-SUFFIX (ZZZ ' \
                            'displays) (NP (DT a) (ADJP (RB fairly) (JJ ' \
                            'simple)) (NN parse) (NN tree))) (. !)))'

        tree[0][1].label_suffix = ''
        assert str(tree) == '(S1 (S (NP (DT This)) (VP (ZZZ ' \
                            'displays) (NP (DT a) (ADJP (RB fairly) (JJ ' \
                            'simple)) (NN parse) (NN tree))) (. !)))'

        # slice testing
        pieces = tree[0][1][0:1]
        assert len(pieces) == 1
        assert str(pieces[0]) == '(ZZZ displays)'

        pieces2 = tree[0][1][-2:]
        assert len(pieces2) == 2
        assert str(pieces2[0]) == '(ZZZ displays)'
        assert str(pieces2[1]) == '(NP (DT a) (ADJP (RB fairly) (JJ ' \
                                  'simple)) (NN parse) (NN tree))'
