#include <osg/Notify>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/LOD>

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/fstream>
#include <osgDB/Registry>

#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <string.h>

#include <liblas/liblas.hpp>
#include <liblas/reader.hpp>
#include <liblas/point.hpp>
#include <liblas/detail/timer.hpp>

class ReaderWriterLAS : public osgDB::ReaderWriter
{
    public:

        ReaderWriterLAS()
        {
            supportsExtension("las", "LAS point cloud format");
            supportsExtension("laz", "compressed LAS point cloud format");
            supportsOption("v", "Verbose output");
            supportsOption("noScale", "don't scale vertices according to las header - put scale in matrixTransform");
            supportsOption("noReCenter", "don't transform vertex coords to re-center the pointcloud");
        }

        virtual const char* className() const { return "LAS point cloud reader"; }

        virtual ReadResult readObject(const std::string& filename, const osgDB::ReaderWriter::Options* options) const
        {
            return readNode(filename, options);
        }

        virtual ReadResult readNode(const std::string& file, const osgDB::ReaderWriter::Options* options) const 
        {
            std::string ext = osgDB::getLowerCaseFileExtension(file);
            if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

            std::string fileName = osgDB::findDataFile(file, options);
            if (fileName.empty()) return ReadResult::FILE_NOT_FOUND;

            OSG_INFO << "Reading file " << fileName << std::endl;
            std::ifstream ifs;
            if (!liblas::Open(ifs, file))
            {
                return ReadResult::ERROR_IN_READING_FILE;
            }
            return readNode(ifs, options);
        }

        virtual ReadResult readObject(std::istream& fin, const osgDB::ReaderWriter::Options* options) const
        {
            return readNode(fin, options);
        }

