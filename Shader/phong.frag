#version 330

uniform sampler2D envmap_diff;
uniform sampler2D envmap_spec;
uniform uint seed;
uniform uint outputID;

in vec3 v, n, t;
in vec3 ka, kd, ks;
in float s;
uniform sampler2D diffTex;

uniform vec3 eyePos;

out vec4 color;

#define PI 3.14159265358979

#define RAND_MAX 1000u

uint next_rand(uint seed)
{
    return seed * 15485863u + 32452843u;
}

float randf(uint seed) 
{
    return float(seed % RAND_MAX) / float(RAND_MAX-1u);
}

mat3 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
}

vec3 getColor(sampler2D map, vec3 dir) 
{
    uint local_seed = next_rand(seed);
    float delta_phi = randf(local_seed);
    local_seed = next_rand(local_seed);
    float delta_theta = randf(local_seed);
    local_seed = next_rand(local_seed);
    float angle = randf(local_seed)*2.0*PI;
    delta_phi *= PI/12.0;
    delta_theta *= 2.0*PI;

    dir = rotationMatrix(vec3(cos(angle), 0, sin(angle)), delta_phi) * dir;
    dir = rotationMatrix(vec3(0, 1, 0), delta_theta) * dir;
    
    
    float phi = acos(dir.y+delta_phi) / PI;
    float theta = (atan(dir.z, dir.x)+delta_theta) / (2*PI);
    return texture(map, vec2(theta, phi)).rgb;
}

void main()
{
    if(outputID == 1u)
    {
        color = vec4(v, 1);
        return;
    }
    vec3 in_vec = normalize(eyePos - v);
    vec3 out_vec = dot(n, in_vec) * n * 2 - in_vec;
    vec3 c = kd * getColor(envmap_diff, n) * texture(diffTex, t.xy).rgb;
    c += ka;
    //c += ks * getColor(envmap_spec, out_vec)*0.5;

    color = vec4(c, 1);
}
