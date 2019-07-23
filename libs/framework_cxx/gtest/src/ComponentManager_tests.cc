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

#include <atomic>

#include "gtest/gtest.h"

#include "celix/Framework.h"
#include "celix/ComponentManager.h"

class ComponentManagerTest : public ::testing::Test {
public:
    celix::Framework& framework() { return fw; }
private:
    celix::Framework fw{};
};


TEST_F(ComponentManagerTest, CreateDestroy) {
    auto ctx = framework().context();

    class Cmp {};

    celix::ComponentManager<Cmp> cmpMng{ctx, std::make_shared<Cmp>()};
    EXPECT_FALSE(cmpMng.isEnabled());
    EXPECT_TRUE(cmpMng.isResolved()); //no deps -> resolved
    EXPECT_EQ(cmpMng.getState(), celix::ComponentState::Disabled);

    cmpMng.enable();
    EXPECT_TRUE(cmpMng.isEnabled());
}

TEST_F(ComponentManagerTest, AddSvcDep) {
    auto ctx = framework().context();

    class Cmp {};
    class ISvc {};

    celix::ComponentManager<Cmp> cmpMng{ctx, std::make_shared<Cmp>()};
    cmpMng.addServiceDependency<ISvc>()
            .setRequired(true);
    cmpMng.enable();
    EXPECT_TRUE(cmpMng.isEnabled());

    //dep not available -> cmp manager not resolved
    EXPECT_FALSE(cmpMng.isResolved());


    auto svcReg = ctx->registerService(std::make_shared<ISvc>());
    //dep available -> cmp manager resolved
    EXPECT_TRUE(cmpMng.isResolved());
}