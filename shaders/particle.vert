#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 particlePos;
uniform float size;
uniform float Alpha;

void main()
{
    // Billboard: build camera-facing quad from aPos offsets
    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);
    vec3 worldPos = particlePos + camRight * aPos.x * size + camUp * aPos.y * size;
    TexCoord  = aTexCoord;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
