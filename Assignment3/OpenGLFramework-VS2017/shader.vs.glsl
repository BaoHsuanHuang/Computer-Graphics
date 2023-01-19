#version 330

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

out vec2 texCoord;

uniform mat4 um4p;	
uniform mat4 um4v;
uniform mat4 um4m;



// ================================
out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 vPos;

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
// ================================


vec3 Normalize(vec3 vector)
{
	float l;

	l = float(sqrt(vector.x*vector.x + vector.y*vector.y + vector.z*vector.z));
	vector.x /= l;
	vector.y /= l;
	vector.z /= l;

	return vector;
}

void Lighting(){
	// vec4 view_vertex = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
	// gl_Position = view_vertex;
	vPos = aPos;
	
	// change to eye space normal
	// vertex_normal = transpose(inverse(view) * (aNormal,0.0f);
	vec4 view_aNormal = transpose(inverse(trs)) * vec4(aNormal, 0.0f);
	vertex_normal = view_aNormal.xyz;
	
	vec3 ambient, diffuse, specular;  // intensity = ambient + diffuse + specular

	if(cur_light_mode==0){  // DirectionalLight
		vec4 trs_Pos = trs * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
		
		// nNormal : normalized normal_vector
		// nLight : normalized ligth_source_direction of point light source p
		// nHalf = normalized half_vector between viewpoint and point light source p
		vec3 Light = vec3(dPos.x, dPos.y, dPos.z);
		vec3 nLight = Normalize(Light - (0, 0, 0));
		vec3 View = camera.xyz - trs_Pos.xyz;
		vec3 nView = Normalize(View);
		vec3 nNormal = Normalize(view_aNormal.xyz);
		vec3 nHalf = Normalize(nLight + nView);
		
		// diffuse = Kd * max(N-dot-L, 0) * diffuse
		// specular = Ks * max(N-dot-H)^shininess * specular
		ambient = dAmbient * Ka;
		diffuse = dDiff * Kd * max(dot(nNormal, nLight), 0);
		specular = dSpec * Ks * pow(max(dot(nNormal, nHalf), 0) ,shininess);
		vertex_color = ambient + diffuse + specular;

	}
	else if(cur_light_mode==1){  // PointLight
		vec4 trs_Pos = trs * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
		vec3 Light = vec3(pPos.x, pPos.y, pPos.z);
		vec3 nLight = Normalize(Light - trs_Pos.xyz);
		vec3 View = camera.xyz - trs_Pos.xyz;
		vec3 nView = Normalize(View);
		vec3 nNormal = Normalize(view_aNormal.xyz);
		vec3 nHalf = Normalize(nLight + nView);

		// f : Attenuation function of point light source p
		float d = length(Light - trs_Pos.xyz);
		float f = min(1.0/(pAtteConstant + pAtteLinear*d + pAtteQuad*d*d), 1.0);
		
		ambient = pAmbient * Ka;
		diffuse = pDiff * Kd * max(dot(nNormal, nLight), 0);
		specular = pSpec * Ks * pow(max(dot(nNormal, nHalf), 0) ,shininess);
		vertex_color = ambient + f*(diffuse + specular);
		
	}else if(cur_light_mode==2){  // SpotLight
		vec4 trs_Pos = trs * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
		vec3 Light = vec3(sPos.x, sPos.y, sPos.z);
		vec3 nLight = Normalize(Light - trs_Pos.xyz);
		vec3 View = camera.xyz - trs_Pos.xyz;
		vec3 nView = Normalize(View);
		vec3 nNormal = Normalize(view_aNormal.xyz);
		vec3 nHalf = Normalize(nLight + nView);

		float d = length(Light - trs_Pos.xyz);
		float f = min(1.0/(sAtteConstant + sAtteLinear*d + sAtteQuad*d*d), 1.0);
		
		// spotlight effect : (max{v_dot_d, 0})^spot_exponent
		// v_dot_d = cos(theda2);
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
		vertex_color = ambient + sEffect * f * (diffuse + specular);
	}	

}



// [TODO] passing uniform variable for texture coordinate offset

void main() 
{
	// [TODO]
	// ==========================
	texCoord = aTexCoord;
	gl_Position = um4p * um4v * um4m * vec4(aPos, 1.0);
	if(draw == 0) Lighting();
	else{
		vertex_color = aColor;
		vec4 view_aNormal = transpose(inverse(trs)) * vec4(aNormal, 0.0f);
		vertex_normal = view_aNormal.xyz;
	}
	// ==========================
}
