#! /bin/sh
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

# RERANKDATA=ec50-connll-ic-s5
# RERANKDATA=ec50-f050902-lics5
MODELDIR=second-stage/models/ec50spfinal
# ESTIMATORNICKNAME=cvlm-l1c10P1
ESTIMATORNICKNAME=lbfgs-l1c10F1n1p2
first-stage/PARSE/parseIt -l399 -N50 first-stage/DATA/EN/ $* | second-stage/programs/features/best-parses -l $MODELDIR/features.gz $MODELDIR/$ESTIMATORNICKNAME-weights.gz
