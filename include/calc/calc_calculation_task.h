/**
 * PON Calculator https://github.com/qoala101/ponc
 * @author Volodymyr Hromakov (4y5t6r@gmail.com)
 * @copyright Copyright (c) 2023, MIT License
 */

#ifndef VH_PONC_CALC_CALCULATION_TASK_H_
#define VH_PONC_CALC_CALCULATION_TASK_H_

#include <future>

#include "calc_calculator.h"
#include "calc_tree_node.h"

namespace vh::ponc::calc {
///
class CalculationTask {
 public:
  ///
  explicit CalculationTask(Calculator::ConstructorArgs args);

  ///
  CalculationTask(const CalculationTask &) = delete;
  ///
  CalculationTask(CalculationTask &&) noexcept = delete;

  ///
  auto operator=(const CalculationTask &) -> CalculationTask & = delete;
  ///
  auto operator=(CalculationTask &&) noexcept -> CalculationTask & = delete;

  ///
  ~CalculationTask();

  ///
  void Stop();
  ///
  auto IsRunning() const -> bool;
  ///
  auto GetProgress() const -> float;
  ///
  auto GetResult() -> std::optional<std::vector<calc::TreeNode>>;

 private:
  ///
  auto OnCalculationStep(const calc::Calculator &calculator)
      -> calc::Calculator::StepStatus;

  ///
  std::future<std::vector<calc::TreeNode>> task_{};
  ///
  std::atomic<bool> stop_requested_{};
  ///
  std::atomic<float> progress_{};
};
}  // namespace vh::ponc::calc

#endif  // VH_PONC_CALC_CALCULATION_TASK_H_
