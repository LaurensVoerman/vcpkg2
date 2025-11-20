#include "RCBumpMapping.h"
#include <osgFX/Registry>
#include <osgDB/ReadFile>
#include <sstream>

class RecompileCallback : public osg::Drawable::UpdateCallback {
    virtual void update(osg::NodeVisitor* nv, osg::Drawable* node)
    {
        if(nv->getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR){
            osg_ibr::RCBumpMapping* bmNode = static_cast<osg_ibr::RCBumpMapping*>(node);
            bmNode->rebuildShaders();
            bmNode->dirty(false);
        }
    }
};

using namespace osg_ibr;

RCBumpMapping::RCBumpMapping()
:   Geometry(),
    _diffuse_unit(0),
    _normal_unit(1),
	_inputGamma(false),
	_outPutGamma(false)
{
    dirty(true);
}

RCBumpMapping::RCBumpMapping(const RCBumpMapping& copy, const osg::CopyOp& copyop)
:   Geometry(copy, copyop),
    _diffuse_unit(copy._diffuse_unit),
    _normal_unit(copy._normal_unit),
	_inputGamma(false),
	_outPutGamma(false)
{
    dirty(true);
}

void RCBumpMapping::rebuildShaders()
{
    // vertex program
    std::ostringstream vp_oss;
    vp_oss <<
	    "#version 120\n"
        "attribute vec3 position;"
        "attribute vec2 texcoord;"
	    "attribute vec4 tangent;"
	    "attribute vec3 normal;"

	    "varying vec3 N;"
	    "varying vec3 T;"
	    "varying vec3 B;"
        "varying vec2 uvCoord;";
    for(unsigned int i = 0; i < _spotLights.size();i++){
	    vp_oss << "varying vec3 lightVec"<< _spotLights[i] << ";";
    }
    for(unsigned int i = 0; i < _pointLights.size();i++){
	    vp_oss << "varying vec3 lightVec"<< _pointLights[i] << ";";
    }

    vp_oss <<
	    "void main(void)\n"
	    "{\n"
	    "   uvCoord =  texcoord;"
        "   vec4 vertexPosition = gl_ModelViewMatrix * vec4(position,1);"

	    "   N = normalize( gl_NormalMatrix * normal);\n"
	    "   T = normalize( gl_NormalMatrix * tangent.xyz);\n"
        "   B = cross(normal,tangent.xyz)*tangent.w;\n"

	    "	vec3 lightDir;";
    for(unsigned int i = 0; i < _spotLights.size();i++){
	    vp_oss <<
	    "	lightDir = gl_LightSource["<< _spotLights[i] << "].position.xyz - vertexPosition.xyz;"
	    "	lightVec"<< _spotLights[i] << " = lightDir;\n";
    }
    for(unsigned int i = 0; i < _pointLights.size();i++){
	    vp_oss <<
	    "	lightDir = gl_LightSource["<< _pointLights[i] << "].position.xyz - vertexPosition.xyz;"
	    "	lightVec"<< _pointLights[i] << " = lightDir;\n";
    }
	    vp_oss <<
            "   gl_Position = gl_ModelViewProjectionMatrix * vec4(position,1);"
	    "}"
        "\n";
         
    // fragment program
    std::ostringstream fp_oss;
	    fp_oss <<
	    "#version 120\n"
	    "uniform sampler2D diffuseTex;"
	    "uniform sampler2D normalTex;"
	    "varying vec3 N;"
	    "varying vec3 T;"
	    "varying vec3 B;"
        "varying vec2 uvCoord;";
    for(unsigned int i = 0; i < _spotLights.size();i++){
	    fp_oss <<"varying vec3 lightVec"<< _spotLights[i] << ";";
    }
    for(unsigned int i = 0; i < _pointLights.size();i++){
	    fp_oss <<"varying vec3 lightVec"<< _pointLights[i] << ";";
    }
	    fp_oss <<
	    "void main(void)"
	    "{";
	    fp_oss <<
	    "	vec3 normal = 2.0 * texture2D (normalTex, uvCoord).rgb - 1.0;\n"
        "	normal = normalize (normal.x*normalize(T)+normal.y*normalize(B)+normal.z*normalize(N));"
	    "	float dist;"
	    "	float lambertTerm;"
	    "	vec3 lightVec;";
    if(_inputGamma){
	    fp_oss <<"	vec4 diffuseMaterial = pow(texture2D (diffuseTex, uvCoord),vec4(2.2));";
    } else {
	    fp_oss <<"	vec4 diffuseMaterial = texture2D (diffuseTex, uvCoord);";
    }
	    fp_oss <<"	vec4 final_color = diffuseMaterial * gl_FrontMaterial.ambient;";
    for(unsigned int i = 0; i < _spotLights.size();i++){
	    fp_oss <<
	    "	dist = length(lightVec"<< _spotLights[i] << ");"
	    "	lightVec = normalize(lightVec"<< _spotLights[i] << ");"
	    "	lambertTerm = max(dot(normal,lightVec),0.0);"
	    "	if(lambertTerm > 0.0)\n"
	    "	{"
	    "		float spotEffect = dot(normalize(gl_LightSource["<< _spotLights[i] << "].spotDirection),-lightVec);"
	    "		if (spotEffect > gl_LightSource["<< _spotLights[i] << "].spotCosCutoff) {"
	    "			spotEffect = pow(spotEffect, gl_LightSource["<< _spotLights[i] << "].spotExponent);"
	    "			float att = spotEffect / (gl_LightSource["<< _spotLights[i] << "].constantAttenuation + gl_LightSource["<< _spotLights[i] << "].linearAttenuation * dist + gl_LightSource["<< _spotLights[i] << "].quadraticAttenuation*dist*dist);"
	    "			final_color += att * (gl_LightSource["<< _spotLights[i] << "].diffuse * diffuseMaterial * lambertTerm)/3.14;"
	    "			vec3  halfV = normalize(gl_LightSource["<< _spotLights[i] << "].halfVector.xyz);"
	    "			float  NdotHV = max(dot(normal,halfV),0.0);"
	    "			final_color += att * ((gl_FrontMaterial.shininess+8)/(8*3.14))*gl_FrontMaterial.specular * gl_LightSource["<< _spotLights[i] << "].specular * pow(NdotHV,gl_FrontMaterial.shininess)*lambertTerm;"
	    "		}"
	    "	}";
    }
    for(unsigned int i = 0; i < _pointLights.size();i++){
	    fp_oss <<
	    "	dist = length(lightVec"<< _pointLights[i] << ");"
	    "	lightVec = normalize(lightVec"<< _pointLights[i] << ");"
	    "	lambertTerm = max(dot(normal,lightVec),0.0);"
	    "	if(lambertTerm > 0.0)\n"
	    "	{"
	    "		float att = 1.0 / (gl_LightSource["<< _pointLights[i] << "].constantAttenuation + gl_LightSource["<< _pointLights[i] << "].linearAttenuation * dist + gl_LightSource["<< _pointLights[i] << "].quadraticAttenuation*dist*dist);"
	    "		final_color += att * (gl_LightSource["<< _pointLights[i] << "].diffuse * diffuseMaterial * lambertTerm)/3.14;"
	    "		vec3  halfV = gl_LightSource["<< _pointLights[i] << "].halfVector.xyz;"
	    "		float  NdotHV = max(dot(normal,halfV),0.0);"
	    "		final_color += att * ((gl_FrontMaterial.shininess+8)/(8*3.14))*gl_FrontMaterial.specular * gl_LightSource["<< _pointLights[i] << "].specular * pow(NdotHV,gl_FrontMaterial.shininess)*lambertTerm;"
	    "	}";
    }
    for(unsigned int i = 0; i < _directionalLights.size();i++){
	    fp_oss <<
	    "	lightVec = normalize(gl_LightSource["<< _directionalLights[i] << "].position.xyz);"
	    "	lambertTerm = max(dot(normal,lightVec),0.0);"
	    "	final_color += diffuseMaterial * gl_LightSource["<< _directionalLights[i] << "].ambient;";
	    fp_oss <<
	    "	if(lambertTerm > 0.0)\n"
	    "	{"
	    "		final_color += gl_LightSource["<< _directionalLights[i] << "].diffuse * diffuseMaterial * lambertTerm/3.14;"
	    "		vec3  halfV = normalize(gl_LightSource["<< _directionalLights[i] << "].halfVector.xyz);"
	    "		float  NdotHV = max(dot(normal,halfV),0.0);"
	    "		final_color += ((gl_FrontMaterial.shininess+8)/(8*3.14))*gl_FrontMaterial.specular * gl_LightSource["<< _directionalLights[i] << "].specular * pow(NdotHV,gl_FrontMaterial.shininess)*lambertTerm;"
	    "	}";
    }
    if(_outPutGamma){
	    fp_oss <<"	gl_FragColor = vec4(pow(final_color.rgb,vec3(1.0/2.2)),1);";
    } else {
	    fp_oss <<"	gl_FragColor = final_color;";
    }
	    fp_oss <<"}\n";

    osg::StateSet *ss = getOrCreateStateSet();
    ss->setDataVariance(DYNAMIC); // Needs to be dynamic because we change the shader when light are added/removed
    osg::ref_ptr<osg::Program> program = new osg::Program;
    osg::ref_ptr<osg::Shader> vp = new osg::Shader( osg::Shader::VERTEX);
    vp->setShaderSource(vp_oss.str());

    osg::ref_ptr<osg::Uniform> uniform_diffuse_tex = ss->getOrCreateUniform("diffuseTex",osg::Uniform::SAMPLER_2D);
    uniform_diffuse_tex->set(_diffuse_unit);
    osg::ref_ptr<osg::Uniform> uniform_normal_tex = ss->getOrCreateUniform("normalTex",osg::Uniform::SAMPLER_2D);
    uniform_normal_tex->set(_normal_unit);

    osg::ref_ptr<osg::Shader> fp = new osg::Shader( osg::Shader::FRAGMENT);
    fp->setShaderSource(fp_oss.str());
    program->addShader( vp );
    program->addShader( fp );
    program->addBindAttribLocation("position",0);
    program->addBindAttribLocation("texcoord",1);
    program->addBindAttribLocation("tangent",2);
    program->addBindAttribLocation("normal",3);
    ss->setAttributeAndModes(program.get(), osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);
    ss->addUniform(uniform_diffuse_tex.get() );
    ss->addUniform(uniform_normal_tex.get() );
}

