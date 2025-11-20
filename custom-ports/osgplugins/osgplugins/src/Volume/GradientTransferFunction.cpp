#include <iostream>
#include <string>

#include <osg/Version>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/io_utils>

#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Output>
#include <osgDB/ParameterOutput>

#include "GradientTransferFunction.h"

using namespace osg_ibr;

float signDistance(const osg::Vec2 &p1,const osg::Vec2 &p2,const osg::Vec2 &p3){
	return ((p2[0]-p1[0])*(p3[1]-p2[1])-(p2[1]-p1[1])*(p3[0]-p2[0]));
}

bool GradientTransferFunction::Quad::getColor(float x, float y, osg::Vec4 &outColor){
	osg::Vec4 distances;
	osg::Vec2 p = osg::Vec2(x,y);
	distances[0] = signDistance(p,_points[1],_points[0])/(_points[1]-_points[0]).length();
	distances[1] = signDistance(p,_points[2],_points[1])/(_points[2]-_points[1]).length();
	distances[2] = signDistance(p,_points[3],_points[2])/(_points[3]-_points[2]).length();
	distances[3] = signDistance(p,_points[0],_points[3])/(_points[0]-_points[3]).length();
	if(distances[0] > -0.0001f && distances[1] > -0.0001f && distances[2] > -0.0001f && distances[3] > -0.0001f){
		switch(_gradientMode){
		case(Standard):
			{
				outColor =_color;
				return true;
			}
			break;
		case(Circle):
			{
				float rationX = 1.0f-2.0f*osg::minimum(distances[0],distances[2])/(distances[0]+distances[2]);
				float rationY = 1.0f-2.0f*osg::minimum(distances[1],distances[3])/(distances[1]+distances[3]);
				outColor.set(_color[0],_color[1],_color[2],(1.0f-sqrtf(rationX*rationX+rationY*rationY))*_color[3]);
				return true;
				break;
			}
		case(Gradient):
			{
				float rationX = 2.0f*osg::minimum(distances[0],distances[2])/(distances[0]+distances[2]);
				outColor.set(_color[0],_color[1],_color[2],rationX*_color[3]);
				return true;
			}
			break;
		case(Rainbow):
			{
				float rationX = 6.0f*distances[0]/(distances[0]+distances[2]);
				float rationY = distances[1]/(distances[1]+distances[3]);
				if(rationX < 1.0){
					outColor.set(1.0f,rationX,0.0f,rationY*_color[3]);
				} else if(rationX < 2.0){
					outColor.set(1.0f-(rationX-1.0f),1.0f,0.0f,rationY*_color[3]);
				} else if(rationX < 3.0){
					outColor.set(0.0f,1.0f,(rationX-2.0),rationY*_color[3]);
				} else if(rationX < 4.0){
					outColor.set(0.0f,1.0f-(rationX-3.0),1.0f,rationY*_color[3]);
				} else if(rationX < 5.0){
					outColor.set((rationX-4.0),0.0f,1.0f,rationY*_color[3]);
				} else {
					outColor.set(1.0f,0.0f,1.0f-(rationX-5.0),rationY*_color[3]);
				}
				return true;
			}
			break;
		};
		return false;
	} else {
		return false;
	}
}

GradientTransferFunction::GradientTransferFunction()
{
	allocate(osg::Vec2ui(512,256));
}

GradientTransferFunction::GradientTransferFunction(const GradientTransferFunction& tf, const osg::CopyOp& copyop):
    TransferFunction(tf,copyop)
{
	allocate(tf.getNumberImageCells());
	_baseColor = tf._baseColor;
    assign(tf._quadMap);
}

void GradientTransferFunction::allocate(const osg::Vec2ui &cells)
{
    if(!_image.valid()) _image = new osg::Image;
    _image->allocateImage(cells[0],cells[1],1,GL_RGBA, GL_FLOAT);
    if (!_quadMap.empty()) assign(_quadMap);
}

void GradientTransferFunction::clear(const osg::Vec4& color)
{
    _baseColor = color;
	_quadMap.clear();

    updateImage();
}

void GradientTransferFunction::assign(const QuadMap& quadMap,  bool updateImg,const osg::Vec4& color)
{
    _quadMap = quadMap;
	_baseColor = color;
	if(updateImg){
		updateImage();
	}
}

void GradientTransferFunction::addQuad(const Quad &quad, bool updateImg){
	_quadMap.push_back(quad);
	if(updateImg){
		updateImage();
	}
}

void GradientTransferFunction::updateImage()
{
	if (!_image || _image->data()==0) allocate(osg::Vec2ui(512,256));

    osg::Vec4* imageData = reinterpret_cast<osg::Vec4*>(_image->data());

	float x = 0.0f;
	float y = 0.0f;
	float xStep = 1.0f/((float)_image->s());
	float yStep = 1.0f/((float)_image->t());
	for(int t = 0; t < _image->t(); y+=yStep,t++){
		x = 0.0f;
		for(int s = 0; s < _image->s(); x+=xStep, s++){
			osg::Vec4 color = _baseColor;
			unsigned int hits = 0;
			for(QuadMap::iterator it = _quadMap.begin(); it != _quadMap.end(); ++it){
				Quad &q = *it;
				osg::Vec4 quadColor;
				if(q.getColor(x,y,quadColor)){
					color += quadColor;
					hits++;
				}
			}
			if(hits==0){
				hits = 1;
			}
			color /= (float)hits;
			imageData[s+_image->s()*t] = color;

		}
	}

    _image->dirty();
}