       virtual ReadResult readNode(std::istream& ifs, const Options* options) const {
            // Reading options
            bool _verbose = false;
            bool _scale = true;
            bool _recenter = true;
            if (options)
            {
                std::istringstream iss(options->getOptionString());
                std::string opt;
                while (iss >> opt)
                {
                    if (opt == "v")
                    {
                        _verbose = true;
                    }
                    if (opt == "noScale")
                    {
                        _scale = false;
                    }
                    if (opt == "noReCenter")
                    {
                        _recenter = false;
                    }
                }
            }
            liblas::ReaderFactory f;
            liblas::Reader reader = f.CreateWithStream(ifs);
            liblas::Header const& h = reader.GetHeader();

            if (_verbose)
            {
                //std::cout << "File name: " << file << '\n';
                //std::cout << "Version  : " << reader.GetVersion() << '\n';
                std::cout << "Signature: " << h.GetFileSignature() << '\n';
                std::cout << "Format   : " << h.GetDataFormatId() << '\n';
                std::cout << "Project  : " << h.GetProjectId() << '\n';
                std::cout << "Points count: " << h.GetPointRecordsCount() << '\n';
                std::cout << "VLRecords count: " << h.GetRecordsCount() << '\n';
                std::cout << "Points by return: ";
                std::copy(h.GetPointRecordsByReturnCount().begin(),
                          h.GetPointRecordsByReturnCount().end(),
                          std::ostream_iterator<uint32_t>(std::cout, " "));
                std::cout << std::endl;
            }


            // POINTS ////
            if (h.GetPointRecordsCount() < 1) return NULL;//empty file
            unsigned int targetNumVertices = 10000;

            std::vector<osg::Geode*> lodGeode;
            std::vector<osg::Geometry*> lodGeometry;
            std::vector<osg::Vec3Array*> lodVertices;
            std::vector<osg::Vec4ubArray*> lodColours;

            unsigned int lodCount = 8;
            for (lodCount = 1; lodCount < 9; ++lodCount) {
                unsigned int bit = 1 << lodCount;
                if (h.GetPointRecordsCount() < 1024 * bit) break;
            }
            for (unsigned int i = 0; i < lodCount; ++i) {
                lodGeode.push_back(new osg::Geode);
                lodGeometry.push_back(new osg::Geometry);
                lodVertices.push_back(new osg::Vec3Array());
                lodColours.push_back(new osg::Vec4ubArray);
                lodVertices[i]->reserve(targetNumVertices);
                lodColours[i]->reserve(targetNumVertices);
            }

            liblas::detail::Timer t;
            t.start();
            typedef std::pair<double, double> minmax_t;
            minmax_t mx (DBL_MAX, -DBL_MAX);
            minmax_t my (DBL_MAX, -DBL_MAX);
            minmax_t mz (DBL_MAX, -DBL_MAX);

            uint32_t i = 0;//pointIndex
            bool singleColor = true;
            bool findCenter = _recenter || lodCount > 1;
            liblas::Color singleColorValue;
            while (reader.ReadNextPoint())
            {
                unsigned int lodLevel = 0;
                //if ((pointIndex & 1) == 1) lodLevel = 0;     //tail    1odd points (2n+1)
                //else if ((pointIndex & 2) == 2) lodLevel = 1;//tail   10(4n+2)
                //else if ((pointIndex & 4) == 4) lodLevel = 2;//tail  100(8n+4)
                for (lodLevel = 0; lodLevel < lodCount-1; ++lodLevel) {
                    unsigned int bit = 1 << lodLevel;
                    if ((i & bit) == bit) break;
                }
                osg::Geode* geode = lodGeode[lodLevel];
                osg::Geometry* geometry = lodGeometry[lodLevel];
                osg::Vec3Array* vertices = lodVertices[lodLevel];
                osg::Vec4ubArray* colours = lodColours[lodLevel];

                liblas::Point const& p = reader.GetPoint();

                // Extract color components from LAS point
                liblas::Color c = p.GetColor();
                uint32_t r = c.GetRed() >> 8;
                uint32_t g = c.GetGreen() >> 8;
                uint32_t b = c.GetBlue() >> 8;
                uint32_t a = 255;    // default value, since LAS point has no alpha information

                if (vertices->size() == 0)
                {
                    singleColorValue = c;
                    singleColor = true;
                }
                else
                {
                    if (singleColor)
                    {
                        singleColor = singleColorValue == c;//set false if different color found
                    }
                }
                if (vertices->size() >= targetNumVertices)
                {
                    // finishing setting up the current geometry and add it to the geode.
                    geometry->setUseDisplayList(true);
                    geometry->setUseVertexBufferObjects(true);
                    geometry->setVertexArray(vertices);
                    if (singleColor)
                    {
                        colours->resize(1);
                        geometry->setColorArray(colours, osg::Array::BIND_OVERALL);
                    }
                    else
                    {
                        geometry->setColorArray(colours, osg::Array::BIND_PER_VERTEX);

                    }
                    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));

                    geode->addDrawable(geometry);

                    // allocate a new geometry
                    lodGeometry[lodLevel] = new osg::Geometry;
                    lodVertices[lodLevel] = new osg::Vec3Array;
                    lodColours[lodLevel] = new osg::Vec4ubArray;

                    lodVertices[lodLevel]->reserve(targetNumVertices);
                    lodColours[lodLevel]->reserve(targetNumVertices);
                }
                double X = p.GetRawX();
                double Y = p.GetRawY();
                double Z = p.GetRawZ();
                if (_scale)
                {
                    X *= h.GetScaleX();
                    Y *= h.GetScaleY();
                    Z *= h.GetScaleZ();
                }
                if (findCenter)
                {
                    mx.first = std::min<double>(mx.first, X);
                    mx.second = std::max<double>(mx.second, X);
                    my.first = std::min<double>(my.first, Y);
                    my.second = std::max<double>(my.second, Y);
                    mz.first = std::min<double>(mz.first, Z);
                    mz.second = std::max<double>(mz.second, Z);
                }
                vertices->push_back(osg::Vec3(X, Y, Z));

                colours->push_back(osg::Vec4ub(r, g, b, a));