void RCBumpMapping::dirty(bool val){
    static osg::ref_ptr<RecompileCallback> s_recompileCallback = new RecompileCallback;
    if(val){
        setUpdateCallback(s_recompileCallback);
    } else {
        setUpdateCallback(NULL);
    }
}

bool RCBumpMapping_readLocalData(osg::Object &obj, osgDB::Input &fr);
bool RCBumpMapping_writeLocalData(const osg::Object &obj, osgDB::Output &fw);


REGISTER_OBJECT_WRAPPER(osg_ibr_RCBumpMapping,
                        new osg_ibr::RCBumpMapping,
                        osg_ibr::RCBumpMapping,
                        "osg::Object osg::Drawable osg_ibr::RCBumpMapping osg::Geometry" )
{
    ADD_INT_SERIALIZER( DiffuseTextureUnit, 0 );
    ADD_INT_SERIALIZER( NormalMapTextureUnit, 1 );
    ADD_BOOL_SERIALIZER(InputGamma, false);
    ADD_BOOL_SERIALIZER(OutputGamma, false);
}

REGISTER_DOTOSGWRAPPER(RCBumpMapping)
(
	new osg_ibr::RCBumpMapping,
	"RCBumpMapping",
	"Object Drawable RCBumpMapping Geometry",
	&RCBumpMapping_readLocalData,
	&RCBumpMapping_writeLocalData
);

