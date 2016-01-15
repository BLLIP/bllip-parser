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

from __future__ import print_function

from os.path import exists
import shutil
import unittest
from bllipparser.ModelFetcher import (download_and_install_model,
                                      list_models, get_model_info,
                                      UnknownParserModel)

class ModelFetcherTests(unittest.TestCase):
    def test_download1(self):
        model_dir = download_and_install_model('WSJ', verbose=False)
        self.failUnless(isinstance(model_dir, str))
        self.failUnless(model_dir)
        self.failUnless(exists(model_dir))

        # make sure it works a second time -- would ideally verify that
        # it didn't download a second time but there's no API for that
        # currently
        model_dir = download_and_install_model('WSJ', verbose=False)
        self.failUnless(isinstance(model_dir, str))
        self.failUnless(model_dir)
        self.failUnless(exists(model_dir))

        shutil.rmtree(model_dir)
    def test_download2(self):
        model_info = get_model_info('WSJ')
        self.assertEqual(str(model_info),
                         'Wall Street Journal corpus from Penn Treebank, '
                         'version 2 ("AnyDomain" version) [52MB]')
        model_dir = download_and_install_model(model_info.url, verbose=True)
        self.failUnless(isinstance(model_dir, str))
        self.failUnless(model_dir)
        self.failUnless(exists(model_dir))

        shutil.rmtree(model_dir)
    def test_download3(self):
        model_dir = download_and_install_model('WSJ-PTB3', verbose=True,
                                               models_directory='/tmp/models')
        self.failUnless(isinstance(model_dir, str))
        self.failUnless(model_dir)
        self.failUnless(exists(model_dir))

        shutil.rmtree(model_dir)
    def test_unknown_model(self):
        self.assertRaises(UnknownParserModel, download_and_install_model,
                          'bogusmodelname')
        try:
            download_and_install_model('bogusmodelname')
        except UnknownParserModel as u:
            # make sure these don't crash (this was once a problem...)
            str(u)
            repr(u)
    def test_list_models(self):
        list_models()
