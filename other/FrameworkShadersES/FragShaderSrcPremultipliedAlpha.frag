#version 100
precision mediump float;

varying vec2 v_texCoord;
uniform sampler2D s_texture0;
uniform vec4 u_baseColor;
uniform vec4 u_multiplyColor;
uniform vec4 u_screenColor;

void main()
{
    vec4 texColor = texture2D(s_texture0, v_texCoord);
    texColor.rgb = texColor.rgb * u_multiplyColor.rgb;
    texColor.rgb = (texColor.rgb + u_screenColor.rgb * texColor.a) - (texColor.rgb * u_screenColor.rgb);
    gl_FragColor = texColor * u_baseColor;
}