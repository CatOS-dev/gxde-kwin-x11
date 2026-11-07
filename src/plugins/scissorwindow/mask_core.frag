// SPDX-FileCopyrightText: 2018-2023 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#version 140

#include "saturation.glsl"
#include "colormanagement.glsl"

uniform sampler2D sampler;
uniform sampler2D maskTexture;
uniform vec4 modulation;
in vec2 texcoord0;
out vec4 fragColor;

void main()
{
    vec4 color = texture(sampler, texcoord0);
    color = sourceEncodingToNitsInDestinationColorspace(color);
    color = adjustSaturation(color);
    color *= modulation * texture(maskTexture, texcoord0).a;
    fragColor = nitsToDestinationEncoding(color);
}
