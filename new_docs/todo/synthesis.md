# Synthesis TODO

## 🔴 高优先级

### BDD 符号化优化

进一步优化 BDD Manager，提升 Safe System Move 效率

**相关代码**: `include/synthesis/bdd_manager.hpp`, `src/synthesis/bdd_manager.cpp`

## 🟡 中优先级

### 性能优化

优化性能瓶颈，支持更大规模公式

### 集成测试

添加更多端到端集成测试，测试完整的 synthesis 流程

## 🟢 低优先级

### 策略提取

从 SCC 分类结果中提取获胜策略，支持导出 (dot/json)

**预计时间**: 8 小时
