#pragma once

using UpdateHook = std::function<void ()>;
void setUpdateHook(UpdateHook updateHook);
