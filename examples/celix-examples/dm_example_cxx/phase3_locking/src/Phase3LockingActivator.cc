/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


#include "Phase3LockingCmp.h"
#include "Phase3LockingActivator.h"

#include <memory>

#include "celix/BundleActivator.h"



celix::IBundleActivator* celix::createBundleActivator(celix::BundleContext &ctx) {
    return new Phase3LockingActivator{ctx};
}

Phase3LockingActivator::Phase3LockingActivator(celix::BundleContext& ctx) {
    auto &mng = ctx.getDependencyManager();
    auto inst = std::shared_ptr<Phase3LockingCmp> {new Phase3LockingCmp {}};

    Component<Phase3LockingCmp>& cmp = mng.createComponent<Phase3LockingCmp>(inst)  //set inst using a shared ptr
        .setCallbacks(nullptr, &Phase3LockingCmp::start, &Phase3LockingCmp::stop, nullptr);

    cmp.createServiceDependency<IPhase2>()
            .setStrategy(DependencyUpdateStrategy::locking)
            .setCallbacks(&Phase3LockingCmp::addPhase2, &Phase3LockingCmp::removePhase2);
}

Phase3LockingActivator::~Phase3LockingActivator() {}