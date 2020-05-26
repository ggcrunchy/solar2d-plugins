/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#include "CoronaLua.h"
#include "polylidar/polylidar.hpp"
#include "delaunator.hpp"
#include <vector>

/*

namespace polylidar
{

std::tuple<delaunator::Delaunator, std::vector<std::vector<size_t>>, std::vector<Polygon>> _extractPlanesAndPolygons(py::array_t<double> nparray,
																													 double alpha = DEFAULT_ALPHA, double xyThresh = DEFAULT_XYTHRESH, double lmax = DEFAULT_LMAX, size_t minTriangles = DEFAULT_MINTRIANGLES,
																													 size_t minHoleVertices = DEFAULT_MINHOLEVERTICES, double minBboxArea = DEFAULT_MINBBOX, double zThresh = DEFAULT_ZTHRESH,
																													 double normThresh = DEFAULT_NORMTHRESH, double normThreshMin = DEFAULT_NORMTHRESH_MIN,
																													 double allowedClass = DEFAULT_ALLOWEDCLASS)
{
	// This function allows us to convert keyword arguments into a configuration struct
	auto info = nparray.request();
	std::vector<size_t> shape({(size_t)info.shape[0], (size_t)info.shape[1]});
	Config config{shape[1], alpha, xyThresh, lmax, minTriangles, minHoleVertices, minBboxArea, zThresh, normThresh, normThreshMin, allowedClass};
	Matrix<double> points((double *)info.ptr, shape[0], shape[1]);
	return ExtractPlanesAndPolygons(points, config);
}

std::tuple<std::vector<std::vector<size_t>>, std::vector<Polygon>> _extractPlanesAndPolygonsFromMesh(py::array_t<double> vertices, py::array_t<size_t> triangles, py::array_t<size_t> halfedges,
																									 double alpha = DEFAULT_ALPHA, double xyThresh = DEFAULT_XYTHRESH, double lmax = DEFAULT_LMAX, size_t minTriangles = DEFAULT_MINTRIANGLES,
																									 size_t minHoleVertices = DEFAULT_MINHOLEVERTICES, double minBboxArea = DEFAULT_MINBBOX, double zThresh = DEFAULT_ZTHRESH,
																									 double normThresh = DEFAULT_NORMTHRESH, double normThreshMin = DEFAULT_NORMTHRESH_MIN,
																									 double allowedClass = DEFAULT_ALLOWEDCLASS)
{
	// This function allows us to convert keyword arguments into a configuration struct
	auto info = vertices.request();
	std::vector<size_t> shape({(size_t)info.shape[0], (size_t)info.shape[1]});
	Config config{shape[1], alpha, xyThresh, lmax, minTriangles, minHoleVertices, minBboxArea, zThresh, normThresh, normThreshMin, allowedClass};
	Matrix<double> points((double *)info.ptr, shape[0], shape[1]);
	delaunator::HalfEdgeTriangulation triangulation(points, triangles, halfedges);
	return ExtractPlanesAndPolygonsFromMesh(triangulation, config);
}

std::vector<double> _extractPointCloudFromFloatDepth(py::array_t<float> image, py::array_t<double> intrinsics, size_t stride = DEFAULT_STRIDE)
{
	// Create Image Wrapper
	auto info_im = image.request();
	Matrix<float> im((float *)info_im.ptr, info_im.shape[0], info_im.shape[1]);
	// Create intrinsics wrapper
	auto info_int = intrinsics.request();
	Matrix<double> intrinsics_((double *)info_int.ptr, info_int.shape[0], info_int.shape[1]);
	// Extract point cloud
	std::vector<double> points = ExtractPointCloudFromFloatDepth(im, intrinsics_, stride);
	// std::cout << "extractPointCloudFromFloatDepth C++ : " << points[0] << " Address:" <<  &points[0] << std::endl;

	return points;
}

std::tuple<std::vector<double>, std::vector<size_t>, std::vector<size_t>> _extractUniformMeshFromFloatDepth(py::array_t<float> image, py::array_t<double> intrinsics, size_t stride = DEFAULT_STRIDE)
{
	// Will hold the point cloud
	std::vector<double> points;
	std::vector<size_t> triangles;
	std::vector<size_t> halfedges;
	// Create Image Wrapper
	auto info_im = image.request();
	Matrix<float> im((float *)info_im.ptr, info_im.shape[0], info_im.shape[1]);
	// Create intrinsics wrapper
	auto info_int = intrinsics.request();
	Matrix<double> intrinsics_((double *)info_int.ptr, info_int.shape[0], info_int.shape[1]);

	// Get Data
	std::tie(points, triangles, halfedges) = ExtractUniformMeshFromFloatDepth(im, intrinsics_, stride);
	// std::cout << "_extractUniformMeshFromFloatDepth C++ : " << points[0] << " Address:" <<  &points[0] << std::endl;

	return std::make_tuple(std::move(points), std::move(triangles), std::move(halfedges));
}

std::vector<Polygon> _extractPolygonsFromMesh(py::array_t<double> vertices, py::array_t<size_t> triangles, py::array_t<size_t> halfedges,
											  double alpha = DEFAULT_ALPHA, double xyThresh = DEFAULT_XYTHRESH, double lmax = DEFAULT_LMAX, size_t minTriangles = DEFAULT_MINTRIANGLES,
											  size_t minHoleVertices = DEFAULT_MINHOLEVERTICES, double minBboxArea = DEFAULT_MINBBOX, double zThresh = DEFAULT_ZTHRESH,
											  double normThresh = DEFAULT_NORMTHRESH, double normThreshMin = DEFAULT_NORMTHRESH_MIN,
											  double allowedClass = DEFAULT_ALLOWEDCLASS)
{
	// This function allows us to convert keyword arguments into a configuration struct
	auto info = vertices.request();
	std::vector<size_t> shape({(size_t)info.shape[0], (size_t)info.shape[1]});
	Config config{shape[1], alpha, xyThresh, lmax, minTriangles, minHoleVertices, minBboxArea, zThresh, normThresh, normThreshMin, allowedClass};
	Matrix<double> points((double *)info.ptr, shape[0], shape[1]);
	delaunator::HalfEdgeTriangulation triangulation(points, triangles, halfedges);
	return ExtractPolygonsFromMesh(triangulation, config);
}

std::vector<Polygon> _extractPolygons(py::array_t<double> nparray,
									  double alpha = DEFAULT_ALPHA, double xyThresh = DEFAULT_XYTHRESH, double lmax = DEFAULT_LMAX, size_t minTriangles = DEFAULT_MINTRIANGLES,
									  size_t minHoleVertices = DEFAULT_MINHOLEVERTICES, double minBboxArea = DEFAULT_MINBBOX, double zThresh = DEFAULT_ZTHRESH,
									  double normThresh = DEFAULT_NORMTHRESH, double normThreshMin = DEFAULT_NORMTHRESH_MIN,
									  double allowedClass = DEFAULT_ALLOWEDCLASS)
{
	// This function allows us to convert keyword arguments into a configuration struct
	auto info = nparray.request();
	std::vector<size_t> shape({(size_t)info.shape[0], (size_t)info.shape[1]});
	Config config{shape[1], alpha, xyThresh, lmax, minTriangles, minHoleVertices, minBboxArea, zThresh, normThresh, normThreshMin, allowedClass};
	Matrix<double> points((double *)info.ptr, shape[0], shape[1]);
	return ExtractPolygons(points, config);
}

std::tuple<std::vector<Polygon>, std::vector<float>> _extractPolygonsAndTimings(py::array_t<double> nparray,
																				double alpha = DEFAULT_ALPHA, double xyThresh = DEFAULT_XYTHRESH, double lmax = DEFAULT_LMAX, size_t minTriangles = DEFAULT_MINTRIANGLES,
																				size_t minHoleVertices = DEFAULT_MINHOLEVERTICES, double minBboxArea = DEFAULT_MINBBOX, double zThresh = DEFAULT_ZTHRESH,
																				double normThresh = DEFAULT_NORMTHRESH, double allowedClass = DEFAULT_ALLOWEDCLASS)
{
	auto info = nparray.request();
	std::vector<size_t> shape({(size_t)info.shape[0], (size_t)info.shape[1]});
	// This function allows us to convert keyword arguments into a configuration struct
	Config config{shape[1], alpha, xyThresh, lmax, minTriangles, minHoleVertices, minBboxArea, zThresh, normThresh, 0.5, allowedClass};
	Matrix<double> points((double *)info.ptr, shape[0], shape[1]);
	std::vector<float> timings;
	auto polygons = ExtractPolygonsAndTimings(points, config, timings);

	return std::make_tuple(polygons, timings);
}
*/

CORONA_EXPORT int luaopen_plugin_polylidar (lua_State* L)
{
/*
	lua_newtable(L);// t
	luaL_register(L, NULL, tfd_funcs);
*/
	return 1;
}
