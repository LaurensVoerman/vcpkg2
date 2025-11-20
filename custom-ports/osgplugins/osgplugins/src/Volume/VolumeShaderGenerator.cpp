#include "VolumeShaderGenerator.h"
#include <string>
#include <sstream>

using namespace osg_ibr;

osg::ref_ptr<osg::Program> VolumeShaderGenerator::generateProgram(Volume::ShadingModel shadingModel){
	osg::ref_ptr<osg::Program> program = new osg::Program();

	// make vertex shader
	std::ostringstream vp_oss;
    vp_oss <<
        "#version 110\n"
        "varying vec3 cameraPos;"
        "varying vec3 vertexPos;"
        "varying float outSide;"
        "void main(void)"
        "{"
        "	gl_Position = ftransform();"

        "	cameraPos = (gl_ModelViewMatrixInverse*vec4(0,0,0,1)).xyz;"
        "	vertexPos = gl_Vertex.xyz;"
        "   outSide = 0.0;"
        "	if (cameraPos.x<0.0 || cameraPos.x>1.0 ||"
        "	    cameraPos.y<0.0 || cameraPos.y>1.0 ||"
        "	    cameraPos.z<0.0 || cameraPos.z>1.0)"
        "	{"
        "		outSide = 1.0;"
        "	}"
		"}";
	osg::Shader *vertexShader = new osg::Shader(osg::Shader::VERTEX, vp_oss.str().c_str());
	program->addShader(vertexShader);
	// make fragment shader
	std::ostringstream fp_oss;
    fp_oss <<
		"#version 110\n"
		"uniform sampler3D baseTexture;"
		"uniform sampler3D gradientTexture;"
		
		"uniform sampler1D tfTexture;"
		"uniform sampler2D tf2DTexture;"

		"uniform float ClipPlaneAlpha;"
		"uniform vec3 ClipPlaneNormal;"
		"uniform float ClipPlaneD;"

		"uniform float SampleDensityValue;"
		"uniform float SampleDensityJitterValue;";
	if(shadingModel == Volume::Isosurface) {
		fp_oss <<
		"uniform float IsoSurfaceValue;";
	}
	fp_oss <<
		"varying vec3 cameraPos;"
		"varying vec3 vertexPos;"
        "varying float outSide;"

		"float lightScale(vec3 v){"
		"	return dot(mix(vec3(0.8,0.0,0.2),vec3(0.1,0.76,0.9),v*0.5+0.5),vec3(0.5));"
		"}"
		"void main(void)"
		"{ "
		"	vec4 fragColor = vec4(0.0, 0.0, 0.0, 0.0);"
		"	vec3 t0 = vertexPos;"
		"	vec3 te = cameraPos;"
		"	vec3 eyeDirection = normalize((vertexPos-cameraPos));"
        "	if (outSide > 0.5)"
		"	{"
		"		vec3 exitPlanes = step(0.0,-eyeDirection);"
		"		vec3 t = (vertexPos-exitPlanes)/eyeDirection;"
		"		float tMin = min(min(t.x,t.y),t.z);"
		"		te = vertexPos-tMin*eyeDirection;"
		"	}"
		"	bool lastPlane = false;"
        // distance to the back point
		"	float t0Dn = dot(t0,ClipPlaneNormal)+ClipPlaneD;"
        // distance to the front point
		"	float teDn = dot(te,ClipPlaneNormal)+ClipPlaneD;"
        "	if(t0Dn > 0.0 && teDn <= 0.0){"
		"		float t = (teDn)/dot(eyeDirection,ClipPlaneNormal);"
		"		te = te - t*eyeDirection;";
	if(shadingModel == Volume::Standard || shadingModel == Volume::Light){
	fp_oss <<
		"		float v = texture3D( baseTexture, te).a;"
		"		vec3 color = ClipPlaneAlpha*texture1D( tfTexture, v).rgb;"
		"		fragColor = vec4(color, ClipPlaneAlpha);";
    } else if(shadingModel == Volume::StandardWithGradientMagnitude || shadingModel == Volume::StandardWithGradientMagnitudeAndLight){
	fp_oss <<
		"		float v = texture3D( baseTexture, te).a;"
		"		float length = texture3D( gradientTexture, te).w;"
		"		vec3 color = ClipPlaneAlpha*texture2D( tf2DTexture, vec2(v,length)).rgb;"
		"		fragColor = vec4(color, ClipPlaneAlpha);";
	}
	fp_oss <<
        "	} else if(teDn > 0.0 && t0Dn <= 0.0){"
		"		float t = (t0Dn)/dot(eyeDirection,ClipPlaneNormal);"
		"		t0 = t0 - t*eyeDirection;"
		"		lastPlane = true;"
		"	} else if(t0Dn <= 0.0 && teDn <= 0.0){"
		"		te = t0;"
		"	}"
		"	const float max_iteratrions = 8096.0;";
	if(shadingModel == Volume::Isosurface) {
		fp_oss <<
		"	float SampleDensityValueJitter = SampleDensityValue *(1.0+SampleDensityJitterValue*0.01*fract(sin(gl_FragCoord.x*12.9898+gl_FragCoord.y*78.233)*43758.5453));";
	} else {
		fp_oss <<
		"	float SampleDensityValueJitter = SampleDensityValue *(1.0+SampleDensityJitterValue*0.2*fract(sin(gl_FragCoord.x*12.9898+gl_FragCoord.y*78.233)*43758.5453));";
	}
	fp_oss <<
		"   float num_iterations = ceil(length((te-t0))/SampleDensityValueJitter);"
		"   num_iterations = min(max(num_iterations,2.0),max_iteratrions);"

		"   vec3 deltaTexCoord=(t0-te)/float(num_iterations-1.0);"
		"   vec3 texcoord = te+deltaTexCoord;";
	if(shadingModel == Volume::MaximumIntensityProjection){
		fp_oss <<

		"	const float slices = 256.0;"
		"   float opscaling = length(deltaTexCoord)*slices;"
		"	while(num_iterations>0.0)"
		"	{"
		"		float v = texture3D( baseTexture, texcoord).s;"
		"	    vec4 color = texture1D( tfTexture, v);"
		"	    if (fragColor.w<color.w)"
		"	    {"
		"	        fragColor = color;"
		"	    }"
		"	    texcoord += deltaTexCoord;"

		"	   --num_iterations;"
		"	}";
	} else if(shadingModel == Volume::Isosurface) {
		fp_oss <<
		"   float previousV = texture3D( baseTexture, texcoord).a;"
		"	float m = IsoSurfaceValue;"
		"   float v = 1.0;"
		"   while(m > 0.0 && num_iterations>0.0)"
		"   {"
		"		previousV = v;"
		"		texcoord += deltaTexCoord;"
		"       v = texture3D( baseTexture, texcoord).a;"
		"		m = IsoSurfaceValue-v;"
		"		--num_iterations;"
		"	}"
		"   if (m <= 0.0)"
		"   {"
		"		const float half = 0.5;"
		"		float dir = -1.0;"
		"		for(int i = 0; i < 6; i++){"
		"			deltaTexCoord *= half;"
		"			texcoord += deltaTexCoord*dir;"
		"			v = texture3D( baseTexture, texcoord).a;"
		"			dir = 2.0*step(v,IsoSurfaceValue)-1.0;"
		"		}"
		
		"       vec4 color = texture1D( tfTexture, v);"
		"		vec3 grad = texture3D( gradientTexture, texcoord).rgb*2.0-1.0;"
		"       if (any(grad!=0.0))"
		"       {"
		"			vec3 normal = normalize(grad);"
		"           float lightScale = lightScale(normal);"
		"           color.xyz *= lightScale;"
		"       }"
		"       color.a = 1.0;"
		"       gl_FragColor = color;"
		"       return;"
		"	} else {"
		"		discard;"
		"	}";
	} else if(shadingModel == Volume::Light){
		fp_oss <<
		"	const float slices = 256.0;"
		"   float opscaling = length(deltaTexCoord)*slices;"
		"   while(num_iterations>0.0 && fragColor.w < 0.99)"
		"   {"
		// unrolling the loop to do two itterations seems to give a good performance boost
		// after two iterations it does not seem to matter on a geforce 560
		"       float v = texture3D( baseTexture, texcoord).a;"
		"       vec4 color = texture1D( tfTexture, v);"
		"		vec3 grad = texture3D( gradientTexture, texcoord).rgb*2.0-1.0;"
		"       if (any(grad!=0.0))"
		"       {"
		"           vec3 normal = normalize(grad);"
		"           float lightScale = lightScale(normal);"
		"           color.xyz *= lightScale;"
		"       }"
		"		float alpha = 1.0 - pow((1.0 - color.a),opscaling);"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"       texcoord += deltaTexCoord;"

		"       v = texture3D( baseTexture, texcoord).a;"
		"       color = texture1D( tfTexture, v);"
		"		grad = texture3D( gradientTexture, texcoord).rgb*2.0-1.0;"
		"       if (any(grad!=0.0))"
		"       {"
		"           vec3 normal = normalize(grad);"
		"           float lightScale = lightScale(normal);"
		"           color.xyz *= lightScale;"
		"       }"
		"		alpha = 1.0 - pow((1.0 - color.a),opscaling);"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"       texcoord += deltaTexCoord;"

		"       num_iterations -= 2.0;"
		"   }";
	} else if(shadingModel == Volume::Standard){
		fp_oss << 
		"	const float slices = 256.0;"
		"	float opscaling = length(deltaTexCoord)*slices;"
		"   while(num_iterations>0.0 && fragColor.w < 0.99)"
		"   {"
		// unrolling the loop to do two itterations seems to give a good performance boost
		// after two iterations it does not seem to matter on a geforce 560
		"		float v = texture3D( baseTexture, texcoord).a;"
		"       vec4 color = texture1D( tfTexture, v);"
		"       float alpha = 1.0 - pow((1.0 - color.w),opscaling);"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"       texcoord += deltaTexCoord;"
        
		"       v = texture3D( baseTexture, texcoord).a;"
		"       color = texture1D( tfTexture, v);"
		"       alpha = 1.0 - pow((1.0 - color.w),opscaling);"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"       texcoord += deltaTexCoord; "

		"       num_iterations-=2.0;"
		"   }"
		"	if(lastPlane){"
		"		float v = texture3D( baseTexture, t0).a;"
		"       vec4 color = texture1D( tfTexture, v);"
		"       float alpha = ClipPlaneAlpha;"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"	}";
    } else if(shadingModel == Volume::StandardNoTransferfunction){
        fp_oss <<
            "	const float slices = 256.0;"
            "	float opscaling = length(deltaTexCoord)*slices;"
            "   while(num_iterations>0.0 && fragColor.w < 0.99)"
            "   {"
            // unrolling the loop to do two itterations seems to give a good performance boost
            // after two iterations it does not seem to matter on a geforce 560
            "		vec4 color = texture3D( baseTexture, texcoord).rgba;"
            "       float alpha = 1.0 - pow((1.0 - color.w),opscaling);"
            "       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
            "       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
            "       texcoord += deltaTexCoord;"

            "       color = texture3D( baseTexture, texcoord).rgba;"
            "       alpha = 1.0 - pow((1.0 - color.w),opscaling);"
            "       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
            "       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
            "       texcoord += deltaTexCoord; "

            "       num_iterations-=2.0;"
            "   }";
    } else if(shadingModel == Volume::StandardWithGradientMagnitude){
		fp_oss << 
		"	const float slices = 256.0;"
		"	float opscaling = length(deltaTexCoord)*slices;"
		"   while(num_iterations>0.0 && fragColor.w < 0.99)"
		"   {"
		// unrolling the loop to do two itterations seems to give a good performance boost
		// after two iterations it does not seem to matter on a geforce 560
		"		float v = texture3D( baseTexture, texcoord).a;"
		"		float length = texture3D( gradientTexture, texcoord).w;"
		"       vec4 color = texture2D( tf2DTexture, vec2(v,length));"
		"       float alpha = 1.0 - pow((1.0 - color.w),opscaling);"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"       texcoord += deltaTexCoord;"
        
		"       v = texture3D( baseTexture, texcoord).a;"
		"		length = texture3D( gradientTexture, texcoord).w;"
		"       color = texture2D( tf2DTexture, vec2(v,length));"
		"       alpha = 1.0 - pow((1.0 - color.w),opscaling);"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"       texcoord += deltaTexCoord; "

		"       num_iterations-=2.0;"
		"   }"
		"	if(lastPlane){"
		"		float v = texture3D( baseTexture, t0).a;"
		"		float length = texture3D( gradientTexture, texcoord).w;"
		"       vec4 color = texture2D( tf2DTexture, vec2(v,length));"
		"       float alpha = ClipPlaneAlpha;"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"	}";
	} else if(shadingModel == Volume::StandardWithGradientMagnitudeAndLight){
		fp_oss << 
		"	const float slices = 256.0;"
		"	float opscaling = length(deltaTexCoord)*slices;"
		"   while(num_iterations>0.0 && fragColor.w < 0.99)"
		"   {"
		// unrolling the loop to do two itterations seems to give a good performance boost
		// after two iterations it does not seem to matter on a geforce 560
		"		float v = texture3D( baseTexture, texcoord).a;"
		"		vec3 grad = texture3D( gradientTexture, texcoord).rgb*2.0-1.0;"
		"		float length = texture3D( gradientTexture, texcoord).w;"
		"       vec4 color = texture2D( tf2DTexture, vec2(v,length));"
		"       if (any(grad!=0.0))"
		"       {"
		"           vec3 normal = normalize(grad);"
		"           float lightScale = lightScale(normal);"
		"           color.xyz *= lightScale;"
		"       }"
		"       float alpha = 1.0 - pow((1.0 - color.w),opscaling);"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"       texcoord += deltaTexCoord;"
        
		"       v = texture3D( baseTexture, texcoord).a;"
		"		grad = texture3D( gradientTexture, texcoord).rgb*2.0-1.0;"
		"		length = texture3D( gradientTexture, texcoord).w;"
		"       color = texture2D( tf2DTexture, vec2(v,length));"
		"       if (any(grad!=0.0))"
		"       {"
		"           vec3 normal = normalize(grad);"
		"           float lightScale = lightScale(normal);"
		"           color.xyz *= lightScale;"
		"       }"
		"       alpha = 1.0 - pow((1.0 - color.w),opscaling);"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"       texcoord += deltaTexCoord; "

		"       num_iterations-=2.0;"
		"   }"
		"	if(lastPlane){"
		"		float v = texture3D( baseTexture, t0).a;"
		"		float length = texture3D( gradientTexture, texcoord).w;"
		"       vec4 color = texture2D( tf2DTexture, vec2(v,length));"
		"       float alpha = ClipPlaneAlpha;"
		"       fragColor.xyz = fragColor.xyz + alpha*color.rgb*(1.0-fragColor.w);"
		"       fragColor.w = fragColor.w + alpha*(1.0-fragColor.w);"
		"	}";
	}
	fp_oss <<
		"	gl_FragColor = fragColor;"
		"}";
	osg::Shader *fragmentShader = new osg::Shader(osg::Shader::FRAGMENT, fp_oss.str().c_str());
	program->addShader(fragmentShader);
	return program;
}