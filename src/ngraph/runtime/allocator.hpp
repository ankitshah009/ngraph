//*****************************************************************************
// Copyright 2017-2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "ngraph/except.hpp"
#include "ngraph/util.hpp"

namespace ngraph
{
    namespace runtime
    {
        class Allocator;
    }
}
// Abstract class for the allocator, for allocating and deallocating device memory
class ngraph::runtime::Allocator
{
public:
    virtual ~Allocator() = default;
    virtual void* Malloc(void* handle, size_t size, size_t alignment);
    virtual void Free(void* handle, void* ptr);
};