#include <stdio.h>

#include <osg/ArgumentParser>
#include <osg/ApplicationUsage>
#include <osg/Group>
#include <osg/Notify>
#include <osg/Vec3>
#include <osg/ProxyNode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/BlendFunc>
#include <osg/Timer>

#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/PluginQuery>
#include <osgDB/FileUtils>

#include <osgUtil/Optimizer>
#include <osgUtil/Simplifier>
#include <osgUtil/SmoothingVisitor>

#include <osgViewer/GraphicsWindow>
#include <osgViewer/Version>
#include <osg/TexMat>
#include <osg/TexEnv>
#include <osg/PagedLOD>
#include <osg/UserDataContainer>
#include <osg/Version>
#include <osgAnimation/RigGeometry>
#include <osgUtil/UpdateVisitor>
#include <osg/AlphaFunc>

#include <iomanip>
#include <sstream>
#include <iostream>

#include "OrientationConverter.h"

typedef std::vector<std::string> FileNameList;

	class setNodeMaskVisitor : public osg::NodeVisitor
	{
	public:
		setNodeMaskVisitor() : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), _removeTexMat(true), _removeDefaultModes(0), _setDataVarianceStatic(true) {
			newStateSet = new osg::StateSet();
		}
		bool hasNonIdentityTexMat(osg::StateSet* ss) {
			osg::StateAttribute *tmsa = ss->getTextureAttribute(0,osg::StateAttribute::TEXMAT);
			if (tmsa == NULL) return false;
			bool remove = true;
			if (osg::TexMat *tm = dynamic_cast<osg::TexMat*>(tmsa)) {
				remove = tm->getMatrix().isIdentity();
			}
			if (remove) ss->removeTextureAttribute(0,tmsa);
			else return true; // found a non identity matrix
			return false;
		}
		bool cleanStateSet(osg::StateSet* ss) {
			ss->setDataVariance(osg::Object::STATIC);
			ss->setUserDataContainer(NULL);
			if (ss->getAttribute(osg::StateAttribute::LIGHTMODEL) != NULL) {
				ss->removeAttribute(osg::StateAttribute::LIGHTMODEL);
				osg::notify(osg::NOTICE)<<"removed osg::StateAttribute::LIGHTMODEL"<<std::endl;
			}
			if (_removeDefaultModes) {
				if (ss->getMode(GL_CULL_FACE) == osg::StateAttribute::ON) {
					ss->removeMode(GL_CULL_FACE);
				}
				if (ss->getMode(GL_LIGHTING) == osg::StateAttribute::ON) {
					ss->removeMode(GL_LIGHTING);
				}
				if (ss->getMode(GL_DEPTH_TEST) == osg::StateAttribute::ON) {
					ss->removeMode(GL_DEPTH_TEST);
				}
				ss->removeMode(GL_NORMALIZE);
			}
			return (newStateSet->compare(*ss) == 0);
		}
		virtual void apply(osg::Node &node) {
			node.setNodeMask(~0u);
			if (_setDataVarianceStatic && !node.getUpdateCallback())
				node.setDataVariance(osg::Object::STATIC);
			node.setUserDataContainer(NULL);
			node.setName(NULL);
//			if (osg::StateSet* ss = node.getStateSet()) ss->setDataVariance(osg::Object::STATIC);
			bool found_texMat = false;
			if (osg::StateSet* ss = node.getStateSet()) {
				if (cleanStateSet(ss)) node.setStateSet(NULL);
				if (_removeTexMat) {
					found_texMat = hasNonIdentityTexMat(ss);
					_removeTexMat = false;//actually i think you cannot inherit a texmat, but not I am not sure. So let's keep the texMats below a non I texMat intact
				}
			}
			if (osg::Geode *geode = dynamic_cast<osg::Geode*>(&node)) {
				for (unsigned int drawableIndex = 0; drawableIndex < geode->getNumDrawables(); ++drawableIndex) {
					osg::Drawable *drwbl = geode->getDrawable(drawableIndex);
					drwbl->setDataVariance(osg::Object::STATIC);
					drwbl->setUserDataContainer(NULL);
					drwbl->setName(NULL);
					//if (osg::StateSet* ss = drwbl->getStateSet()) ss->setDataVariance(osg::Object::STATIC);
					if (osg::StateSet* ss = drwbl->getStateSet()) {
						if (cleanStateSet(ss)) drwbl->setStateSet(NULL);
						if (_removeTexMat) hasNonIdentityTexMat(ss);
					}
				}
			}
			traverse(node);
			if (found_texMat) _removeTexMat = true;
		}
		void setRemodeDefaultModes(int on) { _removeDefaultModes = on; }
		int getRemodeDefaultModes() { return _removeDefaultModes;}
		void setDataVarianceStatic(bool value) { _setDataVarianceStatic = value; }
		bool getDataVarianceStatic() { return _setDataVarianceStatic; }
	private:
		bool _removeTexMat;
		int _removeDefaultModes;
		osg::ref_ptr<osg::StateSet> newStateSet;
		bool _setDataVarianceStatic;
	};

class DummyImgReader : public osgDB::ReaderWriter
{
public:
    DummyImgReader() { supportsExtension("dummy","Any image format"); }
    virtual const char* className() const { return "Dummy Image Reader"; }
    virtual ReadResult readImage(const std::string& file, const osgDB::ReaderWriter::Options* ) const {
			ReadResult rr = new osg::Image();
			rr.getImage()->setFileName(file);
			return rr;
	}
    virtual ReadResult readImage(std::istream& , const Options* options) const {	return readImage(std::string(),options); }
};
//REGISTER_OSGPLUGIN(dummy, DummyImgReader)//warning: if this line is enabled the executable will NEVER be able to read ANY images!

class setInitialBoundGeometryVisitor
	: public osg::NodeVisitor
{
public:
	setInitialBoundGeometryVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) { }
	virtual void apply(osg::Geode & geode)
	{
		for (unsigned int ii = 0; ii < geode.getNumDrawables(); ++ii)
		{
			osg::ref_ptr< osg::Geometry > geometry = dynamic_cast< osg::Geometry * >(geode.getDrawable(ii));
			if (geometry.valid())
			{
#if OSG_VERSION_LESS_THAN(3,3,2)
				geometry->setInitialBound(geometry->getBound());
#else
				//getBoundingBox introduced in osg r 14195 (3.2.2) older versions return a box on getBound()
				geometry->setInitialBound(geometry->getBoundingBox());
#endif
				osg::Drawable::ComputeBoundingBoxCallback *cbbcb = new osg::Drawable::ComputeBoundingBoxCallback();
				geometry->setComputeBoundingBoxCallback(cbbcb);
			}
		}
	}
	virtual void apply(osg::Node & node) { traverse(node); }
};

class MyGraphicsContext {
    public:
        MyGraphicsContext()
        {
            osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
            traits->x = 0;
            traits->y = 0;
            traits->width = 1;
            traits->height = 1;
            traits->windowDecoration = false;
            traits->doubleBuffer = false;
            traits->sharedContext = 0;
            traits->pbuffer = true;

            _gc = osg::GraphicsContext::createGraphicsContext(traits.get());

            if (!_gc)
            {
                osg::notify(osg::NOTICE)<<"Failed to create pbuffer, failing back to normal graphics window."<<std::endl;

                traits->pbuffer = false;
                _gc = osg::GraphicsContext::createGraphicsContext(traits.get());
            }

            if (_gc.valid())
            {
                _gc->realize();
                _gc->makeCurrent();
                if (dynamic_cast<osgViewer::GraphicsWindow*>(_gc.get()))
                {
                    std::cout<<"Realized graphics window for OpenGL operations."<<std::endl;
                }
                else
                {
                    std::cout<<"Realized pbuffer for OpenGL operations."<<std::endl;
                }
            }
        }

        bool valid() const { return _gc.valid() && _gc->isRealized(); }

    private:
        osg::ref_ptr<osg::GraphicsContext> _gc;
};

class DefaultNormalsGeometryVisitor
    : public osg::NodeVisitor
{
public:

    DefaultNormalsGeometryVisitor()
        : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) {
    }

    virtual void apply( osg::Geode & geode )
    {
        for( unsigned int ii = 0; ii < geode.getNumDrawables(); ++ii )
        {
            osg::ref_ptr< osg::Geometry > geometry = dynamic_cast< osg::Geometry * >( geode.getDrawable( ii ) );
            if( geometry.valid() )
            {
                osg::ref_ptr< osg::Vec3Array > newnormals = new osg::Vec3Array;
                newnormals->push_back( osg::Z_AXIS );
                geometry->setNormalArray( newnormals.get(), osg::Array::BIND_OVERALL );
            }
        }
    }

    virtual void apply( osg::Node & node )
    {
        traverse( node );
    }

};

class CompressTexturesVisitor : public osg::NodeVisitor
{
public:

    CompressTexturesVisitor(osg::Texture::InternalFormatMode internalFormatMode):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _internalFormatMode(internalFormatMode) {}

    virtual void apply(osg::Node& node)
    {
        if (node.getStateSet()) apply(*node.getStateSet());
        traverse(node);
    }

