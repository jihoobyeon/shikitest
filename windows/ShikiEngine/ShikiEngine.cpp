#include "pch.h"

#include "ShikiEngine.h"

namespace winrt::ShikiEngine
{

// See https://microsoft.github.io/react-native-windows/docs/native-platform for help writing native modules

void ShikiEngine::Initialize(React::ReactContext const &reactContext) noexcept {
  m_context = reactContext;
}

} // namespace winrt::ShikiEngine