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

#ifndef CELIX_CONSTANTS_H_
#define CELIX_CONSTANTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define OSGI_FRAMEWORK_OBJECTCLASS "objectClass"
#define OSGI_FRAMEWORK_SERVICE_ID "service.id"
#define OSGI_FRAMEWORK_SERVICE_PID "service.pid"
#define OSGI_FRAMEWORK_SERVICE_RANKING "service.ranking"

#define CELIX_FRAMEWORK_SERVICE_VERSION "service.version"
#define CELIX_FRAMEWORK_SERVICE_LANGUAGE "service.lang"
#define CELIX_FRAMEWORK_SERVICE_C_LANGUAGE "C"
#define CELIX_FRAMEWORK_SERVICE_CXX_LANGUAGE "C++"

#define OSGI_FRAMEWORK_BUNDLE_ACTIVATOR "Bundle-Activator"

#define OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_CREATE      "celix_bundleActivator_create"
#define OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_START       "celix_bundleActivator_start"
#define OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_STOP        "celix_bundleActivator_stop"
#define OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_DESTROY     "celix_bundleActivator_destroy"

#define OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_CREATE   "bundleActivator_create"
#define OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_START    "bundleActivator_start"
#define OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_STOP     "bundleActivator_stop"
#define OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_DESTROY  "bundleActivator_destroy"

#define OSGI_FRAMEWORK_BUNDLE_SYMBOLICNAME  "Bundle-SymbolicName"
#define OSGI_FRAMEWORK_BUNDLE_VERSION       "Bundle-Version"
#define OSGI_FRAMEWORK_PRIVATE_LIBRARY      "Private-Library"
#define OSGI_FRAMEWORK_EXPORT_LIBRARY       "Export-Library"
#define OSGI_FRAMEWORK_IMPORT_LIBRARY       "Import-Library"

#define OSGI_FRAMEWORK_FRAMEWORK_STORAGE                "org.osgi.framework.storage"
#define OSGI_FRAMEWORK_STORAGE_USE_TMP_DIR              "org.osgi.framework.storage.use.tmp.dir"
#define OSGI_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_NAME     "org.osgi.framework.storage.clean"
#define OSGI_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_DEFAULT  true
#define OSGI_FRAMEWORK_FRAMEWORK_UUID                   "org.osgi.framework.uuid"

#define CELIX_BUNDLES_PATH_NAME     "CELIX_BUNDLES_PATH"
#define CELIX_BUNDLES_PATH_DEFAULT  "bundles"

#define CELIX_AUTO_START_0 "CELIX_AUTO_START_0"
#define CELIX_AUTO_START_1 "CELIX_AUTO_START_1"
#define CELIX_AUTO_START_2 "CELIX_AUTO_START_2"
#define CELIX_AUTO_START_3 "CELIX_AUTO_START_3"
#define CELIX_AUTO_START_4 "CELIX_AUTO_START_4"
#define CELIX_AUTO_START_5 "CELIX_AUTO_START_5"


#ifdef __cplusplus
}
#endif

#endif /* CONSTANTS_H_ */