    virtual void apply(osg::Geode& node)
    {
        if (node.getStateSet()) apply(*node.getStateSet());

        for(unsigned int i=0;i<node.getNumDrawables();++i)
        {
            osg::Drawable* drawable = node.getDrawable(i);
            if (drawable && drawable->getStateSet()) apply(*drawable->getStateSet());
        }

        traverse(node);
    }

    virtual void apply(osg::StateSet& stateset)
    {
        // search for the existence of any texture object attributes
        for(unsigned int i=0;i<stateset.getTextureAttributeList().size();++i)
        {
            osg::Texture* texture = dynamic_cast<osg::Texture*>(stateset.getTextureAttribute(i,osg::StateAttribute::TEXTURE));
            if (texture)
            {
                _textureSet.insert(texture);
            }
        }
    }

    void compress()
    {
        MyGraphicsContext context;
        if (!context.valid())
        {
            osg::notify(osg::NOTICE)<<"Error: Unable to create graphis context, problem with running osgViewer-"<<osgViewerGetVersion()<<", cannot run compression."<<std::endl;
            return;
        }

        osg::ref_ptr<osg::State> state = new osg::State;
#if OSG_VERSION_GREATER_OR_EQUAL(3,4,0)
		state->initializeExtensionProcs();
#endif

        for(TextureSet::iterator itr=_textureSet.begin();
            itr!=_textureSet.end();
            ++itr)
        {
            osg::Texture* texture = const_cast<osg::Texture*>(itr->get());

            osg::Texture2D* texture2D = dynamic_cast<osg::Texture2D*>(texture);
            osg::Texture3D* texture3D = dynamic_cast<osg::Texture3D*>(texture);

            osg::ref_ptr<osg::Image> image = texture2D ? texture2D->getImage() : (texture3D ? texture3D->getImage() : 0);
            if (image.valid()) {
                if (image->s() % 4 || image->t() % 4) {
                    osg::notify(osg::NOTICE) << "Warning: image " << image->getFileName() << " has dimensions not a multiple of 4: " << image->s() << " x " << image->t() << std::endl;
                }
            }
            if (image.valid() &&
                    (image->getPixelFormat() == GL_RGB || image->getPixelFormat() == GL_RGBA) &&
                    !image->isCompressed())// &&
                //(image->s()>=32 && image->t()>=32))
            {
                texture->setInternalFormatMode(_internalFormatMode);

                // need to disable the unref after apply, otherwise the image could go out of scope.
                bool unrefImageDataAfterApply = texture->getUnRefImageDataAfterApply();
                texture->setUnRefImageDataAfterApply(false);

                // get OpenGL driver to create texture from image.
                texture->apply(*state);

                // restore the original setting
                texture->setUnRefImageDataAfterApply(unrefImageDataAfterApply);

                image->readImageFromCurrentTexture(0,true);

                texture->setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);
            }
        }
    }
	void setFilter(osg::Texture::FilterParameter which, osg::Texture::FilterMode filter)
	{
		for (TextureSet::iterator itr = _textureSet.begin(); itr != _textureSet.end(); ++itr) {
			osg::Texture* texture = const_cast<osg::Texture*>(itr->get());
			texture->setFilter(which, filter);
		}
	}
	void setUseHardwareMipMapGeneration(bool val)
	{
		for (TextureSet::iterator itr = _textureSet.begin(); itr != _textureSet.end(); ++itr) {
			osg::Texture* texture = const_cast<osg::Texture*>(itr->get());
			texture->setUseHardwareMipMapGeneration(val);
		}
	}
	void write(std::string &write_textures_ext, std::string &basename, bool rename)
    {
		int textureCounter = 0;
		typedef std::map<std::string, osg::ref_ptr<osg::Image> > ImageNameMap;
		ImageNameMap imageNameMap;
        for(TextureSet::iterator itr=_textureSet.begin();
            itr!=_textureSet.end();
            ++itr)
        {
            osg::Texture* texture = const_cast<osg::Texture*>(itr->get());
            
            osg::Texture2D* texture2D = dynamic_cast<osg::Texture2D*>(texture);
            osg::Texture3D* texture3D = dynamic_cast<osg::Texture3D*>(texture);
            
            osg::ref_ptr<osg::Image> image = texture2D ? texture2D->getImage() : (texture3D ? texture3D->getImage() : 0);
			if (image.valid() )
            {
				std::string ImgFileName;
				std::string ImgBaseName;
				std::string ImgFileExt;
				if (write_textures_ext.empty()) {
					ImgFileExt = "." + osgDB::getFileExtension(image->getFileName());
					if (ImgFileExt.empty()) ImgFileExt = ".ktx";
				} else {
					ImgFileExt = "." + write_textures_ext;
				}
				if (rename || image->getFileName().empty()) {
					do {
							std::stringstream generatedFilename;
							generatedFilename << basename << std::setfill('0') << std::setw(4) << textureCounter++;
							ImgBaseName = generatedFilename.str();
						ImgFileName = ImgBaseName + ImgFileExt;
					} while (imageNameMap.find(ImgFileName) != imageNameMap.end());
					--textureCounter;
				} else {
					int dupeCounter = 0;
					do {
						if (dupeCounter++ > 0) {
							std::stringstream generatedFilename;
							generatedFilename << osgDB::getNameLessExtension(image->getFileName()) << "_0" << dupeCounter;
							ImgBaseName = generatedFilename.str();
						} else {
							ImgBaseName = osgDB::getNameLessExtension(image->getFileName());
						}
						ImgFileName = ImgBaseName + ImgFileExt;
					} while (imageNameMap.find(ImgFileName) != imageNameMap.end());
				}

				imageNameMap[ImgFileName] = image;
//				std::string ImgFileName(image->getFileName());
//
//				if (rename || ImgFileName.empty()) {
//					std::stringstream generatedFilename;
//					generatedFilename << basename << std::setfill ('0') << std::setw(3) <<textureCounter << "." << write_textures_ext;
//                    if (write_textures_ext.empty()) {
//                        generatedFilename << osgDB::getFileExtension(ImgFileName);
//                    }
//					ImgFileName =  generatedFilename.str();
//				} else {
//					ImgFileName = osgDB::getNameLessExtension(ImgFileName) + "." + write_textures_ext;
//				}

				image->setFileName(ImgFileName);
				GLenum pixelFormat = image->getPixelFormat();
				bool compressed = (pixelFormat >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT && pixelFormat <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
				bool writeCompressed = ImgFileExt == std::string(".dds") || ImgFileExt == std::string(".ktx") || ImgFileExt == std::string(".vtf");
				osg::ref_ptr<osg::Image> writeImage = image;//no conversion
				bool NPOT = ((image->s() & (image->s() - 1)) != 0) || ((image->t() & (image->t() - 1)) != 0);
				NPOT = compressed && (((image->s() & 3) != 0) || ((image->t() & 3) != 0));
				if (NPOT) {
					writeCompressed = false;//decompress to be able to resize in ensureValidSizeForTexturing
				}
				if (compressed && !writeCompressed) {
					GLenum pixFormat = image->isImageTranslucent() ? GL_RGBA : GL_RGB;
					osg::ref_ptr<osg::Image> uncompressedImage = new osg::Image();
					uncompressedImage->allocateImage(image->s(), image->t(), image->r(), pixFormat, GL_UNSIGNED_BYTE);
					for (int y = 0; y< image->t(); ++y) {
						for (int x = 0; x < image->s(); ++x) {
							osg::Vec4 color = image->getColor(x, y, 0);
							uncompressedImage->setColor(color, x, y, 0);
						}
					}
					writeImage = uncompressedImage;
				}
				if (NPOT) {
					writeImage->ensureValidSizeForTexturing(8192);//resize to Power of Two
					osg::Image::MipmapDataType emptyMipMaps;
					writeImage->setMipmapLevels(emptyMipMaps);//clear mipmaps
				}
				osgDB::makeDirectoryForFile(ImgFileName);
				osgDB::writeImageFile(*writeImage.get(), ImgFileName, osgDB::Registry::instance()->getOptions());
            }
			++textureCounter;
        }
    }
	osg::ref_ptr<osg::Node> writeNodes()
    {
		osg::ref_ptr<osg::ProxyNode> proxyGroup = new osg::ProxyNode();
        for(TextureSet::iterator itr=_textureSet.begin();
            itr!=_textureSet.end();
            ++itr)
        {
            osg::Texture* texture = const_cast<osg::Texture*>(itr->get());
            
            osg::Texture2D* texture2D = dynamic_cast<osg::Texture2D*>(texture);
            osg::Texture3D* texture3D = dynamic_cast<osg::Texture3D*>(texture);

            osg::ref_ptr<osg::Image> image = texture2D ? texture2D->getImage() : (texture3D ? texture3D->getImage() : 0);
            if (image.valid())
            {
				osg::ref_ptr<osg::Group> group = new osg::Group();
				const std::string imgFileName(image->getFileName());
				//set img extention:
				std::string nodeFileName = osgDB::getStrippedName(imgFileName) + "_geom.osg";
				bool strip_stateset = false;
				if (atoi(osgDB::getStrippedName(imgFileName).c_str())) strip_stateset = true;
//				printf(nodeFileName.c_str());
				const osg::StateAttribute::ParentList parentList = texture->getParents();
				for(osg::StateAttribute::ParentList::const_iterator parent=parentList.begin(); parent!=parentList.end(); ++parent)
				{
					const osg::StateSet::ParentList objList= (*parent)->getParents();
					for(osg::StateSet::ParentList::const_iterator obj = objList.begin();obj != objList.end();++obj) {
						//osg::Node or osg::Drawable
						osg::Node *node =dynamic_cast<osg::Node *>(*obj);
						if (node) {
							if (strip_stateset) {
								osg::Node *node2(node);//SHALLOW_COPY
								node2->setStateSet(NULL);
								group->addChild(node2);
							} else {
								group->addChild(node);
							}
						} else {
							osg::Drawable *drwbl =dynamic_cast<osg::Drawable *>(*obj);
							if (drwbl) {
								osg::Geode *geode = new osg::Geode();
								if (strip_stateset) {
									osg::Drawable *drwbl2(drwbl);
									drwbl2->setStateSet(NULL);
									geode->addDrawable(drwbl2);
								} else {
									geode->addDrawable(drwbl);
								}
								group->addChild(geode);
							}
						}
					}
				}
				if (group->getNumChildren() == 1) {
					osgDB::Registry::instance()->writeNode(*group->getChild(0),nodeFileName,osgDB::Registry::instance()->getOptions());
				} else {//0 or multiple: write group
					osgDB::Registry::instance()->writeNode(*group.get(),nodeFileName,osgDB::Registry::instance()->getOptions());
				}
				proxyGroup->setFileName(proxyGroup->getNumFileNames(), nodeFileName);
            }
        }
		return proxyGroup;
    }

    typedef std::set< osg::ref_ptr<osg::Texture> > TextureSet;
    TextureSet                          _textureSet;
    osg::Texture::InternalFormatMode    _internalFormatMode;

};


