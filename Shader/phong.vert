#version 330 core

// Input vertex data, different for all executions of this shader.
in vec3 vertex;
in vec3 normal;
in vec2 texCoord;

in vec3 diffuse;
in vec3 specular;
in vec3 ambient;
in float shiness;

uniform mat4 viewMat;
uniform mat4 projMat;

out vec3 v, n, t;
out vec3 ka, kd, ks;
out float s;

void main()
{
    gl_Position = projMat * viewMat * vec4(vertex, 1);
    v = vertex;
    n = normal;
    t = vec3(texCoord, 0);
    kd = diffuse;
    ka = ambient;
    ks = specular;
    s = shiness;
}