bool GradientTransferFunction_readLocalData(osg::Object& obj, osgDB::Input &fr) {
    osg_ibr::GradientTransferFunction& tf = static_cast<osg_ibr::GradientTransferFunction&>(obj);

    bool itrAdvanced = false;

    unsigned int resolutionX, resolutionY;
    if(fr.read("Resolution", resolutionX, resolutionY)) {
        tf.allocate(osg::Vec2ui(resolutionX, resolutionY));
        itrAdvanced = true;
    }

    if(fr.matchSequence("Quads {")) {

        int entry = fr[0].getNoNestedBrackets();
        fr += 2;

        while(!fr.eof() && fr[0].getNoNestedBrackets()>entry) {
            if(fr.matchSequence("Quad {")) {
                int entry2 = fr[0].getNoNestedBrackets();
                fr += 2;

                osg_ibr::GradientTransferFunction::Quad q;
                while(!fr.eof() && fr[0].getNoNestedBrackets()>entry2){
                    std::string mode;
                    if(fr.read("mode", mode)){
                        if(mode.compare("Standard") == 0){
                            q._gradientMode = osg_ibr::GradientTransferFunction::Quad::Standard;
                        } else if(mode.compare("Circle") == 0){
                            q._gradientMode = osg_ibr::GradientTransferFunction::Quad::Circle;
                        } else if(mode.compare("Gradient") == 0){
                            q._gradientMode = osg_ibr::GradientTransferFunction::Quad::Gradient;
                        } else {
                            q._gradientMode = osg_ibr::GradientTransferFunction::Quad::Rainbow;
                        }
                    }
                    if(fr.read("color", q._color.r(), q._color.g(), q._color.b(), q._color.a()) &&
                       fr.read("point0", q._points[0].x(), q._points[0].y()) &&
                       fr.read("point1", q._points[1].x(), q._points[1].y()) &&
                       fr.read("point2", q._points[2].x(), q._points[2].y()) &&
                       fr.read("point3", q._points[3].x(), q._points[3].y())){
                        tf.addQuad(q, false);
                    }
                }
            } else {
                ++fr;
            }
            tf.updateImage();
        }
        itrAdvanced = true;
    }


    return itrAdvanced;
}

bool GradientTransferFunction_writeLocalData(const osg::Object& obj, osgDB::Output& fw) {
    const osg_ibr::GradientTransferFunction& tf = static_cast<const osg_ibr::GradientTransferFunction&>(obj);
    const osg_ibr::GradientTransferFunction::QuadMap& quadMap = tf.getColorMap();

    fw.indent()<<"Resolution "<<tf.getNumberImageCells().x() << " " << tf.getNumberImageCells().y() <<std::endl;
    fw.indent()<<"Quads {"<<std::endl;

    fw.moveIn();
    for(osg_ibr::GradientTransferFunction::QuadMap::const_iterator itr = quadMap.begin();
        itr != quadMap.end();
        ++itr)
    {
        const osg_ibr::GradientTransferFunction::Quad& q = *itr;
        fw.indent()<<"Quad {"<<std::endl;
        fw.moveIn();

        switch(q._gradientMode){
            case(osg_ibr::GradientTransferFunction::Quad::Standard) :
                fw.indent()<<"mode Standard"<<std::endl;
                break;
            case(osg_ibr::GradientTransferFunction::Quad::Circle) :
                fw.indent()<<"mode Circle"<<std::endl;
                break;
            case(osg_ibr::GradientTransferFunction::Quad::Gradient) :
                fw.indent()<<"mode Gradient"<<std::endl;
                break;
            case(osg_ibr::GradientTransferFunction::Quad::Rainbow) :
                fw.indent()<<"mode Rainbow"<<std::endl;
                break;
        }

        fw.indent()<<"color"<<" "<<q._color.r()<<" "<<q._color.g()<<" "<<q._color.b()<<" "<<q._color.a()<<std::endl;
        fw.indent()<<"point0"<<" "<<q._points[0].x()<<" "<<q._points[0].y()<<std::endl;
        fw.indent()<<"point1"<<" "<<q._points[1].x()<<" "<<q._points[1].y()<<std::endl;
        fw.indent()<<"point2"<<" "<<q._points[2].x()<<" "<<q._points[2].y()<<std::endl;
        fw.indent()<<"point3"<<" "<<q._points[3].x()<<" "<<q._points[3].y()<<std::endl;

        fw.moveOut();
        fw.indent()<<"}"<<std::endl;
    }
    fw.moveOut();
    fw.indent()<<"}"<<std::endl;

    return true;
}