class FixTransparencyVisitor : public osg::NodeVisitor
{
public:

    enum FixTransparencyMode
    {
        NO_TRANSPARANCY_FIXING,
		MAKE_OPAQUE_TEXTURE_STATESET_OPAQUE,
		MAKE_STATESET_OPAQUE_OR_ALPHA_TEST,
		MAKE_STATESET_OPAQUE_OR_ALPHA_TEST_LIGHTING_OFF,
		MAKE_ALL_STATESET_OPAQUE
    };

    FixTransparencyVisitor(FixTransparencyMode mode=MAKE_OPAQUE_TEXTURE_STATESET_OPAQUE):
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _numTransparent(0),
        _numOpaque(0),
        _numTransparentMadeOpaque(0),
        _mode(mode)
    {
        std::cout<<"Running FixTransparencyVisitor..."<<std::endl;
    }

    ~FixTransparencyVisitor()
    {
        std::cout<<"  Number of Transparent StateSet "<<_numTransparent<<std::endl;
        std::cout<<"  Number of Opaque StateSet "<<_numOpaque<<std::endl;
        std::cout<<"  Number of Transparent State made Opaque "<<_numTransparentMadeOpaque<<std::endl;
    }

    virtual void apply(osg::Node& node)
    {
        if (node.getStateSet()) isTransparent(*node.getStateSet());
        traverse(node);
    }

    virtual void apply(osg::Geode& node)
    {
        if (node.getStateSet()) isTransparent(*node.getStateSet());

        for(unsigned int i=0;i<node.getNumDrawables();++i)
        {
            osg::Drawable* drawable = node.getDrawable(i);
            if (drawable && drawable->getStateSet()) isTransparent(*drawable->getStateSet());
        }

        traverse(node);
    }

    virtual bool isTransparent(osg::StateSet& stateset)
    {
        bool hasTranslucentTexture = false;
        bool hasBlendFunc = dynamic_cast<osg::BlendFunc*>(stateset.getAttribute(osg::StateAttribute::BLENDFUNC))!=0;
        bool hasTransparentRenderingHint = stateset.getRenderingHint()==osg::StateSet::TRANSPARENT_BIN;
        bool hasDepthSortBin = (stateset.getRenderBinMode()==osg::StateSet::USE_RENDERBIN_DETAILS)?(stateset.getBinName()=="DepthSortedBin"):false;
        bool hasTexture = false;


        // search for the existence of any texture object attributes
        for(unsigned int i=0;i<stateset.getTextureAttributeList().size();++i)
        {
            osg::Texture* texture = dynamic_cast<osg::Texture*>(stateset.getTextureAttribute(i,osg::StateAttribute::TEXTURE));
            if (texture)
            {
                hasTexture = true;
                for (unsigned int im=0;im<texture->getNumImages();++im)
                {
                    osg::Image* image = texture->getImage(im);
                    if (image && image->isImageTranslucent()) hasTranslucentTexture = true;
                }
            }
        }

        if (hasTranslucentTexture || hasBlendFunc || hasTransparentRenderingHint || hasDepthSortBin)
        {
            ++_numTransparent;

			bool makeNonTransparent = false;
			bool makeAlphaTest = false;
			bool lightingOff = false;

            switch(_mode)
            {
            case(MAKE_OPAQUE_TEXTURE_STATESET_OPAQUE):
                if (hasTexture && !hasTranslucentTexture)
                {
                    makeNonTransparent = true;
                }
                break;
			case(MAKE_STATESET_OPAQUE_OR_ALPHA_TEST_LIGHTING_OFF) :
				lightingOff = true;
			case(MAKE_STATESET_OPAQUE_OR_ALPHA_TEST):
				if (hasTexture)
				{
					makeNonTransparent = true;
					if (hasTranslucentTexture) makeAlphaTest = true;
				}
				break;
            case(MAKE_ALL_STATESET_OPAQUE):
                makeNonTransparent = true;
                break;
            default:
                makeNonTransparent = false;
                break;
            }

            if (makeNonTransparent)
            {
                stateset.removeAttribute(osg::StateAttribute::BLENDFUNC);
                stateset.removeMode(GL_BLEND);
                stateset.setRenderingHint(osg::StateSet::DEFAULT_BIN);
                ++_numTransparentMadeOpaque;
            }
			if (makeAlphaTest) {
				osg::AlphaFunc *alphaTest = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.5f);
				stateset.setAttributeAndModes(alphaTest);
			}
			if (lightingOff) stateset.setMode(GL_LIGHTING, osg::StateAttribute::OFF);

            return true;
        }
        else
        {
            ++_numOpaque;
            return false;
        }
    }

    unsigned int _numTransparent;
    unsigned int _numOpaque;
    unsigned int _numTransparentMadeOpaque;
    FixTransparencyMode _mode;
};

class PruneStateSetVisitor : public osg::NodeVisitor
{
public:

    PruneStateSetVisitor():
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _numStateSetRemoved(0)
    {
        std::cout<<"Running PruneStateSet..."<<std::endl;
    }

    ~PruneStateSetVisitor()
    {
        std::cout<<"  Number of StateState removed "<<_numStateSetRemoved<<std::endl;
    }

    virtual void apply(osg::Node& node)
    {
        if (node.getStateSet())
        {
            node.setStateSet(0);
            ++_numStateSetRemoved;
        }
        traverse(node);
    }
#if OSG_VERSION_LESS_THAN(3,3,2)
	//versions below svn rev 14195 dont have Drawable inherit from Node
	virtual void apply(osg::Geode& node)
    {
        if (node.getStateSet())
        {
            node.setStateSet(0);
            ++_numStateSetRemoved;
        }
        
        for(unsigned int i=0;i<node.getNumDrawables();++i)
        {
            osg::Drawable* drawable = node.getDrawable(i);
            if (drawable && drawable->getStateSet())
            {
                drawable->setStateSet(0);
                ++_numStateSetRemoved;
            }
        }
        
        traverse(node);
    }
#endif
    unsigned int _numStateSetRemoved;
};

/** Remove colours form osg::Geometry.*/
class RemoveColoursFromGeometryVisitor : public osg::NodeVisitor
{
public:

	RemoveColoursFromGeometryVisitor() :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

	virtual void apply(osg::Geometry& geometry)
	{
		geometry.setColorArray(NULL, osg::Array::BIND_UNDEFINED);
	}

	virtual void apply(osg::Node& node) { traverse(node); }

};

/** Add missing colours to osg::Geometry.*/
class AddMissingColoursToGeometryVisitor : public osg::NodeVisitor
{
public:

    AddMissingColoursToGeometryVisitor():osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

    virtual void apply(osg::Geode& geode)
    {
        for(unsigned int i=0;i<geode.getNumDrawables();++i)
        {
            osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
            if (geometry)
            {
                if (geometry->getColorArray()==0 || geometry->getColorArray()->getNumElements()==0)
                {
                    osg::Vec4Array* colours = new osg::Vec4Array(1);
                    (*colours)[0].set(1.0f,1.0f,1.0f,1.0f);
                    geometry->setColorArray(colours, osg::Array::BIND_OVERALL);
                }
            }
        }
    }

    virtual void apply(osg::Node& node) { traverse(node); }

};
class PagedLODToLODConverter: public osg::NodeVisitor{
public:
	PagedLODToLODConverter():
		NodeVisitor(TRAVERSE_ALL_CHILDREN){
	}

