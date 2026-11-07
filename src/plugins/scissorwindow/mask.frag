// SPDX-FileCopyrightText: 2018-2023 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "saturation.glsl"
#include "colormanagement.glsl"

uniform sampler2D sampler;
uniform sampler2D maskTexture;
uniform vec4 modulation;
varying vec2 texcoord0;

void main()
{
    vec4 color = texture2D(sampler, texcoord0);
    color = sourceEncodingToNitsInDestinationColorspace(color);
    color = adjustSaturation(color);
    color *= modulation * texture2D(maskTexture, texcoord0).a;
    gl_FragColor = nitsToDestinationEncoding(color);
}
