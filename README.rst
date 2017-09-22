BLLIP Reranking Parser
----------------------
.. image:: https://travis-ci.org/BLLIP/bllip-parser.png?branch=master
   :target: https://travis-ci.org/BLLIP/bllip-parser

.. image:: https://badge.fury.io/py/bllipparser.png
   :target: https://badge.fury.io/py/bllipparser

Copyright Mark Johnson, Eugene Charniak, 24th November 2005 --- August 2006

We request acknowledgement in any publications that make use of this
software and any code derived from this software. Please report the
release date of the software that you are using, as this will enable
others to compare their results to yours.

Overview
~~~~~~~~
BLLIP Parser is a statistical natural language parser including a
generative constituent parser (``first-stage``) and discriminative
maximum entropy reranker (``second-stage``). The latest version can
be found on `GitHub <https://github.com/BLLIP/bllip-parser>`_. This
document describes basic usage of the command line interface and
describes how to build and run the reranking parser. There are now
`Python <http://pypi.python.org/pypi/bllipparser/>`_ and Java interfaces
as well. The Python interface is described in `README-python.rst
<https://github.com/BLLIP/bllip-parser/blob/master/README-python.rst>`_.

Compiling the parser
~~~~~~~~~~~~~~~~~~~~
1. *(optional)* For optimal speed, you may want to define ``$GCCFLAGS``
   specifically for your machine. However, this step can be safely
   skipped as the defaults are usually fine. With ``csh`` or ``tcsh``,
   try something like::

     shell> setenv GCCFLAGS "-march=pentium4 -mfpmath=sse -msse2 -mmmx"

   or::

     shell> setenv GCCFLAGS "-march=opteron -m64"

2. Build the parser with::

    shell> make

   -  Sidenote on compiling on OS X

      OS X uses the ``clang`` compiler by default which cannot currently
      compile the parser. Try setting this environment variable before
      building to change the default C++ compiler::

         shell> setenv CXX g++

      Recent versions of OS X may have additional issues. See issues
      `60 <http://github.com/BLLIP/bllip-parser/issues/60>`_,
      `19 <http://github.com/BLLIP/bllip-parser/issues/19>`_, and `13
      <https://github.com/BLLIP/bllip-parser/issues/13>`_ for more
      information.

Obtaining parser models
~~~~~~~~~~~~~~~~~~~~~~~
The `GitHub repository <https://github.com/BLLIP/bllip-parser/>`_
includes parsing and reranker models, though these are mostly around
for historical purposes.  See this page on `BLLIP Parser models
<https://github.com/BLLIP/bllip-parser/blob/master/MODELS.rst>`_ for
information about obtaining newer and more accurate parsing models.

Running the parser
~~~~~~~~~~~~~~~~~~
After it has been built, the parser can be run with::

    shell> parse.sh <sourcefile.txt>

For example::

    shell> parse.sh sample-text/sample-data.txt

The input text must be pre-sentence segmented with each sentence in an
``<s>`` tag::

    <s> Sentence 1 </s>
    <s> Sentence 2 </s>
    ...

Note that there needs to be a space before and after the sentence.