	virtual void apply(osg::Group &group){
		for(unsigned int i = 0; i < group.getNumChildren(); i++){
			osg::PagedLOD * pagedlod = dynamic_cast<osg::PagedLOD*>(group.getChild(i));
			if(pagedlod){
				osg::LOD* lod = convertToLOD(pagedlod);
				group.replaceChild(pagedlod,lod);
			}
		}
		NodeVisitor::apply(group);
	}

	osg::LOD* convertToLOD(osg::PagedLOD *pagedLOD, const osg::CopyOp &copyOp = osg::CopyOp::SHALLOW_COPY){
		printf("converting plod\n");
		osg::LOD* lod = new osg::LOD();
		// copy all the data
		// object data
		lod->setDataVariance(pagedLOD->getDataVariance());
		if (pagedLOD->getUserDataContainer()) lod->setUserDataContainer(osg::clone(pagedLOD->getUserDataContainer(),copyOp));
		lod->setName(pagedLOD->getName());
		// node data
		lod->setInitialBound(pagedLOD->getInitialBound());
		if (pagedLOD->getUpdateCallback()) lod->setUpdateCallback(osg::clone(pagedLOD->getUpdateCallback(),copyOp));
		if (pagedLOD->getCullCallback()) lod->setCullCallback(osg::clone(pagedLOD->getCullCallback(),copyOp));
		lod->setCullingActive(pagedLOD->getCullingActive());
		lod->setNodeMask(pagedLOD->getNodeMask());
		if (pagedLOD->getStateSet()) lod->setStateSet(osg::clone(pagedLOD->getStateSet(),copyOp));
		//lod data
		lod->setCenter(pagedLOD->getCenter());
		lod->setCenterMode(pagedLOD->getCenterMode());
		lod->setRadius(pagedLOD->getRadius());
		lod->setRangeMode(pagedLOD->getRangeMode());
		osg::LOD::RangeList rangeList = pagedLOD->getRangeList();
		for(unsigned int i = 0; i < pagedLOD->getNumFileNames(); i++){
			osg::ref_ptr<osg::Node> newNode;
			if(i < pagedLOD->getNumChildren()){
				newNode = osg::clone(pagedLOD->getChild(i),copyOp);
			} else {
				newNode = osgDB::readRefNodeFile(pagedLOD->getDatabasePath() + pagedLOD->getFileName(i));
			}
			if(newNode){
				if(i < rangeList.size()){
					lod->addChild(newNode,rangeList[i].first,rangeList[i].second);
				} else {
					lod->addChild(newNode);
				}
			}
		}
		return lod;
	}
};

class LODselector : public osg::NodeVisitor {
public:
    LODselector(float distance) :
        NodeVisitor(TRAVERSE_ALL_CHILDREN), _select_distance(distance) {
    }

    virtual void apply(osg::LOD &lod) {
        osg::Group *group = new osg::Group;
        unsigned int numChildren = lod.getNumChildren();
        osg::LOD::RangeList _rangeList = lod.getRangeList();
        float required_range = _select_distance;
        if (lod.getRangeMode() != osg::LOD::DISTANCE_FROM_EYE_POINT) required_range = 2048.0f * lod.getBound().radius() / _select_distance;
        if (_rangeList.size()<numChildren) numChildren = _rangeList.size();
        for (unsigned int i = 0; i < numChildren; ++i)
        {
            if (_rangeList[i].first <= required_range && required_range < _rangeList[i].second) {
                group->addChild(lod.getChild(i));
            }
        }
        while (lod.getNumParents() != 0) {
            lod.getParent(0)->replaceChild(&lod, group);
        }

        NodeVisitor::apply(*group);
    }
    virtual void apply(osg::ProxyNode &proxy) {
        proxy.setLoadingExternalReferenceMode(osg::ProxyNode::LOAD_IMMEDIATELY);

        traverse(proxy);
    }
    float _select_distance;
};

