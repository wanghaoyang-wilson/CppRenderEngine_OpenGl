// ============================================================
//  PyRecT3D — src/core/app_registry.h（内部头，不对外）
//  APP::run / APP::pump 的驱动钩子注册点。
//  显式 API 与隐式 API 各自注册自己的帧推进实现。
// ============================================================
#pragma once
#include <functional>

namespace pyrect3d {
namespace detail {

// 注册每帧推进函数：返回 false 表示请求退出
void setAppPumpHook(std::function<bool()> hook);
std::function<bool()>& appPumpHook();

// 全局退出标志
void setAppQuitFlag(bool v);
bool appQuitRequested();

} // namespace detail
} // namespace pyrect3d
