#version 410 core
#define SPOTLIGHT_NO 4

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;
uniform mat4 view;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// Ambient, diffuse, specular components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

struct spotLight{
	vec3 position;
	vec3 direction;
	float cutOff;
	float outerCutOff;
};
uniform spotLight mySpotLight[4];

struct pointLight {    
    vec3 position;
    float constant;
    float linear;
    float quadratic;  
	vec3 color;
}; 
uniform pointLight myPointLight; 

void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(fNormal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDir);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	//compute ambient light
	ambient = ambientStrength * lightColor;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
}

void computeSpotLight(spotLight mySpotLight) {
    vec3 cameraPosEye = vec3(0.0f);
    vec3 spotPosEye = (view * vec4(mySpotLight.position, 1.0f)).xyz;
    vec3 spotDirEye = normalize(mat3(view) * mySpotLight.direction); 
  
    vec3 lightDirSpot = normalize(spotPosEye - fPosEye.xyz);

    float theta = dot(lightDirSpot, -spotDirEye);
    float epsilon = mySpotLight.cutOff - mySpotLight.outerCutOff;
    float intensity = clamp((theta - mySpotLight.outerCutOff) / epsilon, 0.0f, 1.0f);

    vec3 normalEye = normalize(fNormal); 
    vec3 viewDir = normalize(cameraPosEye - fPosEye.xyz); 

    vec3 lightColorSpot=vec3(1.0f, 1.0f, 1.0f);

    if(theta>mySpotLight.outerCutOff){
        vec3 reflection = reflect(-lightDirSpot, normalEye);
        float specCoeff = pow(max(dot(viewDir, reflection), 0.0f), shininess);

        ambient += ambientStrength*lightColorSpot;
        diffuse +=intensity * max(dot(normalEye, lightDirSpot), 0.0f) * lightColorSpot;
        specular += intensity*  specularStrength * specCoeff * lightColorSpot;
    }
}

void computePointLight() {
    vec3 cameraPosEye = vec3(0.0f);
    vec3 pointPosEye = (view * vec4(myPointLight.position, 1.0f)).xyz;
  //  vec3 pointDirEye = normalize(mat3(view) * myPointLight.direction); 
  
   
    vec3 normalEye = normalize(fNormal); 
    vec3 lightDirPoint = normalize(pointPosEye - fPosEye.xyz);
    vec3 reflection = reflect(-lightDirPoint, normalEye);
    vec3 viewDir = normalize(cameraPosEye - fPosEye.xyz); 

    float specCoeff = pow(max(dot(viewDir, reflection), 0.0f), shininess);
    //compute distance to light
    float dist = length(pointPosEye - fPosEye.xyz);
    //compute attenuation
    float att = 1.0f / (myPointLight.constant + myPointLight.linear * dist + myPointLight.quadratic * (dist * dist));  

    vec3 lightColorPoint = myPointLight.color*2.0; 

    //compute ambient light
    ambient += att * ambientStrength * lightColorPoint;
    //compute diffuse light
    diffuse += att * max(dot(normalEye, lightDirPoint), 0.0f) * lightColorPoint;
    specular += att * specularStrength * specCoeff * lightColorPoint;

}

float computeShadow() {
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    normalizedCoords = normalizedCoords * 0.5 + 0.5; 

    if (normalizedCoords.z > 1.0f)
        return 0.0f;

    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
    float currentDepth = normalizedCoords.z;

    float bias = max(0.05 * (1.0 - dot(fNormal, lightDir)), 0.005);
    float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

    return shadow;
}

float computeFog()
{
 float fogDensity = 0.005f;
 float fragmentDistance = length(fPosEye);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}

void main() 
{
	computeLightComponents();

	vec4 colorFromTexture = texture(diffuseTexture, fTexCoords);

    // Eliminăm fragmentul dacă alfa este sub pragul definit
    if (colorFromTexture.a < 0.1)
        discard;

    for(int i = 0; i<4;i++)
			computeSpotLight(mySpotLight[i]);
    
    computePointLight();

    // Aplicăm componentele de iluminare
    ambient *= texture(diffuseTexture, fTexCoords).rgb ;
    diffuse *= texture(diffuseTexture, fTexCoords).rgb ;
    specular *= texture(specularTexture, fTexCoords).rgb;

    // Calculăm umbra
    float shadow = computeShadow();

    // Combinație finală de iluminare
    vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);
    color=color*0.2;

	// Fog calculation
    float fogFactor = computeFog();
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);

    vec4 finalColor = vec4(color, 0.7);//gr de op
    fColor = mix(fogColor, finalColor, fogFactor);
}
