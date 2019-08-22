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

#ifndef CXX_CELIX_COMPONENT_MANAGER_H
#define CXX_CELIX_COMPONENT_MANAGER_H

#include <memory>
#include <unordered_map>
#include <queue>

#include "celix/Utils.h"
#include "celix/BundleContext.h"

namespace celix {

    enum class ComponentState {
        Disabled,
        Uninitialized,
        Initialized,
        Started
    };

    template<typename T, typename I>
    class ServiceDependency; //forward declaration
    class GenericServiceDependency; //forward declaration

    class GenericComponentManager {
    public:
        virtual ~GenericComponentManager() = default;

        ComponentState getState() const;
        bool isEnabled() const;
        bool isResolved() const;
        std::string getName() const;
        std::string getUUD() const;

        //TODO make friend of SvcDep
        void updateState();

        void removeServiceDependency(const std::string& serviceDependencyUUID);
    protected:
        GenericComponentManager(std::shared_ptr<BundleContext> _ctx, const std::string &_name);

        void setEnabled(bool enable);

        /**** Fields ****/
        const std::shared_ptr<BundleContext> ctx;
        const std::string name;
        const std::string uuid;

        std::mutex callbacksMutex{}; //protects below std::functions
        std::function<void()> init{[]{/*nop*/}};
        std::function<void()> start{[]{/*nop*/}};
        std::function<void()> stop{[]{/*nop*/}};
        std::function<void()> deinit{[]{/*nop*/}};

        std::mutex serviceDependenciesMutex{};
        std::unordered_map<std::string,std::shared_ptr<GenericServiceDependency>> serviceDependencies{}; //key = dep uuid
    private:
        void setState(ComponentState state);
        void setInitialized(bool initialized);
        void transition();

        mutable std::mutex stateMutex{}; //protects below
        ComponentState state = ComponentState::Disabled;
        bool enabled = false;
        bool initialized = false;
        std::queue<std::pair<ComponentState,ComponentState>> transitionQueue{};
    };

    template<typename T>
    class ComponentManager : public GenericComponentManager {
    public:
        using ComponentType = T;

        ComponentManager(std::shared_ptr<BundleContext> ctx, std::shared_ptr<T> cmpInstance, const std::string &name = celix::typeName<T>());
        ~ComponentManager() override = default;

        ComponentManager<T>& enable();
        ComponentManager<T>& disable();

        std::shared_ptr<T> instance();

        ComponentManager<T>& setCallbacks(
                void (T::*init)(),
                void (T::*start)(),
                void (T::*stop)(),
                void (T::*deinit)());

        template<typename I>
        ServiceDependency<T,I>& addServiceDependency();

        template<typename I>
        ServiceDependency<T,I>* findServiceDependency(const std::string& serviceDependencyUUID);

        ComponentManager<T>& extractUUID(std::string& out);
//
//        template<typename F>
//        ServiceDependency<T,F>& addFunctionServiceDependency(const std::string &functionName);
    private:
        std::shared_ptr<T> inst;

    };

    enum class Cardinality {
        One,
        Many
    };

    class GenericServiceDependency {
    public:
        virtual ~GenericServiceDependency() = default;

        bool isResolved() const;
        Cardinality getCardinality() const;
        bool getRequired() const;
        const std::string& getFilter() const;
        std::string getUUD();

        bool isEnabled();
        virtual void setEnabled(bool e) = 0;
    protected:
        GenericServiceDependency(
                std::shared_ptr<BundleContext> ctx,
                std::function<void()> stateChangedCallback);

        //Fields
        const std::shared_ptr<BundleContext> ctx;
        const std::function<void()> stateChangedCallback;
        const std::string uuid;

        mutable std::mutex mutex{}; //protects below
        bool required = false;
        std::string filter{};
        Cardinality cardinality = Cardinality::One;
        std::vector<ServiceTracker> tracker{}; //max 1 (1 == enabled / 0 = disabled
    };

    template<typename T, typename I>
    class ServiceDependency : public GenericServiceDependency {
    public:
        using ComponentType = T;
        using ServiceType = I;

        ServiceDependency(std::shared_ptr<BundleContext> ctx, std::function<void()> stateChangedCallback, std::function<std::shared_ptr<T>()> getCmpInstance);
        ~ServiceDependency() override = default;

        ServiceDependency<T,I>& setFilter(const std::string &filter);
        ServiceDependency<T,I>& setRequired(bool required);
        ServiceDependency<T,I>& setCardinality(celix::Cardinality cardinality);
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(std::shared_ptr<I>));
        ServiceDependency<T,I>& setCallbacks(void (T::*add)(std::shared_ptr<I>), void (T::*remove)(std::shared_ptr<I>));
        //TODO update callback
        //TODO callbacks with properties and owner

        ServiceDependency<T,I>& setFunctionCallbacks(
                std::function<void(std::shared_ptr<I>, const celix::Properties &props, const celix::IResourceBundle &owner)> set,
                std::function<void(std::shared_ptr<I>, const celix::Properties &props, const celix::IResourceBundle &owner)> add,
                std::function<void(std::shared_ptr<I>, const celix::Properties &props, const celix::IResourceBundle &owner)> rem,
                std::function<void(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle *>> rankedServices)> update);

        ServiceDependency<T,I>& extractUUID(std::string& out);
        ServiceDependency<T,I>& enable();
        ServiceDependency<T,I>& disable();

        void setEnabled(bool e) override;

