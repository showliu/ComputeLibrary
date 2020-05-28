/*
 * Copyright (c) 2018-2020 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef ARM_COMPUTE_CLLSTMLAYER_H
#define ARM_COMPUTE_CLLSTMLAYER_H

#include "arm_compute/runtime/IFunction.h"

#include "arm_compute/core/CL/kernels/CLActivationLayerKernel.h"
#include "arm_compute/core/CL/kernels/CLCopyKernel.h"
#include "arm_compute/core/CL/kernels/CLElementwiseOperationKernel.h"
#include "arm_compute/core/CL/kernels/CLMemsetKernel.h"
#include "arm_compute/core/CL/kernels/CLPixelWiseMultiplicationKernel.h"
#include "arm_compute/core/CL/kernels/CLWidthConcatenate2TensorsKernel.h"
#include "arm_compute/core/Types.h"
#include "arm_compute/runtime/CL/CLTensor.h"
#include "arm_compute/runtime/CL/functions/CLConcatenateLayer.h"
#include "arm_compute/runtime/CL/functions/CLElementwiseOperations.h"
#include "arm_compute/runtime/CL/functions/CLFullyConnectedLayer.h"
#include "arm_compute/runtime/CL/functions/CLGEMM.h"
#include "arm_compute/runtime/CL/functions/CLMeanStdDevNormalizationLayer.h"
#include "arm_compute/runtime/IMemoryManager.h"
#include "arm_compute/runtime/MemoryGroup.h"
#include "arm_compute/runtime/common/LSTMParams.h"

#include <memory>

namespace arm_compute
{
class ICLTensor;

/** This function performs a single time step in a Long Short-Term Memory (LSTM) layer.
 *
 */
