#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    DirLight dirLight;
    PointLight pointLight;
    SpotLight spotLight;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform PointLight pointLight;
uniform DirLight dirLight;
uniform SpotLight spotLight; 
uniform vec3 viewPos;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = vec2(aTexCoords.x, aTexCoords.y);

    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    mat3 TBN = transpose(mat3(T, B, N));

    vs_out.dirLight.ambient = dirLight.ambient;
    vs_out.dirLight.diffuse = dirLight.diffuse;
    vs_out.dirLight.specular = dirLight.specular;
    vs_out.dirLight.direction = TBN * dirLight.direction;

    
    vs_out.pointLight.position = TBN * pointLight.position;
    vs_out.pointLight.ambient = pointLight.ambient;
    vs_out.pointLight.diffuse = pointLight.diffuse;
    vs_out.pointLight.specular = pointLight.specular;
    vs_out.pointLight.constant = pointLight.constant;
    vs_out.pointLight.linear = pointLight.linear;
    vs_out.pointLight.quadratic = pointLight.quadratic;
    
    vs_out.spotLight.position = TBN * spotLight.position;
    vs_out.spotLight.direction = TBN * spotLight.direction;
    vs_out.spotLight.ambient = spotLight.ambient;
    vs_out.spotLight.diffuse = spotLight.diffuse;
    vs_out.spotLight.specular = spotLight.specular;
    vs_out.spotLight.constant = spotLight.constant;
    vs_out.spotLight.linear = spotLight.linear;
    vs_out.spotLight.quadratic = spotLight.quadratic;
    vs_out.spotLight.outerCutOff = spotLight.outerCutOff;
    vs_out.spotLight.cutOff = spotLight.cutOff;
    

    vs_out.TangentViewPos  = TBN * viewPos;
    vs_out.TangentFragPos  = TBN * vs_out.FragPos;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}