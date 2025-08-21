#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

struct S_Material
{
   sampler2D Diffuse;
   sampler2D Specular;
   float Shininess;
   sampler2D Emission;
};

struct S_DirectionalLight
{
   vec3 Direction;
   vec3 Ambient;
   vec3 Diffuse;
   vec3 Specular;
};

struct S_PointLight
{
   vec3 Position;
   vec3 Ambient;
   vec3 Diffuse;
   vec3 Specular;
   float Constant;
   float Linear;
   float Quadratic;
};

struct S_SpotLight
{
   vec3 Position;
   vec3 Direction;
   float InnerCutOff;
   float OuterCutOff;
   float Constant;
   float Linear;
   float Quadratic;
   vec3 Ambient;
   vec3 Diffuse;
   vec3 Specular;
};

vec3 CalcDirLight(S_DirectionalLight Light, vec3 Normal, vec3 ViewDir);
vec3 CalcPointLight(S_PointLight Light, vec3 Normal, vec3 FragPos, vec3 ViewDir);
vec3 CalcSpotLight(S_SpotLight Light, vec3 Normal, vec3 FragPos, vec3 ViewDir);

uniform vec3 ViewPos;
uniform S_Material Material;
uniform S_DirectionalLight DirLight;
#define NR_POINT_LIGHTS 4
uniform S_PointLight PointLights[NR_POINT_LIGHTS];
uniform S_SpotLight SpotLight;

void main()
{
   vec3 Emission = vec3(texture(Material.Emission, TexCoords));

   vec3 ViewDir = (ViewPos - FragPos);

   vec3 Result = CalcDirLight(DirLight, Normal, ViewDir);

   for(int i = 0; i < NR_POINT_LIGHTS; i++)
      Result += CalcPointLight(PointLights[i], Normal, FragPos, ViewDir);

   Result += CalcSpotLight(SpotLight, Normal, FragPos, ViewDir);

   FragColor = vec4(Result + Emission, 1.f);
}

vec3 CalcDirLight(S_DirectionalLight Light, vec3 Normal, vec3 ViewDir)
{
   vec3 LightDir = normalize(-Light.Direction);
   vec3 ReflectDir = reflect(Light.Direction, Normal);
   float fDiffuse = max(dot(LightDir, Normal), 0.f);
   float fSpecular = pow(max(dot(ViewDir, ReflectDir), 0.f), Material.Shininess);

   vec3 ambient = Light.Ambient * vec3(texture(Material.Diffuse, TexCoords));
   vec3 diffuse = Light.Diffuse * fDiffuse * vec3(texture(Material.Diffuse, TexCoords));
   vec3 specular = Light.Specular * fSpecular * vec3(texture(Material.Specular, TexCoords));
   return (ambient + diffuse + specular);
}

vec3 CalcPointLight(S_PointLight Light, vec3 Normal, vec3 FragPos, vec3 ViewDir)
{
   vec3 LightDir = normalize(Light.Position - FragPos);
   vec3 ReflectDir = reflect(-LightDir, Normal);
   float fDiffuse = max(dot(LightDir, Normal), 0.f);
   float fSpecular = pow(max(dot(ViewDir, ReflectDir), 0.f), Material.Shininess);
   float Distance = length(Light.Position - FragPos);
   float Attenuation = 1.0 / (Light.Constant + Light.Linear * Distance + Light.Quadratic * (Distance * Distance));

   vec3 ambient = Light.Ambient * vec3(texture(Material.Diffuse, TexCoords));
   vec3 diffuse = Light.Diffuse * fDiffuse * vec3(texture(Material.Diffuse, TexCoords));
   vec3 specular = Light.Specular * fSpecular * vec3(texture(Material.Specular, TexCoords));
   ambient *= Attenuation;
   diffuse *= Attenuation;
   specular *= Attenuation;
   return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(S_SpotLight Light, vec3 Normal, vec3 FragPos, vec3 ViewDir)
{
   vec3 LightDir = normalize(Light.Position - FragPos);
   vec3 ReflectDir = reflect(-LightDir, Normal);
   float fDiffuse = max(dot(LightDir, Normal), 0.f);
   float fSpecular = pow(max(dot(ViewDir, ReflectDir), 0.f), Material.Shininess);
   float Distance = length(Light.Position - FragPos);
   float Attenuation = 1.0 / (Light.Constant + Light.Linear * Distance + Light.Quadratic * (Distance * Distance));

   float ThetaValue = dot(LightDir, normalize(-Light.Direction));
   float Phi = dot(LightDir, normalize(-Light.Direction));
   float Epsilone = Light.InnerCutOff - Light.OuterCutOff;
   float Intensity = clamp((Phi - Light.OuterCutOff) / Epsilone, 0.f, 1.f);
   vec3 ambient = Light.Ambient * vec3(texture(Material.Diffuse, TexCoords));
   vec3 diffuse = Light.Diffuse * fDiffuse * vec3(texture(Material.Diffuse, TexCoords));
   vec3 specular = Light.Specular * fSpecular * vec3(texture(Material.Specular, TexCoords));
   ambient *= Attenuation;
   diffuse *= Attenuation * Intensity;
   specular *= Attenuation * Intensity;
   return (ambient + diffuse + specular);
}