class RigGeometryPrepareVisitor
    : public osg::NodeVisitor
{
public:

    RigGeometryPrepareVisitor()
        : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) {
    }
    virtual void apply(osg::Node& node) { traverse(node); }
    virtual void apply(osg::Geode& node)
    {
        for(unsigned int i=0;i<node.getNumDrawables();++i)
        {
            osg::Drawable* drawable = node.getDrawable(i);
			osgAnimation::RigGeometry *rigGeo = dynamic_cast<osgAnimation::RigGeometry *>(drawable);
			if (rigGeo) {
				//rigGeo->copyFrom(*(new osg::Geometry())); //destroy output array, will be copyed from input on init
				rigGeo->copyFrom(*(rigGeo->getSourceGeometry()));
				unsigned int vertexCount = rigGeo->getVertexArray()->getNumElements();
				if (const osgAnimation::VertexInfluenceMap* vm = rigGeo->getInfluenceMap()) {
#if CALCULATE_REMAINDER
					osg::FloatArray* reaminingWeights = new osg::FloatArray(vertexCount);//NOT (it->second.size());
					reaminingWeights->setName("remainder");
					for ( osg::FloatArray::iterator it = reaminingWeights->begin(); it != reaminingWeights->end(); ++it) (*it) = 1.0f;
					unsigned int runit = rigGeo->getNumVertexAttribArrays();
					rigGeo->setVertexAttribBinding(runit,osg::Geometry::BIND_PER_VERTEX);
					rigGeo->setVertexAttribArray(runit,reaminingWeights);
#endif
					std::vector<osg::FloatArray *> weightsArrays;
					for (osgAnimation::VertexInfluenceMap::const_iterator it = vm->begin(); it != vm->end(); ++it) 
					{
						osg::FloatArray* weights = new osg::FloatArray(vertexCount);//NOT (it->second.size());
						weights->setName(it->first);
						weightsArrays.push_back(weights);
						unsigned int unit = rigGeo->getNumVertexAttribArrays();
						rigGeo->setVertexAttribBinding(unit,osg::Geometry::BIND_PER_VERTEX);
						rigGeo->setVertexAttribArray(unit,weights);
						const osgAnimation::VertexInfluence& vi = it->second;
						for (osgAnimation::VertexInfluence::const_iterator itv = vi.begin(); itv != vi.end(); itv++) 
						{
							(*weights)[itv->first] += itv->second;
#if CALCULATE_REMAINDER
							(*reaminingWeights)[itv->first] -= itv->second;
#endif
						}
					}
					//force 2 main influences to set position
					unsigned int boneCount = weightsArrays.size();
					unsigned int maxBoneCount = 2;
					if (boneCount >= maxBoneCount) {//at least 2 bones
						for (unsigned int i = 0; i < vertexCount; ++i) {
#if OSG_VERSION_LESS_THAN(3,5,10)
							osgAnimation::VertexList heavyBones; //vector of index,weight pairs
#else
							osgAnimation::IndexWeightList heavyBones;
#endif
							unsigned int lightWeightBoneIndex = 0;
							for (unsigned int j = 0; j < maxBoneCount; ++j) {
								heavyBones.push_back( osgAnimation::VertexIndexWeight(j, (*weightsArrays[j])[i]) );
								if (heavyBones[j].second < heavyBones[lightWeightBoneIndex].second) lightWeightBoneIndex = j;
							}
							for (unsigned int j = maxBoneCount; j < boneCount; ++j) {
								float weight = (*weightsArrays[j])[i];
								if (weight > heavyBones[lightWeightBoneIndex].second) {
									heavyBones[lightWeightBoneIndex].first = j;//replace bone with least weight
									heavyBones[lightWeightBoneIndex].second = weight;
									for (unsigned int k = 0; k < maxBoneCount; ++k) { //find new lightWeight
										if (heavyBones[k].second < heavyBones[lightWeightBoneIndex].second) lightWeightBoneIndex = k;
									}
								}
							}
							//normalize weights sum to 1
							float totalWeight = 0.0f;
							for (unsigned int j = 0; j < maxBoneCount; ++j) totalWeight += heavyBones[j].second;
							for (unsigned int j = 0; j < maxBoneCount; ++j) heavyBones[j].second /= totalWeight;
							//write result
							for (unsigned int j = 0; j < boneCount; ++j) (*weightsArrays[j])[i] = 0.0f;
							for (unsigned int j = 0; j < maxBoneCount; ++j) (*weightsArrays[heavyBones[j].first])[i] = heavyBones[j].second;
						}
					}
				}
			}
		}
    }
};
class RigGeometryApplyVisitor
    : public osg::NodeVisitor
{
public:

    RigGeometryApplyVisitor()
        : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) {
    }
    virtual void apply( osg::Node & node )
    {
        traverse( node );
    }
    virtual void apply(osg::Geode& node)
    {
        for(unsigned int i=0;i<node.getNumDrawables();++i)
        {
            osg::Drawable* drawable = node.getDrawable(i);
			osgAnimation::RigGeometry *rigGeo = dynamic_cast<osgAnimation::RigGeometry *>(drawable);
			if (rigGeo) {
				unsigned int unit = rigGeo->getSourceGeometry()->getNumVertexAttribArrays();
				unsigned int vertexCount = rigGeo->getVertexArray()->getNumElements();

				if (osgAnimation::VertexInfluenceMap* vm = rigGeo->getInfluenceMap()) {
#if CALCULATE_REMAINDER
					osgAnimation::VertexInfluence rvi;
					rvi.setName("remainder");
					rvi.resize(vertexCount);
					(*vm)["remainder"] = rvi;
#endif
					for (osgAnimation::VertexInfluenceMap::iterator it = vm->begin(); it != vm->end(); ++it)
					{
						osg::FloatArray* weights = dynamic_cast<osg::FloatArray*>(rigGeo->getVertexAttribArray(unit++));
						osgAnimation::VertexInfluence& vi = it->second;
						vi.resize(vertexCount);
						unsigned int viIndex = 0;
						for (unsigned int vertex = 0; vertex < vertexCount; ++vertex) {
							float weight = (*weights)[vertex];
							if (fabs(weight) > 0.001f) {
								vi[viIndex].first = vertex;
								vi[viIndex].second = weight;
								++viIndex;
							}
						}
						vi.resize(viIndex);
					}
				}
				rigGeo->getVertexAttribArrayList().resize(rigGeo->getSourceGeometry()->getNumVertexAttribArrays());//destoy extra lists
				rigGeo->setSourceGeometry(new osg::Geometry(*rigGeo));

#if 1
				//rigGeo->copyFrom(*(new osg::Geometry())); 
				//destroy output array, will be copyed from input on init
				rigGeo->setStateSet(NULL);
				rigGeo->getPrimitiveSetList().resize(0);
				rigGeo->setVertexArray(NULL);
				rigGeo->setNormalArray(NULL);
				rigGeo->setColorArray(NULL);
				rigGeo->setSecondaryColorArray(NULL);
				rigGeo->setFogCoordArray(NULL);
				rigGeo->getTexCoordArrayList().resize(0);
				rigGeo->getVertexAttribArrayList().resize(0);
#endif
			}
		}
    }
};
static void usage( const char *prog, const char *msg )
{
	osg::setNotifyLevel(osg::NOTICE);
    if (msg)
    {
        osg::notify(osg::NOTICE)<< std::endl;
        osg::notify(osg::NOTICE) << msg << std::endl;
    }

    // basic usage
    osg::notify(osg::NOTICE)<< std::endl;
    osg::notify(osg::NOTICE)<<"usage:"<< std::endl;
    osg::notify(osg::NOTICE)<<"    " << prog << " [options] infile1 [infile2 ...] outfile"<< std::endl;
    osg::notify(osg::NOTICE)<< std::endl;

    // print env options - especially since optimizer is always _on_
    osg::notify(osg::NOTICE)<<"environment:" << std::endl;
    osg::ApplicationUsage::UsageMap um = osg::ApplicationUsage::instance()->getEnvironmentalVariables();
    std::string envstring;
    osg::ApplicationUsage::instance()->getFormattedString( envstring, um );
    osg::notify(osg::NOTICE)<<envstring << std::endl;

    // print tool options
    osg::notify(osg::NOTICE)<<"options:"<< std::endl;
    osg::notify(osg::NOTICE)<<"    -O option          - ReaderWriter option"<< std::endl;
    osg::notify(osg::NOTICE)<< std::endl;
    osg::notify(osg::NOTICE)<<"    --compressed       - Enable the usage of compressed textures,"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         defaults to OpenGL ARB compressed textures."<< std::endl;
    osg::notify(osg::NOTICE)<<"    --compressed-arb   - Enable the usage of OpenGL ARB compressed textures"<< std::endl;
    osg::notify(osg::NOTICE)<<"    --compressed-dxt1  - Enable the usage of S3TC DXT1 compressed textures"<< std::endl;
    osg::notify(osg::NOTICE)<<"    --compressed-dxt3  - Enable the usage of S3TC DXT3 compressed textures"<< std::endl;
    osg::notify(osg::NOTICE)<<"    --compressed-dxt5  - Enable the usage of S3TC DXT5 compressed textures"<< std::endl;
    osg::notify(osg::NOTICE)<< std::endl;
    osg::notify(osg::NOTICE)<<"    --fix-transparency - fix statesets which are currently"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         declared as transparent, but should be opaque."<< std::endl;
    osg::notify(osg::NOTICE)<<"                         Defaults to using the fixTranspancyMode"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         MAKE_OPAQUE_TEXTURE_STATESET_OPAQUE."<< std::endl;
    osg::notify(osg::NOTICE)<<"    --fix-transparency-mode <mode_string>  - fix statesets which are currently"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         declared as transparent but should be opaque."<< std::endl;
    osg::notify(osg::NOTICE)<<"                         The mode_string determines which algorithm is used"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         to fix the transparency, options are:"<< std::endl;
    osg::notify(osg::NOTICE)<<"                                 MAKE_OPAQUE_TEXTURE_STATESET_OPAQUE,"<<std::endl;
	osg::notify(osg::NOTICE) << "                                 MAKE_STATESET_OPAQUE_OR_ALPHA_TEST," << std::endl;
	osg::notify(osg::NOTICE) << "                                 MAKE_STATESET_OPAQUE_OR_ALPHA_TEST_LIGHTING_OFF," << std::endl;
	osg::notify(osg::NOTICE) << "                                 MAKE_ALL_STATESET_OPAQUE." << std::endl;

    osg::notify(osg::NOTICE)<< std::endl;
    osg::notify(osg::NOTICE)<<"    -l libraryName     - load plugin of name libraryName"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         i.e. -l osgdb_pfb"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         Useful for loading reader/writers which can load"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         other file formats in addition to its extension."<< std::endl;
    osg::notify(osg::NOTICE)<<"    -e extensionName   - load reader/wrter plugin for file extension"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         i.e. -e pfb"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         Useful short hand for specifying full library name as"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         done with -l above, as it automatically expands to the"<< std::endl;
    osg::notify(osg::NOTICE)<<"                         full library name appropriate for each platform."<< std::endl;
    osg::notify(osg::NOTICE)<<"    -o orientation     - Convert geometry from input files to output files."<< std::endl;
    osg::notify(osg::NOTICE)<<
                              "                         Format of orientation argument must be the following:\n"
                              "\n"
                              "                             X1,Y1,Z1-X2,Y2,Z2\n"
                              "                         or\n"
                              "                             degrees-A0,A1,A2\n"
                              "\n"
                              "                         where X1,Y1,Z1 represent the UP vector in the input\n"
                              "                         files and X2,Y2,Z2 represent the UP vector of the\n"
                              "                         output file, or degrees is the rotation angle in\n"
                              "                         degrees around axis (A0,A1,A2).  For example, to\n"
                              "                         convert a model built in a Y-Up coordinate system to a\n"
                              "                         model with a Z-up coordinate system, the argument may\n"
                              "                         look like\n"
                              "\n"
                              "                             0,1,0-0,0,1"
                              "\n"
                              "                          or\n"
                              "                             -90-1,0,0\n"
                              "\n" << std::endl;
    osg::notify(osg::NOTICE)<<"    -t translation     - Convert spatial position of output files.  Format of\n"
                              "                         translation argument must be the following :\n"
                              "\n"
                              "                             X,Y,Z\n"
                              "\n"
                              "                         where X, Y, and Z represent the coordinates of the\n"
                              "                         absolute position in world space\n"
                              << std::endl;
    osg::notify(osg::NOTICE)<<"    --use-world-frame  - Perform transformations in the world frame, rather\n"
                              "                         than relative to the center of the bounding sphere.\n"
                              << std::endl;
    osg::notify(osg::NOTICE)<<"    --simplify n       - Run simplifier prior to output. Argument must be a" << std::endl
                            <<"                         normalized value for the resultant percentage" << std::endl
                            <<"                         reduction." << std::endl
                            <<"                         Example: --simplify .5" << std::endl
                            <<"                                 will produce a 50% reduced model." << std::endl
                            << std::endl;
    osg::notify(osg::NOTICE)<<"    -s scale           - Scale size of model.  Scale argument must be the \n"
                              "                         following :\n"
                              "\n"
                              "                             SX,SY,SZ\n"
                              "\n"
                              "                         where SX, SY, and SZ represent the scale factors\n"
                              "                         Caution: Scaling is done in destination orientation\n"
                              << std::endl;
    osg::notify(osg::NOTICE)<<"    --smooth           - Smooth the surface by regenerating surface normals on\n"
                              "                         all geometry nodes"<< std::endl;
    osg::notify(osg::NOTICE)<<"    --addMissingColors - Add a white color value to all geometry nodes\n"
                              "                         that don't have their own color values\n"
                              "                         (--addMissingColours also accepted)."<< std::endl;
    osg::notify(osg::NOTICE)<<"    --overallNormal    - Replace normals with a single overall normal."<< std::endl;
    osg::notify(osg::NOTICE)<<"    --enable-object-cache - Enable caching of objects, images, etc."<< std::endl;

    osg::notify( osg::NOTICE ) << std::endl;
    osg::notify( osg::NOTICE ) <<
        "    --formats          - List all supported formats and their supported options." << std::endl;
    osg::notify( osg::NOTICE ) <<
        "    --format <format>  - Display information about the specified <format>,\n"
        "                         where <format> is the file extension, such as \"flt\"." << std::endl;
    osg::notify( osg::NOTICE ) <<
        "    --plugins          - List all supported plugin files." << std::endl;
    osg::notify( osg::NOTICE ) <<
        "    --plugin <plugin>  - Display information about the specified <plugin>,\n"
        "                         where <plugin> is the plugin's full path and file name." << std::endl;
	//Ex version:
	osg::notify(osg::NOTICE) << "    --writeTextures <ext> - Write out the textures found in the scenegraph." << std::endl;
	osg::notify(osg::NOTICE) << "    --doNotRenameTextures - try not to change texture filenames." << std::endl;
	osg::notify(osg::NOTICE) << "    --writeTextureNodes   - Write out geometry per texture." << std::endl;
	osg::notify(osg::NOTICE) << "    --doNotLoadTextures   - Don't read textures." << std::endl;
	osg::notify(osg::NOTICE) << "    --cleanNodeMask       - set nodemask 0xffffffff and remove names& userdata. set datavariance static, remove LIGHTMODEL and indentity TexMat" << std::endl;
	osg::notify(osg::NOTICE) << "    --cleanNodeMask1      - like cleanNodeMask, but don't set Datavariace on nodes." << std::endl;
	osg::notify(osg::NOTICE) << "    --cleanNodeMask2      - add removal of GL_CULL_FACE ON,GL_LIGHTING ON,GL_NORMALIZE ON,GL_DEPTH_TEST ON and GL_NORMALIZE ANY" << std::endl;
	osg::notify(osg::NOTICE) << "    --prune-StateSet      - remove all statesets." << std::endl;
	osg::notify(osg::NOTICE) << "    --convertPagedLODS_to_LODS - convert all pagedLODS to LODS." << std::endl;
	osg::notify(osg::NOTICE) << "    --bsphere             - load file and print bounding sphere" << std::endl;
	osg::notify(osg::NOTICE) << "    --ibound              - store initial bound for geometry" << std::endl;
	osg::notify(osg::NOTICE) << "    --mipMapGenOff        - set useHardwareMipMapGeneration FALSE for all textures" << std::endl;
	osg::notify(osg::NOTICE) << "    --login <url> <username> <password> - Provide authentication information for http file access." << std::endl;
	osg::notify(osg::NOTICE) << "    --readImage - call readRefImageFile - not readRefNodeFiles" << std::endl;
	osg::notify(osg::NOTICE) << "    --lodRange %f         - select lod distance to export" << std::endl;
	osg::notify(osg::NOTICE) << "    --removeColours       - remove all colour arrays from geometry" << std::endl;


}


