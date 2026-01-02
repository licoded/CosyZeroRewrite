# CMake 架构

> 项目 CMake 构建系统的模块化设计

---

## 设计原则

**模块化**: CMake 配置按功能拆分，每个文件职责单一
**可维护**: 新增依赖只需修改对应模块
**清晰**: 主入口 (根 CMakeLists.txt) 保持简洁

---

## 目录结构

```
CMakeLists.txt          # 主入口 (保持最小化)
cmake/
  modules/              # FindXXX.cmake 外部依赖模块
    FindZ3.cmake        # Z3 SMT 求解器
    FindSpdlog.cmake    # spdlog 日志库 (可选)
    FindCatch2.cmake    # Catch2 测试框架
    # 添加新的 Find 模块到这里
  CompilerOptions.cmake # 编译器标志和设置
  Dependencies.cmake    # 依赖检测和配置
  LibraryTargets.cmake  # 库构建目标
  Tests.cmake           # 单元测试配置
  Fuzzing.cmake         # Fuzzing 目标 (libFuzzer)
  Installation.cmake    # 安装规则
```

---

## .gitignore 规则

CMake 生成的文件与源码 CMake 模块分离：

```gitignore
# 排除根目录生成的 CMake 文件
/*.cmake
/CMakeCache.txt
/CMakeFiles/

# 保留源码 CMake 模块
!cmake/**/*.cmake

# 排除构建目录中的生成文件
/**/CMakeFiles/*.cmake
/**/cmake_install.cmake
/**/CTestTestfile.cmake
```

---

## 添加新依赖

### 步骤

1. **创建 Find 模块**:
   ```cmake
   # cmake/modules/Find<Package>.cmmake
   include(FindPackageHandleStandardArgs)

   find_path(<Package>_INCLUDE_DIR ...)
   find_library(<Package>_LIBRARY ...)

   find_package_handle_standard_args(<Package>
     REQUIRED_VARS <Package>_INCLUDE_DIR <Package>_LIBRARY
   )

   if(NOT TARGET <Package>::<Package>)
     add_library(<Package>::<Package> UNKNOWN IMPORTED)
     set_target_properties(<Package>::<Package> PROPERTIES
       IMPORTED_LOCATION "${<Package>_LIBRARY}"
       INTERFACE_INCLUDE_DIRECTORIES "${<Package>_INCLUDE_DIR}"
     )
   endif()
   ```

2. **更新依赖检测** (`cmake/Dependencies.cmake`):
   ```cmake
   find_package(<Package> REQUIRED)  # 或 OPTIONAL
   if(<Package>_FOUND)
     list(APPEND PROJECT_DEPENDENCIES <Package>::<Package>)
   endif()
   ```

3. **更新 .gitignore** (如有生成文件)

---

## 现有模块说明

### CompilerOptions.cmake

```cmake
# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译器警告
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()
```

### Dependencies.cmake

```cmake
# 必需依赖
find_package(Z3 REQUIRED)

# 可选依赖
find_package(spdlog QUIET)
if(spdlog_FOUND)
  target_compile_definitions(formula PRIVATE USE_SPDLOG)
endif()
```

### Tests.cmake

```cmake
# 测试可执行文件
add_executable(formula_tests ...)
target_link_libraries(formula_tests PRIVATE formula catch2)

# 注册到 CTest
add_test(NAME formula_tests COMMAND formula_tests)
```

---

## 构建变量

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTING` | ON | 构建测试 |
| `BUILD_FUZZING` | OFF | 构建 fuzzing 目标 |
| `USE_SPDLOG` | AUTO | 使用 spdlog (如检测到) |
| `CMAKE_BUILD_TYPE` | Debug | 构建类型 |

---

## 常用命令

```bash
# 标准构建
mkdir build && cd build
cmake ..
make -j$(nproc)

# 指定构建类型
cmake -DCMAKE_BUILD_TYPE=Release ..

# 启用 fuzzing
cmake -DBUILD_FUZZING=ON ..

# 运行测试
make test
# 或
ctest --output-on-failure
```
