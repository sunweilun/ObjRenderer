#version 330

uniform sampler2D envmap;
uniform sampler2D diffTex;
uniform uint seed;
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;

in vec3 v, n, t;

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

vec2 wrapTexdiff(vec2 diff)
{
	vec2 result = diff;
	if(diff.x > 0.5) result.x = 1.0-diff.x;
	if(diff.y > 0.5) result.y = 1.0-diff.y;
	return result;
}

vec3 getColor(sampler2D map, vec3 dir, float s = 1.0) 
{
    uint local_seed = next_rand(seed);
    float delta_phi = randf(local_seed);
    local_seed = next_rand(local_seed);
    float delta_theta = randf(local_seed);
    local_seed = next_rand(local_seed);
    float angle = randf(local_seed)*2.0*PI;
    delta_phi *= PI/6.0;
    delta_theta *= 2.0*PI;

    dir = rotationMatrix(vec3(cos(angle), 0, sin(angle)), delta_phi) * dir;
    dir = rotationMatrix(vec3(0, 1, 0), delta_theta) * dir;

    vec2 uv;
    float theta = atan(dir.z, dir.x)+PI;
    float phi = acos(clamp(dir.y, -1, 1));
    uv.x = theta / (2*PI);
    uv.y = phi / PI;
   
    float delta_shininess = acos(clamp(pow(0.9, 1/s), -1, 1))/PI*2.0;

    float drdx = length(dFdx(dir));
    float drdy = length(dFdy(dir));
    float dr = max(drdx, drdy);
    float da = asin(dr*0.5)*2.0/PI+delta_shininess;
    float d_theta = da/max(sin(phi), da); 
    return textureGrad(map, vec2(uv.x, (uv.y+1.0)/3.0), 
        vec2(d_theta, 0.0), vec2(0.0, da)).rgb;
}

vec4 shadePhong()
{
    vec3 in_vec = normalize(eyePos - v);
    vec3 out_vec = dot(n, in_vec) * n * 2 - in_vec;
    vec3 c = kd * getColor(envmap, normalize(n));
    c *= texture(diffTex, t.xy).rgb;
    c += ka;
    c += ks * getColor(envmap, out_vec, s);
    return vec4(c, 1);
}