int main( int argc, char **argv )
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" is a utility for converting between various input and output databases formats.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] filename ...");
    arguments.getApplicationUsage()->addCommandLineOption("-h or --help","Display command line parameters");
    arguments.getApplicationUsage()->addCommandLineOption("--help-env","Display environmental variables available");


    // if user request help write it out to cout.
    if (arguments.read("-h") || arguments.read("--help"))
    {
        osg::setNotifyLevel(osg::NOTICE);
        usage( arguments.getApplicationName().c_str(), 0 );
        //arguments.getApplicationUsage()->write(std::cout);
        return 1;
    }

    if (arguments.read("--help-env"))
    {
        arguments.getApplicationUsage()->write(std::cout, osg::ApplicationUsage::ENVIRONMENTAL_VARIABLE);
        return 1;
    }
	{
		std::string url, username, password;
		while (arguments.read("--login", url, username, password))
		{
			osgDB::Registry::instance()->getOrCreateAuthenticationMap()->addAuthenticationDetails(
				url,
				new osgDB::AuthenticationDetails(username, password)
			);
		}
	}
	bool readImage = false;
	if (arguments.read("--readImage")) readImage = true;
    if (arguments.read("--plugins"))
    {
        osgDB::FileNameList plugins = osgDB::listAllAvailablePlugins();
        for(osgDB::FileNameList::iterator itr = plugins.begin();
            itr != plugins.end();
            ++itr)
        {
            std::cout<<"Plugin "<<*itr<<std::endl;
        }
        return 0;
    }

    std::string plugin;
    if (arguments.read("--plugin", plugin))
    {
        osgDB::outputPluginDetails(std::cout, plugin);
        return 0;
    }

    std::string ext;
    if (arguments.read("--format", ext))
    {
        plugin = osgDB::Registry::instance()->createLibraryNameForExtension(ext);
        osgDB::outputPluginDetails(std::cout, plugin);
        return 0;
    }

    if (arguments.read("--formats"))
    {
        osgDB::FileNameList plugins = osgDB::listAllAvailablePlugins();
        for(osgDB::FileNameList::iterator itr = plugins.begin();
            itr != plugins.end();
            ++itr)
        {
            osgDB::outputPluginDetails(std::cout,*itr);
        }
        return 0;
    }

    if (arguments.argc()<=1)
    {
        arguments.getApplicationUsage()->write(std::cout,osg::ApplicationUsage::COMMAND_LINE_OPTION);
        return 1;
    }

    FileNameList fileNames;
    OrientationConverter oc;
    bool do_convert = false;

    if (arguments.read("--use-world-frame"))
    {
        oc.useWorldFrame(true);
    }

    std::string str;
    while (arguments.read("-O",str))
    {
        osgDB::ReaderWriter::Options* options = new osgDB::ReaderWriter::Options;
        options->setOptionString(str);
        osgDB::Registry::instance()->setOptions(options);
    }

    while (arguments.read("-e",ext))
    {
        std::string libName = osgDB::Registry::instance()->createLibraryNameForExtension(ext);
        osgDB::Registry::instance()->loadLibrary(libName);
    }

    std::string libName;
    while (arguments.read("-l",libName))
    {
        osgDB::Registry::instance()->loadLibrary(libName);
    }

    while (arguments.read("-o",str))
    {
        osg::Vec3 from, to;
        if( sscanf( str.c_str(), "%f,%f,%f-%f,%f,%f",
                &from[0], &from[1], &from[2],
                &to[0], &to[1], &to[2]  )
            != 6 )
        {
            float degrees;
            osg::Vec3 axis;
            // Try deg-axis format
            if( sscanf( str.c_str(), "%f-%f,%f,%f",
                    &degrees, &axis[0], &axis[1], &axis[2]  ) != 4 )
            {
                usage( argv[0], "Orientation argument format incorrect." );
                return 1;
            }
            else
            {
                oc.setRotation( degrees, axis );
                do_convert = true;
            }
        }
        else
        {
            oc.setRotation( from, to );
            do_convert = true;
        }
    }

    while (arguments.read("-s",str))
    {
        osg::Vec3 scale(0,0,0);
        if( sscanf( str.c_str(), "%f,%f,%f",
                &scale[0], &scale[1], &scale[2] ) != 3 )
        {
            usage( argv[0], "Scale argument format incorrect." );
            return 1;
        }
        oc.setScale( scale );
        do_convert = true;
    }

    float simplifyPercent = 1.0;
    bool do_simplify = false;
    while ( arguments.read( "--simplify",str ) )
    {
        float nsimp = 1.0;
        if( sscanf( str.c_str(), "%f",
                &nsimp ) != 1 )
        {
            usage( argv[0], "Scale argument format incorrect." );
            return 1;
        }
        std::cout << str << " " << nsimp << std::endl;
        simplifyPercent = nsimp;
        osg::notify( osg::INFO ) << "Simplifying with percentage: " << simplifyPercent << std::endl;
        do_simplify = true;
    }

    while (arguments.read("-t",str))
    {
        osg::Vec3 trans(0,0,0);
        if( sscanf( str.c_str(), "%f,%f,%f",
                &trans[0], &trans[1], &trans[2] ) != 3 )
        {
            usage( argv[0], "Translation argument format incorrect." );
            return 1;
        }
        oc.setTranslation( trans );
        do_convert = true;
    }
	bool do_update = false; //do an update traversal to get transformed vertices in rigGeometry array's
	while (arguments.read("-u")) do_update = true;
    
    FixTransparencyVisitor::FixTransparencyMode fixTransparencyMode = FixTransparencyVisitor::NO_TRANSPARANCY_FIXING;
    std::string fixString;
    while(arguments.read("--fix-transparency")) fixTransparencyMode = FixTransparencyVisitor::MAKE_OPAQUE_TEXTURE_STATESET_OPAQUE;
    while(arguments.read("--fix-transparency-mode",fixString))
    {
         if (fixString=="MAKE_OPAQUE_TEXTURE_STATESET_OPAQUE") fixTransparencyMode = FixTransparencyVisitor::MAKE_OPAQUE_TEXTURE_STATESET_OPAQUE;
		 if (fixString == "MAKE_STATESET_OPAQUE_OR_ALPHA_TEST") fixTransparencyMode = FixTransparencyVisitor::MAKE_STATESET_OPAQUE_OR_ALPHA_TEST;
		 if (fixString == "MAKE_STATESET_OPAQUE_OR_ALPHA_TEST_LIGHTING_OFF") fixTransparencyMode = FixTransparencyVisitor::MAKE_STATESET_OPAQUE_OR_ALPHA_TEST_LIGHTING_OFF;

         if (fixString=="MAKE_ALL_STATESET_OPAQUE") fixTransparencyMode = FixTransparencyVisitor::MAKE_ALL_STATESET_OPAQUE;
    };

	bool cleanNodeMask = false;
    bool cleanNodeMask1 = false;
	bool cleanNodeMask2 = false;
	

	while (arguments.read("--cleanNodeMask")) cleanNodeMask = true;
    while (arguments.read("--cleanNodeMask1")) cleanNodeMask1 = true;
	while (arguments.read("--cleanNodeMask2")) cleanNodeMask2 = true;

    bool pruneStateSet = false;
    while(arguments.read("--prune-StateSet")) pruneStateSet = true;

	bool convertPagedLODS_to_LODS = false;
	while(arguments.read("--convertPagedLODS_to_LODS")) convertPagedLODS_to_LODS = true;
    float lodRange = 0.0f;
    while (arguments.read("--lodRange", str))
    {
        if (sscanf(str.c_str(), "%f", &lodRange) != 1)
        {
            usage(argv[0], "lodRange argument format incorrect.");
            return 1;
        }
        std::cout << str << " " << lodRange << std::endl;
        osg::notify(osg::INFO) << "selecting LOD distance: " << lodRange << std::endl;
    }
	
    osg::Texture::InternalFormatMode internalFormatMode = osg::Texture::USE_IMAGE_DATA_FORMAT;
    while(arguments.read("--compressed") || arguments.read("--compressed-arb")) { internalFormatMode = osg::Texture::USE_ARB_COMPRESSION; }

    while(arguments.read("--compressed-dxt1")) { internalFormatMode = osg::Texture::USE_S3TC_DXT1_COMPRESSION; }
    while(arguments.read("--compressed-dxt3")) { internalFormatMode = osg::Texture::USE_S3TC_DXT3_COMPRESSION; }
    while(arguments.read("--compressed-dxt5")) { internalFormatMode = osg::Texture::USE_S3TC_DXT5_COMPRESSION; }

    bool smooth = false;
    while(arguments.read("--smooth")) { smooth = true; }

	bool removeColours = false;
	while (arguments.read("--removeColours") || arguments.read("--removeColors")) { removeColours = true; }

    bool addMissingColours = false;
    while(arguments.read("--addMissingColours") || arguments.read("--addMissingColors")) { addMissingColours = true; }

    bool do_overallNormal = false;
    while(arguments.read("--overallNormal") || arguments.read("--overallNormal")) { do_overallNormal = true; }

    bool enableObjectCache = false;
    while(arguments.read("--enable-object-cache")) { enableObjectCache = true; }

	bool do_write_textures = false;
	std::string write_textures_ext;
    while(arguments.read("--writeTextures",write_textures_ext)) { do_write_textures = true; }
	bool rename_textures = true;
	while (arguments.read("--doNotRenameTextures")) { rename_textures = false; }

	bool do_not_load_textures = false;
    while(arguments.read("--doNotLoadTextures") ) { do_not_load_textures = true; }
	if (do_not_load_textures) {
		static osgDB::RegisterReaderWriterProxy<DummyImgReader> g_proxy_DummyImgReader;
		//REGISTER_OSGPLUGIN(dummy, DummyImgReader)
	}

	bool do_write_texture_nodes = false;
    while(arguments.read("--writeTextureNodes") ) { do_write_texture_nodes = true; }

	bool bsphere = false;
	while (arguments.read("--bsphere")) {
		bsphere = true;
		do_not_load_textures = true;
	}
	bool ibound = false;
	while (arguments.read("--ibound")) {
		ibound = true;
	}
	bool mipMapGenOff = false;
	while (arguments.read("--mipMapGenOff")) {
		mipMapGenOff = true;
	}
	
    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }

    for(int pos=1;pos<arguments.argc();++pos)
    {
        if (!arguments.isOption(pos))
        {
            fileNames.push_back(arguments[pos]);
        }
    }

    if (enableObjectCache)
    {
        if (osgDB::Registry::instance()->getOptions()==0) osgDB::Registry::instance()->setOptions(new osgDB::Options());
        osgDB::Registry::instance()->getOptions()->setObjectCacheHint(osgDB::Options::CACHE_ALL);
//        osgDB::Registry::instance()->getOptions()->setObjectCacheHint((osgDB::Options::CacheHintOptions)(osgDB::Options::CACHE_ALL & ~osgDB::Options::CACHE_IMAGES & ~osgDB::Options::CACHE_OBJECTS));
    }

    std::string fileNameOut("converted.osg");
    if (fileNames.size()>1)
    {
        fileNameOut = fileNames.back();
        fileNames.pop_back();
    }

    osg::Timer_t startTick = osg::Timer::instance()->tick();
	osg::ref_ptr<osg::Node> root;
	if (!readImage) {
		root = osgDB::readRefNodeFiles(fileNames);

		if (root.valid())
		{
			osg::Timer_t endTick = osg::Timer::instance()->tick();
			osg::notify(osg::INFO) << "Time to load files " << osg::Timer::instance()->delta_m(startTick, endTick) << " ms" << std::endl;
		}
		else
		{
			osg::notify(osg::NOTICE) << "Error no data loaded." << std::endl;
			return 1;
		}
	} else {
		if (fileNames.size() == 1) {
			std::string fileNameIn = fileNames.back();
			startTick = osg::Timer::instance()->tick();
			osg::notify(osg::NOTICE) << "reading osg::Image from '" << fileNameIn << "'." << std::endl;
			osg::ref_ptr<osg::Image> volumeImage = osgDB::readRefImageFile(fileNameIn);
			if (volumeImage.valid()) {
				osg::Timer_t endTick = osg::Timer::instance()->tick();
				osg::notify(osg::INFO) << "Time to load files " << osg::Timer::instance()->delta_m(startTick, endTick) << " ms" << std::endl;
#if 0
				//disable RobustBinaryFormat to be readable from a stream
				osgDB::ReaderWriter::Options* local_opt = osgDB::Registry::instance()->getOptions();
				if (!local_opt) local_opt = new osgDB::ReaderWriter::Options();
				else local_opt = new osgDB::ReaderWriter::Options(*local_opt);
//				local_opt->setPluginStringData("fileType", "Binary");
				local_opt->setPluginStringData("RobustBinaryFormat", "false");
				osg::notify(osg::NOTICE) << "writing osg::Image to '" << fileNameOut << "'." << std::endl;
				osgDB::ReaderWriter::WriteResult result = osgDB::Registry::instance()->writeImage(*volumeImage.get(), fileNameOut, local_opt);
#else
				osgDB::ReaderWriter::WriteResult result = osgDB::Registry::instance()->writeImage(*volumeImage.get(), fileNameOut, osgDB::Registry::instance()->getOptions());
#endif
				if (result.success())
				{
					osg::notify(osg::NOTICE) << "osg::Image written to '" << fileNameOut << "'." << std::endl;
					return 0;
				}
				else 
				{
					osg::notify(osg::NOTICE) << "Warning writeImage write to '" << fileNameOut;
					if (result.message().empty())
					{
						osg::notify(osg::NOTICE) << "' not supported." << std::endl;
					}
					else
					{
						osg::notify(osg::NOTICE) << "' error: " << std::endl << result.message() << std::endl;
					}
				}
			} else {
				osg::ref_ptr<osg::Object> rootObj = osgDB::readRefObjectFile(fileNameIn);
				if (rootObj.valid()) {
					osgDB::ReaderWriter::WriteResult result = osgDB::Registry::instance()->writeObject(*rootObj.get(), fileNameOut, osgDB::Registry::instance()->getOptions());
					if (result.success())
					{
						osg::notify(osg::NOTICE) << "osg::Object written to '" << fileNameOut << "'." << std::endl;
						return 0;
					}
					else 
					{
						osg::notify(osg::NOTICE) << "Warning writeObject write to '" << fileNameOut;
						if (result.message().empty())
						{
							osg::notify(osg::NOTICE) << "' not supported." << std::endl;
						}
						else
						{
							osg::notify(osg::NOTICE) << "' error: " << result.message() << std::endl;
						}
					}
				}
			}
			return 1;
		}
        osg::notify(osg::NOTICE)<<"Error cannot handle mutiple input images."<< std::endl;
        return 1;
    }
	/*
	dump out coords
	if (osg::Group *rootGroup = dynamic_cast<osg::Group *>(root.get())) {
		osg::notify(osg::NOTICE) << "root has " <<  rootGroup->getNumChildren() << " chidren" << std::endl;
		for (unsigned int child = 0; child < rootGroup->getNumChildren();++child) {
			osg::Geode *childGeode = dynamic_cast<osg::Geode *>(rootGroup->getChild(child));
			if (childGeode) {
				for (unsigned int drwble = 0; drwble < childGeode->getNumDrawables(); ++drwble) {
					osg::Array *va = childGeode->getDrawable(drwble)->asGeometry()->getVertexArray();
					osg::Vec3Array *v3a = dynamic_cast<osg::Vec3Array *>(va);
	#if 0
					osg::Vec3 center = (*v3a)[0] + (*v3a)[1] + (*v3a)[2] + (*v3a)[3];
					center *= 0.25;
					if ((child & 0x1) == 0 ) {//even children only
						osg::notify(osg::NOTICE) << "<activeObject prototype=\"eik_zomerB\" > <setting x=\""<< center.x() <<"\" y=\""<< center.y() <<"\" z=\""<< center.z() <<"\" /></activeObject>"<< std::endl;
					}
	#else
					if (childGeode->getDrawable(drwble)->getName() == "Metaal-IJzer") {
						osg::Vec3 center = (*v3a)[0];// + osg::Vec3(45.7111,-18.4075,-3.995);
						osg::Vec3 forward = (*v3a)[0] - (*v3a)[1];
						double heading = osg::RadiansToDegrees(atan2(forward.x(), -forward.y()));
						osg::notify(osg::NOTICE) << "<activeObject prototype=\"blue_chair\" > <setting x=\""<< center.x() <<"\" y=\""<< center.y() <<"\" z=\""<< center.z() <<"\" h=\""<<heading<<"\" /></activeObject>"<< std::endl;
					}
	#endif
				}
			}
		}
	}
	*/	
