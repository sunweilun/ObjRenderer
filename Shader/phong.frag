#version 330

uniform sampler2D envmap_diff;
uniform sampler2D envmap_spec;

in vec3 v, n, t;
in vec3 ka, kd, ks;
in float s;

uniform vec3 eyePos;

out vec3 color;

#define pi 3.14159265358979

vec3 getColor(sampler2D map, vec3 dir) 
{
    float phi = acos(dir.y) / pi;
    float theta = (atan(dir.z, dir.x)+pi) / (2*pi);
    return texture(map, vec2(theta, phi)).rgb;
}

void main()
{
    vec3 in_vec = normalize(eyePos - v);
    vec3 out_vec = dot(n, in_vec) * n * 2 - in_vec;
    vec3 c = kd * getColor(envmap_diff, n) + ka;
    color = c + ks * getColor(envmap_spec, out_vec);
}
