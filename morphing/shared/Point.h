#ifndef POINT_H
#define POINT_H

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
Point2D operator * (const Point2D &a, double t);
Point2D operator * (double t, const Point2D &a);
Point2D operator / (const Point2D &a, double t);
Point2D operator * (const Point2D &a, const Point2D &b);
Point2D operator / (const Point2D &a, const Point2D &b);

#endif