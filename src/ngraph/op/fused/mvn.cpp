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
#include <algorithm>

#include "mvn.hpp"
#include "ngraph/op/add.hpp"
#include "ngraph/op/constant.hpp"
#include "ngraph/op/divide.hpp"
#include "ngraph/op/fused/mvn.hpp"
#include "ngraph/op/multiply.hpp"
#include "ngraph/op/sqrt.hpp"
#include "ngraph/op/subtract.hpp"
#include "ngraph/op/sum.hpp"
#include "ngraph/op/util/broadcasting.hpp"

using namespace std;
using namespace ngraph;

op::MVN::MVN(const std::shared_ptr<Node>& data,
             bool across_channels,
             bool normalize_variance,
             double eps)
    : FusedOp("MVN", {data})
    , m_across_channels{across_channels}
    , m_normalize_variance{normalize_variance}
    , m_eps{eps}
{
    constructor_validate_and_infer_types();
}

NodeVector op::MVN::decompose_op() const
{
    auto data = get_argument(0);
    auto data_shape = data->get_shape(); // assume that data have n and c channels.
    auto element_count = shape_size(data_shape) / data_shape[0];

    AxisSet reduction_axes;
    for (size_t i = 1; i < data_shape.size(); ++i)
    {
        if (!m_across_channels && i == 1)
        {
            element_count /= data_shape[i];
            continue;
        }
        reduction_axes.insert(i);
    }

    // calculate mean normalization
    auto sum = make_shared<ngraph::op::Sum>(data, reduction_axes);
    auto element_count_node = op::Constant::create(
        data->get_element_type(), sum->get_shape(), vector<size_t>{element_count});
    auto mean = sum / element_count_node;
    mean = legacy_style_broadcast_for_binary_operation(data, mean, 0).at(1);
    auto mean_normalization = data - mean;

    if (!m_normalize_variance)
    {
        return {mean_normalization};
    }
    else
    {
        // calculate variance
        auto variance = mean_normalization * mean_normalization;
        auto variance_sum = make_shared<ngraph::op::Sum>(variance, reduction_axes);
        auto variance_mean = variance_sum / element_count_node;
        variance = make_shared<op::Sqrt>(variance_mean);
        // add epsilon
        auto eps_node = op::Constant::create(
            data->get_element_type(), variance->get_shape(), vector<double>{m_eps});
        variance = variance + eps_node;
        variance =
            legacy_style_broadcast_for_binary_operation(mean_normalization, variance, 0).at(1);

        return {mean_normalization / variance};
    }
}

shared_ptr<Node> op::MVN::copy_with_new_args(const NodeVector& new_args) const
{
    if (new_args.size() != 1)
    {
        throw ngraph_error("Incorrect number of new arguments");
    }
    return make_shared<MVN>(new_args.at(0), m_across_channels, m_normalize_variance, m_eps);
}