class CLLSTMLayer : public IFunction
{
public:
    /** Default constructor */
    CLLSTMLayer(std::shared_ptr<IMemoryManager> memory_manager = nullptr);
    /** Initialize function's tensors.
     *
     * @param[in]  input                       Source tensor. Input is a 2D tensor with dimensions [input_size, batch_size]. Data types supported: F16/F32.
     * @param[in]  input_to_forget_weights     2D weights tensor with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  input_to_cell_weights       2D weights tensor with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  input_to_output_weights     2D weights tensor with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  recurrent_to_forget_weights 2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  recurrent_to_cell_weights   2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  recurrent_to_output_weights 2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  forget_gate_bias            1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in]  cell_bias                   1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in]  output_gate_bias            1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in]  output_state_in             2D weights tensor with dimensions [output_size, batch_size]. Data type supported: Same as @p input.
     * @param[in]  cell_state_in               2D tensor with dimensions [num_units, batch_size]. Data type supported: Same as @p input.
     * @param[out] scratch_buffer              2D tensor with dimensions [num_units * 4, batch_size] with CIFG or [num_units * 3, batch_size] without CIGF. Data type supported: Same as @p input.
     * @param[out] output_state_out            2D weights tensor with dimensions [output_size, batch_size]. Data type supported: Same as @p input.
     * @param[out] cell_state_out              2D tensor with dimensions [num_units, batch_size]. Data type supported: Same as @p input.
     * @param[out] output                      Destination tensor. Output is a 2D tensor with dimensions [output_size, batch_size].
     *                                         Data types supported: Same as @p input.
     * @param[in]  lstm_params                 Weights tensors used in peephole optimization:
     *                                         input_to_input_weights     2D weights tensor with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     *                                         recurrent_to_input_weights 2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     *                                         cell_to_input_weights      1D weights tensor with dimensions [num_units]. Can be nullptr. Data type supported: Same as @p input.
     *                                         cell_to_forget_weights     1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         cell_to_output_weights     1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         input_gate_bias            1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input
     *                                         projection_weights         2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     *                                         projection_bias            1D weights tensor with dimensions [output_size]. Data type supported: Same as @p input.
     *                                         input_layer_norm_weights   1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         forget_layer_norm_weights  1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         cell_layer_norm_weights    1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         output_layer_norm_weights  1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in]  activation_info             Contains activation information described in @ref ActivationLayerInfo.
     * @param[in]  cell_threshold              (Optional) The clipping threshold for the cell state, such that values are bound within [-cell_clip, cell_clip].
     *                                         If set to 0.0f then clipping is disabled.
     * @param[in]  projection_threshold        (Optional) The clipping threshold for the output from the projection layer, such that values are bound within [-proj_clip, proj_clip].
     *                                         If set to 0.0f then clipping is disabled.
     */
    void configure(const ICLTensor *input,
                   const ICLTensor *input_to_forget_weights, const ICLTensor *input_to_cell_weights, const ICLTensor *input_to_output_weights,
                   const ICLTensor *recurrent_to_forget_weights, const ICLTensor *recurrent_to_cell_weights, const ICLTensor *recurrent_to_output_weights,
                   const ICLTensor *forget_gate_bias, const ICLTensor *cell_bias, const ICLTensor *output_gate_bias,
                   const ICLTensor *output_state_in, const ICLTensor *cell_state_in,
                   ICLTensor *scratch_buffer, ICLTensor *output_state_out, ICLTensor *cell_state_out, ICLTensor *output,
                   const LSTMParams<ICLTensor> &lstm_params, const ActivationLayerInfo &activation_info, float cell_threshold = 0.f, float projection_threshold = 0.f);
    /** Initialize function's tensors.
     *
     * @param[in]  compile_context             The compile context to be used.
     * @param[in]  input                       Source tensor. Input is a 2D tensor with dimensions [input_size, batch_size]. Data types supported: F16/F32.
     * @param[in]  input_to_forget_weights     2D weights tensor with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  input_to_cell_weights       2D weights tensor with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  input_to_output_weights     2D weights tensor with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  recurrent_to_forget_weights 2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  recurrent_to_cell_weights   2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  recurrent_to_output_weights 2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     * @param[in]  forget_gate_bias            1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in]  cell_bias                   1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in]  output_gate_bias            1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in]  output_state_in             2D weights tensor with dimensions [output_size, batch_size]. Data type supported: Same as @p input.
     * @param[in]  cell_state_in               2D tensor with dimensions [num_units, batch_size]. Data type supported: Same as @p input.
     * @param[out] scratch_buffer              2D tensor with dimensions [num_units * 4, batch_size] with CIFG or [num_units * 3, batch_size] without CIGF. Data type supported: Same as @p input.
     * @param[out] output_state_out            2D weights tensor with dimensions [output_size, batch_size]. Data type supported: Same as @p input.
     * @param[out] cell_state_out              2D tensor with dimensions [num_units, batch_size]. Data type supported: Same as @p input.
     * @param[out] output                      Destination tensor. Output is a 2D tensor with dimensions [output_size, batch_size].
     *                                         Data types supported: Same as @p input.
     * @param[in]  lstm_params                 Weights tensors used in peephole optimization:
     *                                         input_to_input_weights     2D weights tensor with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     *                                         recurrent_to_input_weights 2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     *                                         cell_to_input_weights      1D weights tensor with dimensions [num_units]. Can be nullptr. Data type supported: Same as @p input.
     *                                         cell_to_forget_weights     1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         cell_to_output_weights     1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         input_gate_bias            1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input
     *                                         projection_weights         2D weights tensor with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     *                                         projection_bias            1D weights tensor with dimensions [output_size]. Data type supported: Same as @p input.
     *                                         input_layer_norm_weights   1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         forget_layer_norm_weights  1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         cell_layer_norm_weights    1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     *                                         output_layer_norm_weights  1D weights tensor with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in]  activation_info             Contains activation information described in @ref ActivationLayerInfo.
     * @param[in]  cell_threshold              (Optional) The clipping threshold for the cell state, such that values are bound within [-cell_clip, cell_clip].
     *                                         If set to 0.0f then clipping is disabled.
     * @param[in]  projection_threshold        (Optional) The clipping threshold for the output from the projection layer, such that values are bound within [-proj_clip, proj_clip].
     *                                         If set to 0.0f then clipping is disabled.
     */
    void configure(const CLCompileContext &compile_context, const ICLTensor *input,
                   const ICLTensor *input_to_forget_weights, const ICLTensor *input_to_cell_weights, const ICLTensor *input_to_output_weights,
                   const ICLTensor *recurrent_to_forget_weights, const ICLTensor *recurrent_to_cell_weights, const ICLTensor *recurrent_to_output_weights,
                   const ICLTensor *forget_gate_bias, const ICLTensor *cell_bias, const ICLTensor *output_gate_bias,
                   const ICLTensor *output_state_in, const ICLTensor *cell_state_in,
                   ICLTensor *scratch_buffer, ICLTensor *output_state_out, ICLTensor *cell_state_out, ICLTensor *output,
                   const LSTMParams<ICLTensor> &lstm_params, const ActivationLayerInfo &activation_info, float cell_threshold = 0.f, float projection_threshold = 0.f);