    private:
        const std::function<std::shared_ptr<T>()> getCmpInstance;


        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> set{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> add{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> rem{};
        std::function<void(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle *>> rankedServices)> update{};
    };
}

std::ostream& operator<< (std::ostream& out, celix::ComponentState state);



/**************************** IMPLEMENTATION *************************************************************************/

template<typename T>
celix::ComponentManager<T>::ComponentManager(
        std::shared_ptr<BundleContext> ctx,
        std::shared_ptr<T> cmpInstance,
        const std::string &name) : GenericComponentManager{ctx, name}, inst{std::move(cmpInstance)} {
}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::enable() {
    setEnabled(true);
    return *this;
}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::disable() {
    setEnabled(false);
    return *this;
}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::setCallbacks(
        void (T::*memberInit)(),
        void (T::*memberStart)(),
        void (T::*memberStop)(),
        void (T::*memberDeinit)()) {
    std::lock_guard<std::mutex> lck{callbacksMutex};
    init = [this, memberInit]() {
        if (memberInit) {
            (this->instance()->*memberInit)();
        }
    };
    start = [this, memberStart]() {
        if (memberStart) {
            (this->instance()->*memberStart)();
        }
    };
    stop = [this, memberStop]() {
        if (memberStop) {
            (this->instance()->*memberStop)();
        }
    };
    deinit = [this, memberDeinit]() {
        if (memberDeinit) {
            (this->instance()->*memberDeinit)();
        }
    };
    return *this;
}

template<typename T>
std::shared_ptr<T> celix::ComponentManager<T>::instance() {
    return inst;
}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::extractUUID(std::string &out) {
    out = uuid;
    return *this;
}

template<typename T>
template<typename I>
celix::ServiceDependency<T,I>& celix::ComponentManager<T>::addServiceDependency() {
    auto *dep = new celix::ServiceDependency<T,I>{ctx, [this]{updateState();}, [this]{return instance();}};
    std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
    serviceDependencies[dep->getUUD()] = std::unique_ptr<GenericServiceDependency>{dep};
    return *dep;
}

template<typename T>
template<typename I>
celix::ServiceDependency<T,I>* celix::ComponentManager<T>::findServiceDependency(const std::string& serviceDependencyUUID) {
    std::lock_guard<std::mutex> lck{serviceDependenciesMutex};
    auto it = serviceDependencies.find(serviceDependencyUUID);
    return it != serviceDependencies.end() ? nullptr : it->second.get();
}






template<typename T, typename I>
celix::ServiceDependency<T,I>::ServiceDependency(
        std::shared_ptr<celix::BundleContext> _ctx,
        std::function<void()> _stateChangedCallback,
        std::function<std::shared_ptr<T>()> _getCmpInstance) : GenericServiceDependency{std::move(_ctx), std::move(_stateChangedCallback)}, getCmpInstance{_getCmpInstance} {};

template<typename T, typename I>
void celix::ServiceDependency<T,I>::setEnabled(bool enable) {
    bool currentlyEnabled = isEnabled();
    std::vector<ServiceTracker> newTracker{};
    if (enable && !currentlyEnabled) {
        //enable
        ServiceTrackerOptions<I> opts{};
        opts.filter = this->filter;
        opts.setWithOwner = set;
        opts.addWithOwner = add;
        opts.removeWithOwner = rem;
        opts.updateWithOwner = [this](std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle *>> rankedServices) {
            if (this->update) {
                //TODO lock?
                this->update(std::move(rankedServices));
            }
            this->stateChangedCallback();
        };
        newTracker.emplace_back(ctx->trackServices(opts));
        std::lock_guard<std::mutex> lck{mutex};
        std::swap(tracker, newTracker);

    } else if (!enable and currentlyEnabled) {
        //disable
        std::lock_guard<std::mutex> lck{mutex};
        std::swap(tracker, newTracker/*empty*/);
    }

    //newTracker out of scope -> RAII -> for disable clear current tracker, for enable empty newTracker
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::enable() {
    setEnabled(true);
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::disable() {
    setEnabled(false);
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setFunctionCallbacks(
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> setArg,
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> addArg,
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> remArg,
        std::function<void(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle *>> rankedServices)> updateArg) {

    //TODO lock or disable?
    set = {};
    add = {};
    rem = {};
    update = {};

    if (setArg) {
        set = std::move(setArg);
    }
    if (addArg) {
        add = addArg;
    }
    if (remArg) {
        rem = remArg;
    }
    if (updateArg) {
        update = std::move(updateArg);
    }
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setCallbacks(void (T::*setfp)(std::shared_ptr<I>)) {
    std::lock_guard<std::mutex> lck{mutex};
    auto setFunc = [this, setfp](std::shared_ptr<I> svc, const celix::Properties &, const celix::IResourceBundle &) {
        (getCmpInstance()->instance->*setfp)(svc);
    };
    return setFunctionCallbacks(std::move(setFunc), {}, {}, {});
}


template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setCallbacks(void (T::*addfp)(std::shared_ptr<I>), void (T::*remfp)(std::shared_ptr<I>)) {
    std::lock_guard<std::mutex> lck{mutex};
    auto addFunc = [this, addfp](std::shared_ptr<I> svc, const celix::Properties &, const celix::IResourceBundle &) {
        (getCmpInstance()->instance->*addfp)(svc);
    };
    auto remFunc = [this, remfp](std::shared_ptr<I> svc, const celix::Properties &, const celix::IResourceBundle &) {
        (getCmpInstance()->instance->*remfp)(svc);
    };
    return setFunctionCallbacks({}, std::move(addFunc), std::move(remFunc), {});
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setFilter(const std::string &f) {
    std::lock_guard<std::mutex> lck{mutex};
    filter = f;
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setRequired(bool r) {
    std::lock_guard<std::mutex> lck{mutex};
    required = r;
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setCardinality(celix::Cardinality c) {
    std::lock_guard<std::mutex> lck{mutex};
    cardinality = c;
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::extractUUID(std::string& out) {
    out = uuid;
    return *this;
}


#endif //CXX_CELIX_COMPONENT_MANAGER_H