                // Warning: Printing zillion of points may take looong time
                //std::cout << i << ". " << p << '\n';
                i++;
            }
            // calculate the mid point of the point cloud
            double mid_x = 0.5*(mx.second + mx.first);
            double mid_y = 0.5*(my.second + my.first);
            double mid_z = 0.5*(mz.second + mz.first);
            osg::Vec3 midVec(mid_x, mid_y, mid_z);

            for (unsigned int lodLevel = 0; lodLevel < lodCount; ++lodLevel) {
                osg::Geode* geode = lodGeode[lodLevel];
                osg::Geometry* geometry = lodGeometry[lodLevel];
                osg::Vec3Array* vertices = lodVertices[lodLevel];
                osg::Vec4ubArray* colours = lodColours[lodLevel];
                geometry->setUseDisplayList(true);
                geometry->setUseVertexBufferObjects(true);
                geometry->setVertexArray(vertices);
                if (singleColor)
                {
                    colours->resize(1);
                    geometry->setColorArray(colours, osg::Array::BIND_OVERALL);
                }
                else
                {
                    geometry->setColorArray(colours, osg::Array::BIND_PER_VERTEX);

                }
                geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));

                geode->addDrawable(geometry);

                if (_recenter)
                {
                    //Transform vertices to midpoint
                    for (unsigned int geomIndex = 0; geomIndex < geode->getNumDrawables(); ++geomIndex)
                    {
                        osg::Geometry *geom = dynamic_cast<osg::Geometry*>(geode->getDrawable(geomIndex));
                        if (geom)
                        {
                            osg::Vec3Array* vertArray = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
                            size_t vertArraySize = vertArray->size();
                            for (size_t vertexIndex = 0; vertexIndex < vertArraySize; ++vertexIndex)
                            {
                                (*vertArray)[vertexIndex] -= midVec;
                            }
                        }
                    }
                }
            }
            double const d2 = t.stop();

            if (_verbose)
            {
                std::cout << "Read points: " << i << " Elapsed Time: " << d2
                    << std::endl << std::endl;
            }

            // MatrixTransform with the mid-point translation

            osg::MatrixTransform *mt = new osg::MatrixTransform;
            mt->setDataVariance(osg::Object::STATIC);//can be optimized away
            if (_scale)//vertex positions are scaled already
            {
                if (_recenter)
                {
                    mt->setMatrix(osg::Matrix::translate(osg::Vec3d(h.GetOffsetX() + mid_x, h.GetOffsetY() + mid_y, h.GetOffsetZ() + mid_z)));
                }
                else
                {
                    mt->setMatrix(osg::Matrix::translate(osg::Vec3d(h.GetOffsetX(), h.GetOffsetY(), h.GetOffsetZ())));
                }
            }
            else
            {
                if (_recenter)
                {
                    mid_x *= h.GetScaleX();
                    mid_y *= h.GetScaleY();
                    mid_z *= h.GetScaleZ();
                    mt->setMatrix(osg::Matrix::scale(osg::Vec3d(h.GetScaleX(), h.GetScaleY(), h.GetScaleZ())) * osg::Matrix::translate(osg::Vec3d(h.GetOffsetX() + mid_x, h.GetOffsetY() + mid_y, h.GetOffsetZ() + mid_z)));
                }
                else
                {
                    mt->setMatrix(osg::Matrix::scale(osg::Vec3d(h.GetScaleX(), h.GetScaleY(), h.GetScaleZ())) * osg::Matrix::translate(osg::Vec3d(h.GetOffsetX(), h.GetOffsetY(), h.GetOffsetZ())));
                }
            }

            if (lodCount == 1) {
                mt->addChild(lodGeode[0]);
            } else {
                //osg::Vec3d size(mx.second - mx.first, my.second - my.first, mz.second - mz.first);
                double length = osg::maximum(mx.second - mx.first, my.second - my.first);
                length = osg::maximum(length, mz.second - mz.first);
                osg::LOD *lod = new osg::LOD();
                if (_recenter) {
                    lod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
                } else {
                    lod->setCenter(osg::LOD::vec_type(mid_x, mid_y, mid_z));
                }
                float rmin = length;
                osg::Group *lodGroup = new osg::Group();
                for (unsigned int lodLevel = 0; lodLevel < lodCount; ++lodLevel) lodGroup->addChild(lodGeode[lodLevel]);
                lod->addChild(lodGroup, 0.0f, rmin);
                for (unsigned int lodLevel = 1; lodLevel < lodCount; ++lodLevel) {
                    float rmax = 2.0f * rmin;
                    if (lodLevel == lodCount - 1) {
                        lod->addChild(lodGeode[lodLevel], rmin, rmax);
                    } else {
                        lodGroup = new osg::Group();
                        for (unsigned int childLevel = lodLevel; childLevel < lodCount; ++childLevel) lodGroup->addChild(lodGeode[childLevel]);
                        lod->addChild(lodGroup, rmin, rmax);
                    }
                    rmin = rmax;
                }
                mt->addChild(lod);
            }
            

            return mt;
        }
};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(las, ReaderWriterLAS)