    /** Static function to check if given info will lead to a valid configuration of @ref CLLSTMLayer
     *
     * @param[in] input                       Source tensor info. Input is a 2D tensor with dimensions [input_size, batch_size]. Data types supported: F16/F32.
     * @param[in] input_to_forget_weights     2D weights tensor info with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     * @param[in] input_to_cell_weights       2D weights tensor info with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     * @param[in] input_to_output_weights     2D weights tensor info with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     * @param[in] recurrent_to_forget_weights 2D weights tensor info with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     * @param[in] recurrent_to_cell_weights   2D weights tensor info with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     * @param[in] recurrent_to_output_weights 2D weights tensor info with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     * @param[in] forget_gate_bias            1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in] cell_bias                   1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in] output_gate_bias            1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in] output_state_in             2D weights tensor info with dimensions [output_size, batch_size]. Data type supported: Same as @p input.
     * @param[in] cell_state_in               2D tensor info with dimensions [num_units, batch_size]. Data type supported: Same as @p input.
     * @param[in] scratch_buffer              2D tensor info with dimensions [num_units * 4, batch_size] with CIFG or [num_units * 3, batch_size] without CIGF.
     *                                        Data type supported: Same as @p input.
     * @param[in] output_state_out            2D weights tensor info with dimensions [output_size, batch_size]. Data type supported: Same as @p input.
     * @param[in] cell_state_out              2D tensor info with dimensions [num_units, batch_size]. Data type supported: Same as @p input.
     * @param[in] output                      Destination tensor info. Output is a 2D tensor with dimensions [output_size, batch_size]. Data types supported: Same as @p input.
     * @param[in] lstm_params                 Weights tensors info used in peephole optimization:
     *                                        input_to_input_weights     2D weights tensor info with dimensions [input_size, num_units]. Data type supported: Same as @p input.
     *                                        recurrent_to_input_weights 2D weights tensor info with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     *                                        cell_to_input_weights      1D weights tensor info with dimensions [num_units]. Can be nullptr. Data type supported: Same as @p input.
     *                                        cell_to_forget_weights     1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input.
     *                                        cell_to_output_weights     1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input.
     *                                        input_gate_bias            1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input
     *                                        projection_weights         2D weights tensor info with dimensions [output_size, num_units]. Data type supported: Same as @p input.
     *                                        projection_bias            1D weights tensor info with dimensions [output_size]. Data type supported: Same as @p input.
     *                                        input_layer_norm_weights   1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input.
     *                                        forget_layer_norm_weights  1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input.
     *                                        cell_layer_norm_weights    1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input.
     *                                        output_layer_norm_weights  1D weights tensor info with dimensions [num_units]. Data type supported: Same as @p input.
     * @param[in] activation_info             Contains activation information described in @ref ActivationLayerInfo.
     * @param[in] cell_threshold              (Optional) The clipping threshold for the cell state, such that values are bound within [-cell_clip, cell_clip].
     *                                        If set to 0.0f then clipping is disabled.
     * @param[in] projection_threshold        (Optional) The clipping threshold for the output from the projection layer, such that values are bound within [-proj_clip, proj_clip].
     *                                        If set to 0.0f then clipping is disabled.
     *
     * @return a status
     */
    static Status validate(const ITensorInfo *input,
                           const ITensorInfo *input_to_forget_weights, const ITensorInfo *input_to_cell_weights, const ITensorInfo *input_to_output_weights,
                           const ITensorInfo *recurrent_to_forget_weights, const ITensorInfo *recurrent_to_cell_weights, const ITensorInfo *recurrent_to_output_weights,
                           const ITensorInfo *forget_gate_bias, const ITensorInfo *cell_bias, const ITensorInfo *output_gate_bias,
                           const ITensorInfo *output_state_in, const ITensorInfo *cell_state_in,
                           const ITensorInfo *scratch_buffer, const ITensorInfo *output_state_out, const ITensorInfo *cell_state_out, const ITensorInfo *output,
                           const LSTMParams<ITensorInfo> &lstm_params, const ActivationLayerInfo &activation_info, float cell_threshold = 0.f, float projection_threshold = 0.f);

