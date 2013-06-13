# @@@LICENSE
#
#      Copyright (c) 2010-2013 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# LICENSE@@@

#!/bin/sh

# Initial fake values for battery reads

mkdir -p /tmp/powerd/fake/battery/
cd /tmp/powerd/fake/battery/

echo 99.21875 > getage
echo 85703 > getavgcurrent
echo 748.800 > getcoulomb
echo 85703 > getcurrent
echo 1150.000 > getfull40
echo 66 > getpercent
echo 761.250 > getrawcoulomb
echo 38 > gettemp
echo 3928400 > getvoltage
