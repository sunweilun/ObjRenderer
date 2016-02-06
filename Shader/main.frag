#version 330

uniform uint outputID;

out vec4 color;

vec4 shadePhong();
vec4 shadeCoord();

void main()
{
    switch(outputID)
    {
	case 0u:
        color = shadeCoord();
		return;
	case 1u:
		color = shadePhong();
		return;
    }
}
