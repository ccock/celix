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


#include <iostream>
#include <celix/ComponentManager.h>


#include "celix/ComponentManager.h"

celix::GenericServiceDependency::GenericServiceDependency(
        std::shared_ptr<celix::BundleContext> _ctx,
        std::function<void()> _stateChangedCallback,
        std::function<void()> _enable,
        std::function<void()> _disable) : ctx{_ctx}, stateChangedCallback{std::move(_stateChangedCallback)}, enable{std::move(_enable)}, disable{std::move(_disable)} {}


bool celix::GenericServiceDependency::isResolved() const {
    std::lock_guard<std::mutex> lck{mutex};
    if (!tracker.empty()) {
        return tracker[0].trackCount() >= 1;
    } else {
        return false;
    }
}

celix::Cardinality celix::GenericServiceDependency::getCardinality() const {
    std::lock_guard<std::mutex> lck{mutex};
    return cardinality;
}

bool celix::GenericServiceDependency::getRequired() const {
    std::lock_guard<std::mutex> lck{mutex};
    return required;
}

const std::string &celix::GenericServiceDependency::getFilter() const {
    std::lock_guard<std::mutex> lck{mutex};
    return filter;
}

void celix::GenericServiceDependency::setEnable(bool e) {
    if (e) {
        enable();
    } else {
        disable();
    }
}

celix::ComponentState celix::GenericComponentManager::getState() const {
    std::lock_guard<std::mutex> lck{mutex};
    return state;
}

bool celix::GenericComponentManager::isEnabled() const  {
    std::lock_guard<std::mutex> lck{mutex};
    //TODO update state
    return enabled;
}

bool celix::GenericComponentManager::isResolved() const {
    std::lock_guard<std::mutex> lck{mutex};
    for (auto &pair : serviceDependencies) {
        if (!pair.second->isResolved()) {
            return false;
        }
    }
    return true;
}

void celix::GenericComponentManager::removeServiceDependency(long serviceDependencyId) {
    std::lock_guard<std::mutex> lck{mutex};
    serviceDependencies.erase(serviceDependencyId);
}

void celix::GenericComponentManager::setEnabled(bool e) {
    std::lock_guard<std::mutex> lck{mutex};
    enabled = e;
    for (auto &pair : serviceDependencies) {
        pair.second->setEnable(e);
    }
}

void celix::GenericComponentManager::updateState() {
    std::lock_guard<std::mutex> lck{mutex};

    //TODO state thing

    //TODO check enabled.

    bool allDependenciesResolved = true;
    for (auto &pair : serviceDependencies) {
        if (!pair.second->isResolved()) {
            allDependenciesResolved = false;
            break;
        }
    }

    std::cout << "resolved? " << (allDependenciesResolved ? "true" : "false") << std::endl;
}