The parser distribution currently includes a basic Penn Treebank Wall
Street Journal parsing models which ``parse.sh`` will use by default. 
The Python interface to the parser includes a mechanism for listing and
downloading additional parsing models (some of which are more accurate,
depending on what you're parsing).

The script ``parse-and-fuse.sh`` demonstrates how to run syntactic
parse fusion. Fusion can also be run via the Python bindings.

The script ``parse-eval.sh`` takes a list of treebank files as arguments
and extracts the terminal strings from them, runs the two-stage parser
on those terminal strings and then evaluates the parsing accuracy with
Sparseval. For example, if the Penn Treebank 3 is installed at
``/usr/local/data/Penn3/``, the following code evaluates the two-stage
parser on section 24::

   shell> parse-eval.sh /usr/local/data/Penn3/parsed/mrg/wsj/24/wsj*.mrg

The ``Makefile`` will attempt to automatically download and build
Sparseval for you if you run ``make sparseval``.

For more information on `Sparseval
<http://www.clsp.jhu.edu/vfsrv/ws2005/groups/eventdetect/files/SParseval.tgz>`_
see this `paper
<http://www.lrec-conf.org/proceedings/lrec2006/pdf/116_pdf.pdf>`_::

    @inproceedings{roark2006sparseval,
        title={SParseval: Evaluation metrics for parsing speech},
        author={Roark, Brian and Harper, Mary and Charniak, Eugene and 
                Dorr, Bonnie and Johnson, Mark and Kahn, Jeremy G and 
                Liu, Yang and Ostendorf, Mari and Hale, John and
                Krasnyanskaya, Anna and others},
        booktitle={Proceedings of LREC},
        year={2006}
    }

We no longer distribute `evalb <http://nlp.cs.nyu.edu/evalb/>`_ with the
parser since it sometimes skips sentences unnecessarily. Sparseval does
not have these issues.

More questions?
~~~~~~~~~~~~~~~
There is more information about different components of the
parser spread across ``README`` files in this distribution (see
below). BLLIP Parser is
maintained by `David McClosky <http://nlp.stanford.edu/~mcclosky>`_.

- Usage help: `StackOverflow <http://stackoverflow.com/tags/charniak-parser/info>`_ (use ``charniak-parser`` tag)
- Bug reports and feature requests: `GitHub issue tracker <http://github.com/BLLIP/bllip-parser/issues>`_
- Twitter: `@bllipparser <https://twitter.com/bllipparser>`_

Parser details
^^^^^^^^^^^^^^
For details on the running the parser, see `first-stage/README.rst
<https://github.com/BLLIP/bllip-parser/blob/master/first-stage/README.rst>`_.
For help retraining the parser, see `first-stage/TRAIN/README.rst
<https://github.com/BLLIP/bllip-parser/blob/master/first-stage/TRAIN/README.rst>`_ (also includes some information about the parser model file formats).

Reranker details
^^^^^^^^^^^^^^^^
See `second-stage/README
<https://github.com/BLLIP/bllip-parser/blob/master/second-stage/README>`_
for an overview.  `second-stage/README-retrain.rst
<https://github.com/BLLIP/bllip-parser/blob/master/second-stage/README-retrain.rst>`_ details how to retrain the reranker.  The
``second-stage/programs/*/README`` files include additional notes about
different reranker components.

Other versions of the parser
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
We haven't tested these all of these and can't support them, but they
may be useful if you're working on other platforms or languages.

- `Native Charniak parser for Windows
  <https://github.com/dorony/CharniakParserWindows>`_ (doesn't need
  cygwin, no reranker)
- `Rutu Mulkar-Mehta's Windows version
  <http://www.rutumulkar.com/software.html>`_
- `French branch <https://bitbucket.org/djame/bllip-parser-fr>`_ by
  `Djame Seddah <http://pauillac.inria.fr/~seddah/>`_
- `Liang Huang's forest reranker
  <http://web.engr.oregonstate.edu/~huanlian/>`_  (includes forest-dumping
  extensions)
- `Javascript (emscripten) version
  <https://github.com/antimatter15/charniak-emscripten/>`_
  by `Kevin Kwok <http://antimatter15.com>`_ (`live
  demo, <http://antimatter15.com/charniak-emscripten/index.html>`_ no reranker)

References
^^^^^^^^^^

Parser and reranker:

* Eugene Charniak and Mark Johnson. "`Coarse-to-fine n-best parsing and
  MaxEnt discriminative reranking
  <http://aclweb.org/anthology/P/P05/P05-1022.pdf>`_."  Proceedings of
  the 43rd Annual Meeting on Association for Computational Linguistics.
  `Association for Computational Linguistics, 2005
  <http://bllip.cs.brown.edu/publications/index_bib.shtml#charniak-johnson:2005:ACL>`_.

* Eugene Charniak. "`A maximum-entropy-inspired parser
  <http://aclweb.org/anthology/A/A00/A00-2018.pdf>`_." Proceedings of
  the 1st North American chapter of the Association for Computational
  Linguistics conference. `Association for Computational Linguistics, 2000
  <http://bllip.cs.brown.edu/publications/index_bib.shtml#Charniak:2000:NAACL>`_.

Self-training:

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
