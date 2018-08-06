/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */
#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <vector>
#include "paddle/fluid/framework/lod_tensor.h"
#include "paddle/fluid/framework/op_registry.h"

namespace paddle {
namespace operators {

using Tensor = framework::Tensor;

template <typename DeviceContext, typename T>
class SamplingIdKernel : public framework::OpKernel<T> {
 public:
  void Compute(const framework::ExecutionContext& context) const override {
    const Tensor* input = context.Input<Tensor>("X");
    const int batch_size = static_cast<int>(input->dims()[0]);
    const int width = static_cast<int>(input->dims()[1]);

    std::vector<T> ins_vector;
    framework::TensorToVector(*input, context.device_context(), &ins_vector);

    std::vector<T> ids(batch_size);
    for (size_t i = 0; i < batch_size; ++i) {
      double r = this->getRandReal();
      int idx = width - 1;
      for (int j = 0; j < width; ++j) {
        if ((r -= ins_vector[i * width + j]) < 0) {
          idx = j;
          break;
        }
      }
      ids[i] = ins_vector[i * width + idx];
    }

    std::vector<int64_t> out_dim;
    out_dim.push_back(static_cast<int64_t>(batch_size));

    Tensor* output = context.Output<Tensor>("Out");
    output->Resize(framework::make_ddim(out_dim));
    output->mutable_data<T>(context.GetPlace());
    framework::TensorFromVector(ids, context.device_context(), output);
  }

 private:
  double getRandReal() const {
    std::call_once(init_flag_, &SamplingIdKernel::getRndInstance);
    return rnd();
  }

  static void getRndInstance() {
    // Will be used to obtain a seed for the random number engine
    std::random_device rd;
    // Standard mersenne_twister_engine seeded with rd()
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    rnd = std::bind(dis, std::ref(gen));
  }

  static std::once_flag init_flag_;
  static std::function<double()> rnd;
};
}  // namespace operators
}  // namespace paddle
