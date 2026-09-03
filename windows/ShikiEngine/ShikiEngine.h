#pragma once

#include "pch.h"
#include "resource.h"

#if __has_include("codegen/NativeShikiEngineDataTypes.g.h")
  #include "codegen/NativeShikiEngineDataTypes.g.h"
#endif
// Note: The following lines use Mustache template syntax which will be processed during
// project generation to produce standard C++ code. If existing codegen spec files are found,
// use the actual filename; otherwise use conditional includes.
#if __has_include("codegen/NativeShikiEngineSpec.g.h")
  #include "codegen/NativeShikiEngineSpec.g.h"
#endif

#include "NativeModules.h"

namespace winrt::ShikiEngine
{

// See https://microsoft.github.io/react-native-windows/docs/native-platform for help writing native modules

REACT_MODULE(ShikiEngine)
struct ShikiEngine
{
  // Note: Mustache template syntax below will be processed during project generation
  // to produce standard C++ code based on detected codegen files.
#if __has_include("codegen/NativeShikiEngineSpec.g.h")
  using ModuleSpec = ShikiEngineCodegen::ShikiEngineSpec;
#endif

  REACT_INIT(Initialize)
  void Initialize(React::ReactContext const &reactContext) noexcept;

private:
  React::ReactContext m_context;
};

} // namespace winrt::ShikiEngine