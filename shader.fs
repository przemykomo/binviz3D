#version 330 core

in vec2 fragTexCoord;

out vec4 fragColor;

void main()
{
    fragColor = vec4(fragTexCoord, fragTexCoord.r, 1.0);
}
