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

import unittest
from bllipparser.Utility import import_maybe

class UtilityTests(unittest.TestCase):
    def test_import_maybe(self):
        time_module = import_maybe('time')
        import time
        self.assertEqual(time, time_module)
        nothing = import_maybe('adummypackagenamewhichbetternotexist')
        self.assertEqual(nothing, None)
