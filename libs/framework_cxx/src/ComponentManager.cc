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
#include <uuid/uuid.h>
#include <celix/ComponentManager.h>
#include <glog/logging.h>


#include "celix/ComponentManager.h"

static std::string genUUID() {
    uuid_t tmpUUID;
    uuid_string_t str;

    uuid_generate(tmpUUID);
    uuid_unparse(tmpUUID, str);

    return std::string{str};
}

celix::GenericServiceDependency::GenericServiceDependency(
        std::shared_ptr<celix::BundleContext> _ctx,
        std::function<void()> _stateChangedCallback) :
            ctx{std::move(_ctx)},
            stateChangedCallback{std::move(_stateChangedCallback)},
            uuid{genUUID()} {}


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

std::string celix::GenericServiceDependency::getUUD() {
    return uuid;
}

bool celix::GenericServiceDependency::isEnabled() {
    std::lock_guard<std::mutex> lck{mutex};
    return !tracker.empty();
}

celix::ComponentState celix::GenericComponentManager::getState() const {
    std::lock_guard<std::mutex> lck{stateMutex};
    return state;
}

bool celix::GenericComponentManager::isEnabled() const  {
    std::lock_guard<std::mutex> lck{stateMutex};
    return enabled;
}

bool celix::GenericComponentManager::isResolved() const {
    std::lock_guard<std::mutex> lck{stateMutex};
    for (auto &pair : serviceDependencies) {
        if (!pair.second->isResolved()) {
            return false;
        }
    }
    return true;
}

void celix::GenericComponentManager::setEnabled(bool e) {
    {
        std::lock_guard<std::mutex> lck{stateMutex};
        enabled = e;
    }
    std::vector<std::shared_ptr<GenericServiceDependency>> deps{};
    {
        std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
        for (auto &pair : serviceDependencies) {
            deps.push_back(pair.second);
        }
    }

    //NOTE updating outside of the lock
    for (auto &dep : deps) {
        dep->setEnabled(e);
    }
    updateState();
}

void celix::GenericComponentManager::updateState() {
    bool allDependenciesResolved = true;
    {
        std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
        for (auto &pair : serviceDependencies) {
            if (!pair.second->isResolved()) {
                allDependenciesResolved = false;
                break;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lck{stateMutex};
        ComponentState currentState = state;
        ComponentState targetState = ComponentState::Disabled;

        if (enabled && allDependenciesResolved) {
            targetState = ComponentState::Started;
        } else if (enabled) /*not all dep resolved*/ {
            targetState = initialized ? ComponentState::Initialized : ComponentState::Uninitialized;
        }

        if (currentState != targetState) {
            transitionQueue.emplace(currentState, targetState);
        }
    }
    transition();
}

void celix::GenericComponentManager::transition() {
    ComponentState currentState;
    ComponentState targetState;

    {
        std::lock_guard<std::mutex> lck{stateMutex};
        if (transitionQueue.empty()) {
            return;
        }
        auto pair = transitionQueue.front();
        transitionQueue.pop();
        currentState = pair.first;
        targetState = pair.second;
    }

    DLOG(INFO) << "Transition for " << name << " from " << currentState << " to " << targetState;

    //TODO note callbacks are called outside of mutex. make local copies or ensure that callback can only be changed with disabled component managers ...

    if (targetState == ComponentState::Disabled) {
        switch (currentState) {
            case celix::ComponentState::Disabled:
                //nop
                break;
            case celix::ComponentState::Uninitialized:
                //TODO may disable callback to cmp
                setState(ComponentState::Disabled);
                break;
            case celix::ComponentState::Initialized:;
                deinit();
                setInitialized(false);
                setState(ComponentState::Uninitialized);
                break;
            case celix::ComponentState::Started:
                stop();
                setState(ComponentState::Initialized);
                break;
        }
    } else if (targetState == ComponentState::Uninitialized) {
        switch (currentState) {
            case celix::ComponentState::Disabled:
                //TODO maybe enable callback
                setState(ComponentState::Uninitialized);
                break;
            case celix::ComponentState::Uninitialized:
                //nop
                break;
            case celix::ComponentState::Initialized:
                deinit();
                setInitialized(false);
                setState(ComponentState::Uninitialized);
                break;
            case celix::ComponentState::Started:
                stop();
                setState(ComponentState::Initialized);
                break;
        }
    } else if (targetState == ComponentState::Initialized) {
        switch (currentState) {
            case celix::ComponentState::Disabled:
                //TODO maybe enabled callback
                setState(ComponentState::Uninitialized);
                break;
            case celix::ComponentState::Uninitialized:
                init();
                setInitialized(true);
                setState(ComponentState::Initialized);
                break;
            case celix::ComponentState::Initialized:
                //nop
                break;
            case celix::ComponentState::Started:
                stop();
                setState(ComponentState ::Initialized);
                break;
        }
    } else if (targetState == ComponentState::Started) {
        switch (currentState) {
            case celix::ComponentState::Disabled:
                //TODO maybe enabled callback
                setState(ComponentState::Uninitialized);
                break;
            case celix::ComponentState::Uninitialized:
                init();
                setInitialized(true);
                setState(ComponentState::Initialized);
                break;
            case celix::ComponentState::Initialized:
                start();
                setState(ComponentState::Started);
                break;
            case celix::ComponentState::Started:
                //nop
                break;
        }
    } else {
        LOG(ERROR) << "Unexpected target state: " << targetState;
    }

    updateState();
}

void celix::GenericComponentManager::setState(ComponentState s) {
    std::lock_guard<std::mutex> lck{stateMutex};
    state = s;
}


std::string celix::GenericComponentManager::getUUD() const {
    return uuid;
}

celix::GenericComponentManager::GenericComponentManager(std::shared_ptr<celix::BundleContext> _ctx, const std::string &_name) : ctx{std::move(_ctx)}, name{_name}, uuid{genUUID()} {
}

void celix::GenericComponentManager::removeServiceDependency(const std::string& serviceDependencyUUID) {
    std::lock_guard<std::mutex> lck{stateMutex};
    auto it = serviceDependencies.find(serviceDependencyUUID);
    if (it != serviceDependencies.end()) {
        serviceDependencies.erase(it);
    } else {
        LOG(WARNING) << "Cannot find service dependency with uuid " << serviceDependencyUUID << " in component manager " << name << " with uuid " << uuid << ".";
    }
}

std::string celix::GenericComponentManager::getName() const {
    return name;
}

void celix::GenericComponentManager::setInitialized(bool i) {
    std::lock_guard<std::mutex> lck{stateMutex};
    initialized = i;
}


std::ostream& operator<< (std::ostream& out, celix::ComponentState state)
{
    switch (state) {
        case celix::ComponentState::Disabled:
            out << "Disabled";
            break;
        case celix::ComponentState::Uninitialized:
            out << "Uninitialized";
            break;
        case celix::ComponentState::Initialized:
            out << "Initialized";
            break;
        case celix::ComponentState::Started:
            out << "Started";
            break;
    }
    return out;
}

