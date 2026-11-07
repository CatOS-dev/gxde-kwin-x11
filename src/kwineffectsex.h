// SPDX-FileCopyrightText: 2021 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "effect/effectframe.h"
#include "effect/effecthandler.h"
#include "plugins/slideback/motionmanager.h"

namespace KWin
{

using EffectFrameEx = EffectFrame;
using EffectsHandlerEx = EffectsHandler;
using EffectScreen = Output;
using EffectWindowList = QList<EffectWindow *>;

extern KWIN_EXPORT EffectsHandler *effectsEx;

} // namespace KWin
