#version 330

in vec2 texCoord;

out vec4 fragColor;


// ======================
// out vec4 FragColor;
in vec3 vertex_color; // the input_variable from vertex_shader (same name and same type)
in vec3 vertex_normal;
in vec3 vPos;

// uniform mat4 mvp;
// uniform mat4 view;
uniform int cur_light_mode;
uniform int draw;
uniform float shininess;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform vec3 dPos;
uniform vec3 pPos;
uniform vec3 sPos;
uniform vec3 dDiff;
uniform vec3 pDiff;
uniform vec3 sDiff;
uniform vec3 dAmbient;
uniform vec3 pAmbient;
uniform vec3 sAmbient;
uniform vec3 dSpec;
uniform vec3 pSpec;
uniform vec3 sSpec;
uniform float pAtteConstant;
uniform float pAtteLinear;
uniform float pAtteQuad;
uniform float sAtteConstant;
uniform float sAtteLinear;
uniform float sAtteQuad;
uniform vec3 sDirection;
uniform float sCutoff;
uniform float sExponent;
uniform mat4 originView;
uniform vec3 camera;
uniform mat4 trs;
// ======================
uniform int cur_eye_idx;
uniform int isEye;
uniform sampler2D uTexture;
uniform float row;
uniform float col;


vec3 Normalize(vec3 vector)
{
	float l;

	l = float(sqrt(vector.x*vector.x + vector.y*vector.y + vector.z*vector.z));
	vector.x /= l;
	vector.y /= l;
	vector.z /= l;

	return vector;
}


void DirectionalLight(){
	vec2 updateTexture;
	if(isEye == 0) updateTexture = texCoord;
	else updateTexture = texCoord - vec2(row, col); 

	vec3 ambient, diffuse, specular;
	vec4 trs_Pos = trs * vec4(vPos.x, vPos.y, vPos.z, 1.0f);
	vec3 Light = vec3(dPos.x, dPos.y, dPos.z);
	vec3 nLight = Normalize(Light - (0, 0, 0));
	vec3 View = camera.xyz - trs_Pos.xyz;
	vec3 nView = Normalize(View);
	vec3 nNormal = Normalize(vertex_normal);
	vec3 nHalf = Normalize(nLight + nView);
	
	ambient = dAmbient * Ka;
	diffuse = dDiff * Kd * max(dot(nNormal, nLight), 0);
	specular = dSpec * Ks * pow(max(dot(nNormal, nHalf), 0) ,shininess);

	// FragColor = vec4(ambient + diffuse + specular, 1.0f);
	fragColor = vec4(ambient + diffuse + specular, 1.0f) * texture(uTexture, updateTexture);
}

void PointLight(){
	vec2 updateTexture;
	if(isEye == 0) updateTexture = texCoord;
	else updateTexture = texCoord - vec2(row, col); 

	vec3 ambient, diffuse, specular;
	vec4 trs_Pos = trs * vec4(vPos.x, vPos.y, vPos.z, 1.0f);
	vec3 Light = vec3(pPos.x, pPos.y, pPos.z);
	vec3 nLight = Normalize(Light - trs_Pos.xyz);
	vec3 View = camera.xyz - trs_Pos.xyz;
	vec3 nView = Normalize(View);
	vec3 nNormal = Normalize(vertex_normal);
	vec3 nHalf = Normalize(nLight + nView);

	float d = length(Light - trs_Pos.xyz);
	float f = min(1.0/(pAtteConstant + pAtteLinear*d + pAtteQuad*d*d), 1.0);
	
	ambient = pAmbient * Ka;
	diffuse = pDiff * Kd * max(dot(nNormal, nLight), 0);
	specular = pSpec * Ks * pow(max(dot(nNormal, nHalf), 0) ,shininess);

	// FragColor = vec4(ambient + f*(diffuse + specular), 1.0f);
	fragColor = vec4(ambient + f*(diffuse + specular), 1.0f) * texture(uTexture, updateTexture);
}

void SpotLight(){
	vec2 updateTexture;
	if(isEye == 0) updateTexture = texCoord;
	else updateTexture = texCoord - vec2(row, col); 

	vec3 ambient, diffuse, specular;
	vec4 trs_Pos = trs * vec4(vPos.x, vPos.y, vPos.z, 1.0f);
	vec3 Light = vec3(sPos.x, sPos.y, sPos.z);
	vec3 nLight = Normalize(Light - trs_Pos.xyz);
	vec3 View = camera.xyz - trs_Pos.xyz;
	vec3 nView = Normalize(View);
	vec3 nNormal = Normalize(vertex_normal);
	vec3 nHalf = Normalize(nLight + nView);

	float d = length(Light - trs_Pos.xyz);
	float f = min(1.0/(sAtteConstant + sAtteLinear*d + sAtteQuad*d*d), 1.0);
	
	vec3 v = nLight;
	vec3 dir = Normalize(-sDirection);
	float v_dot_d = dot(v, dir);
	float theda = sCutoff / 180.0 * 3.1415926;
	float sEffect = 0.0;
	if(v_dot_d > cos(theda)) sEffect = pow(max(v_dot_d, 0), sExponent);
	else sEffect = 0.0;

	ambient = sAmbient * Ka;
	diffuse = sDiff * Kd * max(dot(nNormal, nLight), 0);
	specular = sSpec * Ks * pow(max(dot(nNormal, nHalf), 0) ,shininess);

	// FragColor = vec4(ambient + sEffect * f * (diffuse + specular), 1.0f);
	fragColor = vec4(ambient + sEffect * f * (diffuse + specular), 1.0f) * texture(uTexture, updateTexture);
}

void Lighting(){
	if(cur_light_mode==0) DirectionalLight();
	else if(cur_light_mode==1) PointLight();
	else if(cur_light_mode==2) SpotLight();	
}


// [TODO] passing texture from main.cpp
// Hint: sampler2D
void main() {
	if(draw==1){
		Lighting();
	}else{
		vec2 updateTexture;
		if(isEye == 0){
			fragColor = vec4(vertex_color, 1.0f) * texture(uTexture, texCoord);
		} 
		else{
			updateTexture = texCoord - vec2(row, col);
			fragColor = vec4(vertex_color, 1.0f) * texture(uTexture, updateTexture);
		}
	}
	// fragColor = vec4(texCoord.xy, 0, 1);
	// [TODO] sampleing from texture
	// Hint: texture
}
