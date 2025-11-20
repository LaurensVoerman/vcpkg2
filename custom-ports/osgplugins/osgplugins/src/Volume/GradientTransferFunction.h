#ifndef OSGVOLUME_GRADIENTTRANSFERFUNTION
#define OSGVOLUME_GRADIENTTRANSFERFUNTION

#include <osg/TransferFunction>


namespace osg_ibr {
	/** A Transferfunction based on gradient
	 * Instead of having a map of 1D points with colors like normal transfer functions do 
	 * this transferfunction has a map of 2D quads, each quad has a color and a mode.
	 * The mode specifies what color the pixels in the transferfunction that are covered by the quad qill get:
	 * - standard mode: all pixels will have the same color and alpha
	 * - circle mode: same as standard but alpha will be less if the pixels is further from the quads center
	 * - gradient mode: same as circle but this tile alpha is dependend only at the horizontal distance from center
	 * The quad does not need to be rectangular, is that case color modes will be warped
	 */
	class GradientTransferFunction : public osg::TransferFunction
	{
		public :

			class Quad{
			public:
				enum Mode
				{
					Standard = 0,
					Circle = 1,
					Gradient = 2,
					Rainbow = 3
				};
				bool getColor(float x, float y, osg::Vec4 &outColor);
				osg::Vec2 _points[4]; // clockwise
				osg::Vec4 _color;
				Mode	  _gradientMode;
				bool operator==(const Quad &other) const {
					return	_points[0] == other._points[0] && _points[1] == other._points[1] &&
							_points[2] == other._points[2] && _points[3] == other._points[3] &&
							_color == other._color && _gradientMode == other._gradientMode;
				}
			};

			GradientTransferFunction();

			META_Object(osg_ibr, GradientTransferFunction)

		   /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
			GradientTransferFunction(const GradientTransferFunction& tf, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

			void allocate(const osg::Vec2ui &cells);

			osg::Vec2ui getNumberImageCells() const { return _image.valid() ? osg::Vec2ui(_image->s(),_image->t()) : osg::Vec2ui(0,0); }

			void clear(const osg::Vec4& color = osg::Vec4(0.0f,0.0f,0.0f,0.0f));

			void addQuad(const Quad &quad, bool updateImg = true);

			/** Manually update the associate osg::Image to represent the colors assigned in the color map.*/
			void updateImage();

			typedef std::vector<Quad> QuadMap;

			/** Get the color map that stores the mapping between the the transfer function value and the colour it maps to.*/
			QuadMap& getColorMap() { return _quadMap; }

			/** Get the const color map that stores the mapping between the the transfer function value and the colour it maps to.*/
			const QuadMap& getColorMap() const { return _quadMap; }

			/** Assign a color map and automatically update the image to make sure they are in sync.*/
			void assign(const QuadMap& quadMap, bool updateImage = true ,const osg::Vec4& color = osg::Vec4(0.0f,0.0f,0.0f,0.0f) );
	private:
		QuadMap _quadMap;
		osg::Vec4 _baseColor;
	};
};

#endif