#if 0
	std::map<int, std::map<int, osg::ref_ptr<osg::Geode> > > ;
	if (osg::Geode *childGeode = dynamic_cast<osg::Geode *>(root.get())) {
		for (unsigned int drwble = 0; drwble < childGeode->getNumDrawables(); ++drwble) {
			osg::Array *va = childGeode->getDrawable(drwble)->asGeometry()->getVertexArray();
			osg::Vec3Array *v3a = dynamic_cast<osg::Vec3Array *>(va);
			osg::Array *ca = childGeode->getDrawable(drwble)->asGeometry()->getColorArray();
			osg::Vec4Array *c4a = dynamic_cast<osg::Vec4Array *>(ca);
			int min_ix = (int) (v3a[0][0] /100.0f);
			int min_iy = (int) (v3a[0][1] /100.0f);
			int max_ix = min_iy;
			int max_iy = min_iy;
			for (unsigned int index = 0; index < v3a->size();++index) {
				int ix = (int) (v3a[index][0] /100.0f);
				if (ix < min_ix) min_ix = ix;
				if (ix > max_ix) max_ix = ix;
				int iy = (int) (v3a[index][1] /100.0f);
				if (iy < min_iy) min_ix = iy;
				if (iy > max_iy) max_ix = iy;
			}
			int size_ix = max_ix - min_ix +1;
			int size_iy = max_iy - min_iy +1;
			std::vector<osg::Geometry*> geoms;
			for (unsigned int count = 0; count <size_ix * size_iy; ++count) {
				osg::Geometry* geom( new osg::Geometry );
				geoms.push_back(geom);
				osg::Vec3Array* vAry = new osg::Vec3Array;
				vAry->reserve( 40000000 );
				geom->setVertexArray( vAry );

				osg::Vec4Array* cAry = new osg::Vec4Array;
				geom->setColorArray( cAry );
				geom->setColorBinding( osg::Geometry::BIND_PER_VERTEX );
			}
			for (unsigned int index = 0; index < v3a->size();++index) {
				int ix = (int) (v3a[index][0] /100.0f) - min_ix;
				int iy = (int) (v3a[index][1] /100.0f) - min_iy;
			}
		}
	}