bool RCBumpMapping_readLocalData(osg::Object &obj, osgDB::Input &fr)
{
    RCBumpMapping &myobj = static_cast<RCBumpMapping &>(obj);
    bool itAdvanced = false;


    if (fr[0].matchWord("diffuseTextureUnit")) {
        int n;
        if (fr[1].getInt(n)) {
            myobj.setDiffuseTextureUnit(n);
            fr += 2;
            itAdvanced = true;
        }
    }

    if (fr[0].matchWord("normalMapTextureUnit")) {
        int n;
        if (fr[1].getInt(n)) {
            myobj.setNormalMapTextureUnit(n);
            fr += 2;
            itAdvanced = true;
        }
    }

	if (fr[0].matchWord("inputGamma")) {
        int n;
        if (fr[1].getInt(n)) {
            myobj.setInputGamma((n!=0));
            fr += 2;
            itAdvanced = true;
        }
    }

	if (fr[0].matchWord("outputGamma")) {
        int n;
        if (fr[1].getInt(n)) {
            myobj.setOutputGamma((n!=0));
            fr += 2;
            itAdvanced = true;
        }
    }

    return itAdvanced;
}

bool RCBumpMapping_writeLocalData(const osg::Object &obj, osgDB::Output &fw)
{
    const RCBumpMapping &myobj = static_cast<const RCBumpMapping &>(obj);

    fw.indent() << "diffuseTextureUnit " << myobj.getDiffuseTextureUnit() << "\n";
    fw.indent() << "normalMapTextureUnit " << myobj.getNormalMapTextureUnit() << "\n";
	fw.indent() << "inputGamma " << myobj.getInputGamma() << "\n";
	fw.indent() << "outputGamma " << myobj.getOutputGamma() << "\n";
   
    return true;
}