REGISTER_DOTOSGWRAPPER(TransferFunction1D_Proxy)
(
    new osg_ibr::GradientTransferFunction,
    "GradientTransferFunction",
    "Object GradientTransferFunction",
    GradientTransferFunction_readLocalData,
    GradientTransferFunction_writeLocalData
);

static bool checkQuadMap(const osg_ibr::GradientTransferFunction& tf) {
    return true;
}

static bool readQuadMap(osgDB::InputStream& is, osg_ibr::GradientTransferFunction& tf) {

    osg::Vec2ui resolution;
    is >> is.PROPERTY("Quads") >> resolution;
    resolution[0] = osg::clampBetween(resolution[0], 2u, 1024u);
    resolution[1] = osg::clampBetween(resolution[1], 2u, 1024u);
    tf.allocate(resolution);

    unsigned int size = is.readSize();
    is >> is.BEGIN_BRACKET;
    for(unsigned int i = 0; i < size; i++){
        is >> is.PROPERTY("Quad") >> is.BEGIN_BRACKET;
        osg_ibr::GradientTransferFunction::Quad q;
        is >> is.PROPERTY("mode");

        std::string mode;
        is.readWrappedString(mode);

        if(mode.compare("Standard") == 0){
            q._gradientMode = osg_ibr::GradientTransferFunction::Quad::Standard;
        } else if(mode.compare("Circle") == 0){
            q._gradientMode = osg_ibr::GradientTransferFunction::Quad::Circle;
        } else if(mode.compare("Gradient") == 0){
            q._gradientMode = osg_ibr::GradientTransferFunction::Quad::Gradient;
        } else {
            q._gradientMode = osg_ibr::GradientTransferFunction::Quad::Rainbow;
        }

        is >> is.PROPERTY("color") >> q._color.r() >> q._color.g() >> q._color.b() >> q._color.a();
        is >> is.PROPERTY("point0") >> q._points[0].x() >> q._points[0].y();
        is >> is.PROPERTY("point1") >> q._points[1].x() >> q._points[1].y();
        is >> is.PROPERTY("point2") >> q._points[2].x() >> q._points[2].y();
        is >> is.PROPERTY("point3") >> q._points[3].x() >> q._points[3].y();

        is >> is.END_BRACKET;
        tf.addQuad(q, false);
    }
    is >> is.END_BRACKET;
    tf.updateImage();


    return true;
}

static bool writeQuadMap(osgDB::OutputStream& os, const osg_ibr::GradientTransferFunction& tf) {
    const osg_ibr::GradientTransferFunction::QuadMap& quadMap = tf.getColorMap();
#if(OSG_VERSION_LESS_THAN(3,3,1))
	os << os.PROPERTY("Quads") << tf.getNumberImageCells().x() << tf.getNumberImageCells().y();
#else
	//OutputStream::operator<<(const osg::Vec2ui& v)  added to osg r13788
	os << os.PROPERTY("Quads") << tf.getNumberImageCells();
#endif
	os.writeSize(quadMap.size());
    os << os.BEGIN_BRACKET << std::endl;
    
    for(osg_ibr::GradientTransferFunction::QuadMap::const_iterator itr = quadMap.begin();
        itr != quadMap.end();
        ++itr)
    {
        const osg_ibr::GradientTransferFunction::Quad& q = *itr;
        os << os.PROPERTY("Quad") << os.BEGIN_BRACKET <<std::endl;
        os << os.PROPERTY("mode");
        switch(q._gradientMode){
            case(osg_ibr::GradientTransferFunction::Quad::Standard) :
                os.writeWrappedString("Standard"); 
                break;
            case(osg_ibr::GradientTransferFunction::Quad::Circle) :
                os.writeWrappedString("Circle");
                break;
            case(osg_ibr::GradientTransferFunction::Quad::Gradient) :
                os.writeWrappedString("Gradient");
                break;
            case(osg_ibr::GradientTransferFunction::Quad::Rainbow) :
                os.writeWrappedString("Rainbow");
                break;
        }
        os <<std::endl;

        os << os.PROPERTY("color") <<q._color.r()<<q._color.g()<<q._color.b()<<q._color.a()<<std::endl;
        os << os.PROPERTY("point0") <<q._points[0].x()<<q._points[0].y()<<std::endl;
        os << os.PROPERTY("point1") <<q._points[1].x()<<q._points[1].y()<<std::endl;
        os << os.PROPERTY("point2") <<q._points[2].x()<<q._points[2].y()<<std::endl;
        os << os.PROPERTY("point3") <<q._points[3].x()<<q._points[3].y()<<std::endl;

        os << os.END_BRACKET <<std::endl;
    }
    os << os.END_BRACKET <<std::endl;

    return true;
}

REGISTER_OBJECT_WRAPPER(osg_ibr_GradientTransferFunction,
                        new osg_ibr::GradientTransferFunction(),
                        osg_ibr::GradientTransferFunction,
                        "osg::Object osg_ibr::GradientTransferFunction") {
    ADD_USER_SERIALIZER(QuadMap);
}