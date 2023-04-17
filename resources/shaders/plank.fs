#version 330 core
out vec4 FragColor;


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

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    DirLight dirLight;
    PointLight pointLight;
    SpotLight spotLight;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

uniform sampler2D diffuse_map;
uniform sampler2D normal_map;
uniform sampler2D height_map;
uniform sampler2D specular_map;
uniform float shininess;
uniform float heightScale;
uniform int flag;


vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    float height =  texture(height_map, texCoords).r;
    return texCoords - viewDir.xy * (height * heightScale);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 texCoords);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 texCoords);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 texCoords);

void main()
{
    // offset texture coordinates with Parallax Mapping
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = fs_in.TexCoords;

    texCoords = ParallaxMapping(texCoords,  viewDir);

    // obtain normal from normal map
    vec3 normal = texture(normal_map, texCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    vec3 result = vec3(0.0);
    if (flag == 1){
        result += CalcDirLight(fs_in.dirLight, normal, viewDir, texCoords);
    }else if (flag == 2){
        result += CalcPointLight(fs_in.pointLight, normal, fs_in.TangentFragPos, viewDir, texCoords);
    }else if (flag == 3){
        result += CalcSpotLight(fs_in.spotLight, normal, fs_in.TangentFragPos, viewDir, texCoords);
    }

    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec2 texCoords)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(diffuse_map, texCoords).rgb);
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse_map, texCoords).rgb);
    vec3 specular = light.specular * spec * texture(specular_map, texCoords).yyy;
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 texCoords)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * vec3(texture(diffuse_map, texCoords).rgb);
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse_map, texCoords).rgb);
    vec3 specular = light.specular * spec * texture(specular_map, texCoords).yyy;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec2 texCoords)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(diffuse_map, texCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse_map, texCoords));
    vec3 specular = light.specular * spec * vec3(texture(specular_map, texCoords));
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}