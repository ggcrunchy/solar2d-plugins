/*** CubicMVCs.h */

#pragma once
#ifndef _CUBIC_MVCS_H_
#define _CUBIC_MVCS_H_

#include <vector>
#include <string>
using std::vector;
using std::string;
/*
#ifndef M_PI
#define M_PI 3.141592653589793238
#endif

struct Point2D {
	Point2D(double x_ = 0.00, double y_ = 0.00) {
		x = x_;
		y = y_;
	}
	double x;
	double y;
};
Point2D operator + (const Point2D &a, const Point2D &b);
Point2D operator - (const Point2D &a, const Point2D &b);
Point2D operator * (const Point2D &a, const Point2D &b);
Point2D operator * (const Point2D &a, double t);
Point2D operator / (const Point2D &a, double t);
Point2D operator / (const Point2D &a, const Point2D &b);
*/
#include "Point.h"
/** Cubic-MVCs (in 2D) */
/* This function is for domains without holes
   Input:  poly, p
   Output: vCoords, gnCoords, gtCoords
   The length of vCoords is poly.size()
   The length of either gnCoords or gtCoords is 2*poly.size()
   gnCoords[2*i]   is the coordinate for gradient at vi   in the left handside normal direction of [vi,vi+1)
   gnCoords[2*i+1] is the coordinate for gradient at vi+1 in the left handside normal direction of [vi,vi+1)
   gtCoords[2*i]   is the coordinate for gradient at vi   in the direction of [vi,vi+1)
   gtCoords[2*i+1] is the coordinate for gradient at vi+1 in the direction of [vi+1,vi) */
void cubicMVCs(const vector<Point2D> &poly, const Point2D &p, vector<double> &vCoords, vector<double> &gnCoords, vector<double> &gtCoords);
/* This function is for domains with one or more holes
   edge[2*i] and edge[2*i+1] indicate the endpoint indices (in array 'poly') for the i-th edge */
void cubicMVCs(const vector<Point2D> &poly, const vector<int> &edge, const Point2D &p, vector<double> &vCoords, vector<double> &gnCoords, vector<double> &gtCoords);

#endif
