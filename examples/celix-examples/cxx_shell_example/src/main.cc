/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include <glog/logging.h>

#include "celix/api.h"


int main(int /*argc*/, char **argv) {
    //TODO move glog init to framework (a pthread_once?), so that glog::glog dep can be removed from executables.
    google::InitGoogleLogging(argv[0]);
    google::LogToStderr();

    //TODO create launcher, which handles config.properties and command args
    auto fw = celix::Framework{};
    fw.waitForShutdown();
    return 0;
}