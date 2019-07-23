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

        //TODO make friend of SvcDep
        void updateState();
    protected:
        GenericComponentManager(std::shared_ptr<BundleContext> _ctx) : ctx{std::move(_ctx)} {};

        void removeServiceDependency(long serviceDependencyId);
        void setEnabled(bool enable);


        /**** Fields ****/
        std::shared_ptr<BundleContext> ctx;

        mutable std::mutex mutex{}; //protects below
        ComponentState state = ComponentState::Disabled;
        bool enabled = false;
        std::function<void()> init{};
        std::function<void()> start{};
        std::function<void()> stop{};
        std::function<void()> deinit{};

        long nextServiceDependencyId = 1L;
        std::unordered_map<long,std::unique_ptr<GenericServiceDependency>> serviceDependencies{};
    };

    template<typename T>
    class ComponentManager : public GenericComponentManager {
    public:
        using ComponentType = T;

        ComponentManager(std::shared_ptr<BundleContext> ctx, std::shared_ptr<T> cmpInstance);
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
        void setEnable(bool enable);

    protected:
        GenericServiceDependency(
                std::shared_ptr<BundleContext> ctx,
                std::function<void()> stateChangedCallback,
                std::function<void()> enable,
                std::function<void()> disable);

        //Fields
        std::shared_ptr<BundleContext> ctx;
        const std::function<void()> stateChangedCallback;
        const std::function<void()> enable;
        std::function<void()> disable;

        mutable std::mutex mutex{}; //protects below
        bool required = false;
        std::string filter{};
        Cardinality cardinality = Cardinality::One;
        std::vector<ServiceTracker> tracker{}; //max 1
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

        ServiceDependency<T,I>& enable();
        ServiceDependency<T,I>& disable();
        bool isResolved();
    private:
        const std::function<std::shared_ptr<T>()> getCmpInstance;


        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> set{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> add{};
        std::function<void(std::shared_ptr<I> svc, const celix::Properties &props, const celix::IResourceBundle &owner)> rem{};
        std::function<void(std::vector<std::tuple<std::shared_ptr<I>, const celix::Properties*, const celix::IResourceBundle *>> rankedServices)> update{};
    };
}



/**************************** IMPLEMENTATION *************************************************************************/

template<typename T>
celix::ComponentManager<T>::ComponentManager(
        std::shared_ptr<BundleContext> ctx,
        std::shared_ptr<T> cmpInstance) : GenericComponentManager{ctx}, inst{std::move(cmpInstance)} {}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::enable() {
    this->setEnabled(true);
    return *this;
}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::disable() {
    this->setEnabled(false);
    return *this;
}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::setCallbacks(
        void (T::*memberInit)(),
        void (T::*memberStart)(),
        void (T::*memberStop)(),
        void (T::*memberDeinit)()) {
    std::lock_guard<std::mutex> lck{mutex};
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
template<typename I>
celix::ServiceDependency<T,I>& celix::ComponentManager<T>::addServiceDependency() {
    auto *dep = new celix::ServiceDependency<T,I>{ctx, []{/*TODO*/}, [this]{return instance();}};
    std::lock_guard<std::mutex> lck{mutex};
    serviceDependencies[nextServiceDependencyId++] = std::unique_ptr<GenericServiceDependency>{dep};
    return *dep;
}








template<typename T, typename I>
celix::ServiceDependency<T,I>::ServiceDependency(
        std::shared_ptr<celix::BundleContext> _ctx,
        std::function<void()> _stateChangedCallback,
        std::function<std::shared_ptr<T>()> _getCmpInstance) : GenericServiceDependency{_ctx, _stateChangedCallback, [this]{enable();}, [this]{disable();}}, getCmpInstance{_getCmpInstance} {};

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::enable() {
    std::lock_guard<std::mutex> lck{mutex};
    if (tracker.size() == 0) { //disabled
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
        this->tracker.push_back(this->ctx->trackServices(opts));
    }
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::disable() {
    std::lock_guard<std::mutex> lck{mutex};
    tracker.clear();
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


#endif //CXX_CELIX_COMPONENT_MANAGER_H