#endif
	if (bsphere) {
		osg::setNotifyLevel(osg::NOTICE);
		const osg::BoundingSphere &bs = root->getBound();
		osg::notify(osg::NOTICE) << "  Center " << bs.center().x() << " " << bs.center().y() << " " << bs.center().z() << " " << std::endl;
		osg::notify(osg::NOTICE) << "  Radius " << bs.radius() << std::endl;
		return 0;
	}
	if (ibound) {
		setInitialBoundGeometryVisitor sibgv;
		root->accept(sibgv);
	}
    if (cleanNodeMask || cleanNodeMask1 || cleanNodeMask2)
    {
		setNodeMaskVisitor snmv;
		if (cleanNodeMask1) snmv.setDataVarianceStatic(false);
		if (cleanNodeMask2) snmv.setRemodeDefaultModes(1);
		root->accept(snmv);
	}
    if (pruneStateSet)
    {
        PruneStateSetVisitor pssv;
        root->accept(pssv);
    }

	// to optimize a matixtransform at the top of the graph, create a temp group:
	osg::ref_ptr<osg::Group> container = new osg::Group;
	container->addChild(root.get());
	root = container;

	if (convertPagedLODS_to_LODS) {
		PagedLODToLODConverter plodConvertor = PagedLODToLODConverter();
		root->accept(plodConvertor);
	}
    if (lodRange != 0.0f) {
        LODselector lodSelector = LODselector(lodRange);
        root->accept(lodSelector);
    }
//////////////////////////// prepare RigGeometry's for optimize
	{
		RigGeometryPrepareVisitor rgprep;
		root->accept(rgprep);
	}
//////////////////////////// prepare RigGeometry's for optimize done
    if (fixTransparencyMode != FixTransparencyVisitor::NO_TRANSPARANCY_FIXING)
    {
        FixTransparencyVisitor atv(fixTransparencyMode);
        root->accept(atv);
    }

    if ( root.valid() )
    {

        if (smooth)
        {
            osgUtil::SmoothingVisitor sv;
            root->accept(sv);
        }

		if (removeColours)
		{
			RemoveColoursFromGeometryVisitor av;
			root->accept(av);
		}

        if (addMissingColours)
        {
            AddMissingColoursToGeometryVisitor av;
            root->accept(av);
        }



        // optimize the scene graph, remove redundant nodes and state etc.
        osgUtil::Optimizer optimizer;
        optimizer.optimize(root.get());

		//now remove the container:
		if (container->getNumChildren() == 1 &&
			container->getEventCallback() == NULL &&
			container->getUpdateCallback() == NULL &&
			container->getCullCallback() == NULL  &&
			container->getComputeBoundingSphereCallback() == NULL &&
			container->getStateSet() == NULL ) {
			osg::ref_ptr<osg::Node> temp = container->getChild(0);
			root = temp;
			container->removeChild(temp.get());
			container = NULL;
		}

        if( do_convert )
            root = oc.convert( root.get() );
		if (mipMapGenOff) {
			CompressTexturesVisitor ctv(osg::Texture::USE_IMAGE_DATA_FORMAT);
			root->accept(ctv);//find all textures
			ctv.setUseHardwareMipMapGeneration(false);
		} 
        if (internalFormatMode != osg::Texture::USE_IMAGE_DATA_FORMAT)
        {
            std::string ext = osgDB::getFileExtension(fileNameOut);
            if (ext=="ive")
            {
                CompressTexturesVisitor ctv(internalFormatMode);
                root->accept(ctv);
                ctv.compress();
            }
            else
            {
				if( !do_write_textures )
					std::cout<<"Warning: compressing texture only supported when outputing to .ive or osg whith  --writeTextures "<<std::endl;
            }
        }
        if( do_write_textures )
		{
             CompressTexturesVisitor ctv(internalFormatMode);
             root->accept(ctv);
			 if (internalFormatMode != osg::Texture::USE_IMAGE_DATA_FORMAT)
				 ctv.compress();
             std::string texBaseName = osgDB::getNameLessExtension(fileNameOut);
			 ctv.write(write_textures_ext, texBaseName, rename_textures);//rename all textures
		}
        if( do_write_texture_nodes )
		{
             CompressTexturesVisitor ctv(internalFormatMode);
             root->accept(ctv);
			 root = ctv.writeNodes();
		}

        // scrub normals
        if ( do_overallNormal )
        {
            DefaultNormalsGeometryVisitor dngv;
            root->accept( dngv );
        }

        // apply any user-specified simplification
        if ( do_simplify )
        {
            osgUtil::Simplifier simple;
            simple.setSmoothing( smooth );
            osg::notify( osg::ALWAYS ) << " smoothing: " << smooth << std::endl;
            simple.setSampleRatio( simplifyPercent );
            root->accept( simple );
        }
//////////////////////////// restore RigGeometry's after optimize
	{
		RigGeometryApplyVisitor rgapply;
		root->accept(rgapply);
	}
	if (do_update) {
		osg::ref_ptr<osgUtil::UpdateVisitor> updateVisitor = new osgUtil::UpdateVisitor();
		updateVisitor->setFrameStamp(new osg::FrameStamp());
		root->accept(*(updateVisitor.get()));
	}
//////////////////////////// restore RigGeometry's after optimize done

        osgDB::ReaderWriter::WriteResult result = osgDB::Registry::instance()->writeNode(*root,fileNameOut,osgDB::Registry::instance()->getOptions());
        if (result.success())
        {
            osg::notify(osg::NOTICE)<<"Data written to '"<<fileNameOut<<"'."<< std::endl;
        }
        else if  (result.message().empty())
        {
            osg::notify(osg::NOTICE)<<"Warning: file write to '"<<fileNameOut<<"' not supported."<< std::endl;
        }
        else
        {
            osg::notify(osg::NOTICE)<<result.message()<< std::endl;
        }
    }
    else
    {
        osg::notify(osg::NOTICE)<<"Error no data loaded."<< std::endl;
        return 1;
    }

    return 0;
}
