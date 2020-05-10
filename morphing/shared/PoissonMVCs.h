/*** PoissonMVCs.h */

#pragma once
#ifndef _POISSON_MVCS_H
#define _POISSON_MVCS_H

#include <vector>
#include <string>
using std::vector;
using std::string;
/*
#ifndef M_PI
#define M_PI 3.141592653589793238
#endif
*/
struct BaseCircle {
	BaseCircle(double cx_ = 0.00, double cy_ = 0.00, double r_ = 0.00) {
		cx = cx_;
		cy = cy_;
		r = r_;
	}
	double cx;
	double cy;
	double r;
};
/*
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
*/
#include "Point.h"

/** Poisson-MVCs (in 2D) */
/* This function is for domains without holes
   Input:  poly, p, c
   Output: coords
   Using default base circle implies mean value coordinates */
void poissonMVCs(const vector<Point2D> &poly, const Point2D &p, vector<double> &coords, BaseCircle c = BaseCircle());
/* This function is for domains with one or more holes
   edge[2*i] and edge[2*i+1] indicate the endpoint indices (in array 'poly') for the i-th edge */
void poissonMVCs(const vector<Point2D> &poly, const vector<int> &edge, const Point2D &p, vector<double> &coords, BaseCircle c = BaseCircle());

/** Poisson-MVCs (in 2D) with the 'basic regular placement' */
/* Compute the minimal circle that covers 'poly' */
void minCircle(const vector<Point2D> &poly, BaseCircle &c);
/* For domains without holes */
void basicPoissonMVCs(const vector<Point2D> &poly, const Point2D &p, vector<double> &coords);
/* For domains with one or more holes */
void basicPoissonMVCs(const vector<Point2D> &poly, const vector<int> &edge, const Point2D &p, vector<double> &coords, BaseCircle c = BaseCircle());

#endif
