#!/usr/bin/env python
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

"""Simple BLLIP Parser unified parsing model repository and installer."""
from __future__ import division
from __future__ import print_function
import sys
from six.moves.urllib.request import FancyURLopener
from six.moves.urllib.parse import urlparse
from os import makedirs, system, chdir, getcwd
from os.path import basename, exists, join, expanduser

DEFAULT_MODELS_DIR = '~/.local/share/bllipparser'

class ModelInfo:
    """Information about a unified BLLIP Parser model. Interface and
    fields subject to change."""
    def __init__(self, model_desc, url, uncompressed_size):
        """uncompressed_size is approximate size in megabytes."""
        self.model_desc = model_desc
        self.url = url
        self.uncompressed_size = uncompressed_size
    def __str__(self):
        return "%s [%sMB]" % (self.model_desc, self.uncompressed_size)

# should this grow large enough, we'll find a better place to store it
# also should include checksum information
models = {
    'GENIA+PubMed': ModelInfo('Self-trained model on GENIA treebank and approx. 200k sentences from PubMed',
                            'https://www.dropbox.com/s/ev3h78gq7526xdj/BLLIP-GENIA-PubMed.tar.bz2?dl=1', 152),
    'OntoNotes-WSJ': ModelInfo('WSJ portion of OntoNotes',
                               'https://www.dropbox.com/s/aiwswoczkjow68y/BLLIP-OntoNotes-WSJ.tar.bz2?dl=1', 61),
    'SANCL2012-Uniform': ModelInfo('Self-trained model on OntoNotes-WSJ and the Google Web Treebank',
                                   'https://www.dropbox.com/s/99wvr79o979lv7z/BLLIP-SANCL2012-Uniform.tar.bz2?dl=1', 890),
    'WSJ+Gigaword': ModelInfo('Self-trained model on PTB2-WSJ and approx. two million sentences from Gigaword (deprecated)',
                              'https://www.dropbox.com/s/1temnpqodo3q310/BLLIP-WSJ-Gigaword2000.tar.bz2?dl=1', 473),
    'WSJ+Gigaword-v2': ModelInfo('Improved self-trained model on PTB WSJ and two million sentences from Gigaword',
                                 'https://www.dropbox.com/s/ilf2cdlhkq6g0t4/BLLIP-WSJ-Gigaword2000-v2.tar.bz2?dl=1', 435),
    'WSJ-PTB3': ModelInfo('Wall Street Journal corpus from Penn Treebank, version 3',
                          'https://www.dropbox.com/s/ulcfs7my1ifriuu/BLLIP-WSJ-PTB3.tar.bz2?dl=1', 55),
    'WSJ': ModelInfo('Wall Street Journal corpus from Penn Treebank, version 2 ("AnyDomain" version)',
                     'https://www.dropbox.com/s/htade2lvno56jge/BLLIP-WSJ-no-AUX.tar.bz2?dl=1', 52),
    'WSJ-with-AUX': ModelInfo('Wall Street Journal corpus from Penn Treebank, version 2 (AUXified version, deprecated)',
                              'https://www.dropbox.com/s/t8pweizt4lgs8fx/BLLIP-WSJ-with-AUX.tar.bz2?dl=1', 55),
}

class UnknownParserModel(ValueError):
    def __str__(self):
        return "Unknown parser model name: " + self.args[0]

def get_model_info(model_name):
    """Given a model name, provides its information as a ModelInfo
    object. Raises UnknownParserModel if model_name is not in the
    model database."""
    try:
        return models[model_name]
    except KeyError:
        raise UnknownParserModel(model_name)

def download_and_install_model(model_name, models_directory=None,
                               verbose=False):
    """Downloads and installs models to a specific directory. Models can
    be specified by simple names (use list_models() for a list of known
    models) or a URL. If the model is already installed as a subdirectory
    of models_directory, it won't download it again. By default,
    models will be downloaded to "~/.local/share/bllipparser". Setting
    verbose=True will print extra details and download progress. Returns
    the path to the new model."""

    if model_name.lower().startswith('http'):
        parsed_url = urlparse(model_name)
        model_url = model_name
        model_name = basename(parsed_url.path).split('.')[0]
    elif model_name in models:
        model_url = models[model_name].url
    else:
        raise UnknownParserModel(model_name)

    models_directory = models_directory or DEFAULT_MODELS_DIR
    models_directory = expanduser(models_directory)

    output_path = join(models_directory, model_name)
    if verbose:
        print("Model directory:", output_path)

    if exists(output_path):
        if verbose:
            print("Model directory already exists, not reinstalling")
        return output_path
    elif verbose:
        print("Fetching model: %s from %s" % (model_name, model_url))

    if verbose:
        def status_func(blocks, block_size, total_size):
            amount_downloaded = blocks * block_size
            if total_size == -1:
                sys.stdout.write('Downloaded %s\r' % amount_downloaded)
            else:
                percent_downloaded = 100 * amount_downloaded / total_size
                size = amount_downloaded / (1024 ** 2)
                sys.stdout.write('Downloaded %.1f%% (%.1f MB)\r' %
                                 (percent_downloaded, size))
    else:
        status_func = None

    # needed since 404s, etc. aren't handled otherwise
    class ErrorAwareOpener(FancyURLopener):
        def http_error_default(self, url, fp, errcode, errmsg, headers):
            raise SystemExit("Error downloading model (%s %s)" %
                             (errcode, errmsg))

    opener = ErrorAwareOpener()
    downloaded_filename, headers = opener.retrieve(model_url,
                                                   reporthook=status_func)
    if verbose:
        sys.stdout.write('\rDownload complete' + (' ' * 20) + '\n')
        print('Downloaded to temporary file', downloaded_filename)

    try:
        makedirs(output_path)
    except OSError as ose:
        if ose.errno != 17:
            raise

    orig_path = getcwd()
    chdir(output_path)
    # by convention, all models are currently in tar.bz2 format
    # we may want to generalize this code later
    assert downloaded_filename.lower().endswith('.bz2')
    command = 'tar xvjf %s' % downloaded_filename
    if verbose:
        print("Extracting with %r to %s" % (command, output_path))
    system(command)
    chdir(orig_path)

    return output_path

def list_models():
    import textwrap
    print(len(models), "known unified parsing models: [uncompressed size]")
    for key, model_info in sorted(models.items()):
        print('%s:' % key)
        print(textwrap.fill(str(model_info), initial_indent='    ',
                            subsequent_indent='    '))

def main():
    from optparse import OptionParser
    parser = OptionParser(usage="""%prog [options]

Tool to help you download and install BLLIP Parser models.""")
    parser.add_option("-l", "--list", action='store_true',
                      help="List known parsing models.")
    parser.add_option("-i", "--install", metavar="NAME", action='append',
                      help="Install a unified parser model.")
    parser.add_option("-d", "--directory", default=DEFAULT_MODELS_DIR,
                      metavar="PATH",
                      help="Directory to install parsing models in (will be "
                           "created if it doesn't exist). Default: %default")

    (options, args) = parser.parse_args()

    if not (options.list or options.install):
        parser.print_help()
        # flip this on to make 'list' the default action
        options.list = True
        print()
    if options.list:
        list_models()
    if options.install:
        for i, model in enumerate(options.install):
            if i:
                print()
            try:
                download_and_install_model(model, options.directory,
                                           verbose=True)
            except UnknownParserModel as u:
                print(u)
                list_models()
                sys.exit(1)

if __name__ == "__main__":
    main()