    // Inherited methods overridden:
    void run() override;
    void prepare() override;

private:
    MemoryGroup                          _memory_group;
    CLFullyConnectedLayer                _fully_connected_input_gate;
    CLArithmeticAddition                 _accum_input_gate1;
    CLSaturatedArithmeticOperationKernel _subtract_input_gate;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_input_gate;
    CLActivationLayerKernel              _activation_input_gate;
    CLFullyConnectedLayer                _fully_connected_forget_gate;
    CLArithmeticAddition                 _accum_forget_gate1;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_forget_gate;
    CLActivationLayerKernel              _activation_forget_gate;
    CLFullyConnectedLayer                _fully_connected_cell_state;
    CLGEMM                               _gemm_cell_state1;
    CLTransposeKernel                    _transpose_cell_state;
    CLSaturatedArithmeticOperationKernel _accum_cell_state1;
    CLSaturatedArithmeticOperationKernel _accum_cell_state2;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_cell_state1;
    CLActivationLayerKernel              _activation_cell_state;
    CLActivationLayerKernel              _cell_clip;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_cell_state2;
    CLFullyConnectedLayer                _fully_connected_output;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_output_state1;
    CLArithmeticAddition                 _accum_output1;
    CLActivationLayerKernel              _activation_output;
    CLActivationLayerKernel              _activation_output_state;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_output_state2;
    CLFullyConnectedLayer                _fully_connected_output_state;
    CLActivationLayerKernel              _projection_clip;
    CLCopyKernel                         _copy_cell_state;
    CLCopyKernel                         _copy_output;
    CLConcatenateLayer                   _concat_scratch_buffer;
    CLWidthConcatenate2TensorsKernel     _concat_inputs_forget_gate;
    CLWidthConcatenate2TensorsKernel     _concat_weights_forget_gate;
    CLWidthConcatenate2TensorsKernel     _concat_weights_input_gate;
    CLWidthConcatenate2TensorsKernel     _concat_weights_output;
    CLMemsetKernel                       _ones_memset_kernel;
    CLMeanStdDevNormalizationLayer       _mean_std_norm_input_gate;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_input_gate_coeff;
    CLSaturatedArithmeticOperationKernel _accum_input_gate_bias;
    CLMeanStdDevNormalizationLayer       _mean_std_norm_forget_gate;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_forget_gate_coeff;
    CLSaturatedArithmeticOperationKernel _accum_forget_gate_bias;
    CLMeanStdDevNormalizationLayer       _mean_std_norm_cell_gate;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_cell_gate_coeff;
    CLSaturatedArithmeticOperationKernel _accum_cell_gate_bias;
    CLMeanStdDevNormalizationLayer       _mean_std_norm_output_gate;
    CLPixelWiseMultiplicationKernel      _pixelwise_mul_output_gate_coeff;
    CLSaturatedArithmeticOperationKernel _accum_output_gate_bias;
    CLTensor                             _input_gate_out1;
    CLTensor                             _input_gate_out2;
    CLTensor                             _input_gate_out3;
    CLTensor                             _input_gate_out4;
    CLTensor                             _forget_gate_out1;
    CLTensor                             _forget_gate_out2;
    CLTensor                             _forget_gate_out3;
    CLTensor                             _forget_gate_out4;
    CLTensor                             _forget_gate_out5;
    CLTensor                             _forget_gate_out6;
    CLTensor                             _cell_state_out1;
    CLTensor                             _cell_state_out2;
    CLTensor                             _cell_state_out3;
    CLTensor                             _cell_state_out4;
    CLTensor                             _cell_state_out5;
    CLTensor                             _output1;
    CLTensor                             _output2;
    CLTensor                             _output3;
    CLTensor                             _output4;
    CLTensor                             _cell_state_activation;
    CLTensor                             _output_state1;
    CLTensor                             _ones;
    CLTensor                             _input_layer_norm_out1;
    CLTensor                             _input_layer_norm_out2;
    CLTensor                             _forget_layer_norm_out1;
    CLTensor                             _forget_layer_norm_out2;
    CLTensor                             _cell_layer_norm_out1;
    CLTensor                             _cell_layer_norm_out2;
    CLTensor                             _output_layer_norm_out1;
    CLTensor                             _output_layer_norm_out2;
    bool                                 _run_peephole_opt;
    bool                                 _run_cifg_opt;
    bool                                 _perform_cell_clipping;
    bool                                 _has_projection_weights;
    bool                                 _perform_projection_clipping;
    bool                                 _is_prepared;
    bool                                 _is_layer_norm_lstm;
};
} // namespace arm_compute
#endif /* ARM_COMPUTE_CLLSTMLAYER_H */
