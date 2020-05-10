
#include "stdafx.h"
#include "Point.h"

inline double inner(const Point2D &a, const Point2D &b) {
	return a.x*b.x+a.y*b.y;
}
Point2D operator + (const Point2D &a, const Point2D &b) {
	return Point2D(a.x+b.x, a.y+b.y);
}
Point2D operator - (const Point2D &a, const Point2D &b) {
	return Point2D(a.x-b.x, a.y-b.y);
}
Point2D operator * (const Point2D &a, double t) {
	return Point2D(a.x*t, a.y*t);
}
Point2D operator * (double t, const Point2D &a) {
	return Point2D(a.x*t, a.y*t);
}
Point2D operator / (const Point2D &a, double t) {
	return Point2D(a.x/t, a.y/t);
}
Point2D operator * (const Point2D &a, const Point2D &b) {
	return Point2D(a.x*b.x-a.y*b.y, a.x*b.y+a.y*b.x);
}
Point2D operator / (const Point2D &a, const Point2D &b) {
	return Point2D(a.x*b.x+a.y*b.y, a.y*b.x-a.x*b.y)/inner(b, b);
}