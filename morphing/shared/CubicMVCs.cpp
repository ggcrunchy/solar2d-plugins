/*** CubicMVCs.cpp */

#include "stdafx.h"
#include "CubicMVCs.h"
#include <cmath>

static vector<Point2D> y;
static vector<Point2D> z;
static vector<double> vCoeff[3];
static vector<double> gnCoeff[3];
static vector<double> gtCoeff[3];

inline double inner(const Point2D &a, const Point2D &b) {
	return a.x*b.x+a.y*b.y;
}
inline double area(const Point2D &a, const Point2D &b) {
	return a.y*b.x-a.x*b.y;
}
inline double dist(const Point2D &a, const Point2D &b) {
	return sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));
}
inline double distSquare(const Point2D &a, const Point2D &b) {
	return (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y);
}
inline double modulus(const Point2D &a) {
	return sqrt(a.x*a.x+a.y*a.y);
}
inline Point2D conj(const Point2D &a) {
	return Point2D(a.x, -a.y);
}
inline Point2D rotateL(const Point2D &a) {
	return Point2D(-a.y, a.x);
}
inline Point2D rotateR(const Point2D &a) {
	return Point2D(a.y, -a.x);
}
/*
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
	return Point2D(a.x*b.x+a.y*b.y, -a.x*b.y+a.y*b.x)/inner(b, b);
}*/
namespace CMVC {
Point2D log(const Point2D &a) {
	double R = std::log(inner(a, a))/2;
	double I = 0.00;
	if(a.x == 0 && a.y > 0) {
		I = M_PI/2;
	}else if(a.x == 0 && a.y < 0) {
		I = 3*M_PI/2;
	}else if(a.x > 0.00 && a.y >= 0) {
		I = std::atan(a.y/a.x);
	}else if(a.x > 0.00 && a.y < 0) {
		I = std::atan(a.y/a.x)+2*M_PI;
	}else if(a.x < 0.00) {
		I = std::atan(a.y/a.x)+M_PI;
	}
	return Point2D(R, I);
}
Point2D atan(const Point2D &a) {
	return rotateR(log((1+rotateL(a))/(1-rotateL(a))))/2;
}

bool crossOrigin(const Point2D &a, const Point2D &b) {
	double areaAB = fabs(area(a, b));
	double distSquareAB = distSquare(a, b);
	double maxInner = (1+1E-10)*distSquareAB;
	return areaAB < 1E-10*distSquareAB && inner(a-b, a) < maxInner && inner(b-a, b) < maxInner;
}

bool checkPolygon(const vector<Point2D> &poly) {
	if(int(poly.size()) < 3) {
		return false;
	}
	for(int i = 0;  i < int(poly.size());  i++) {
		int j = (i+1)%int(poly.size());
		if(poly[i].x == poly[j].x && poly[i].y == poly[j].y) {
			return false;
		}
	}
	return true;
}
};
using namespace CMVC;

void boundaryCoords(const vector<Point2D> &poly, const Point2D &p, vector<double> &vCoords, vector<double> &gnCoords, vector<double> &gtCoords, int i, int j) {
	for(int k = 0;  k < int(poly.size());  k++) {
		vCoords[k] = 0.00;
	}
	for(int k = 0;  k < 2*int(poly.size());  k++) {
		gnCoords[k] = 0.00;
		gtCoords[k] = 0.00;
	}
	double distI = dist(p, poly[i]);
	double distJ = dist(p, poly[j]);
	double distIJ = dist(poly[i], poly[j]);
	double alphaI = distJ/(distI+distJ);
	double alphaJ = distI/(distI+distJ);
	double cubicI = alphaI*alphaI*alphaJ;
	double cubicJ = alphaJ*alphaJ*alphaI;
	vCoords[i] = alphaI+cubicI-cubicJ;
	vCoords[j] = alphaJ+cubicJ-cubicI;
	gtCoords[2*i+0] = distIJ*cubicI;
	gtCoords[2*i+1] = distIJ*cubicJ;
}

void cubicMVCs(const vector<Point2D> &poly, const Point2D &p, vector<double> &vCoords, vector<double> &gnCoords, vector<double> &gtCoords) {
/*	if(checkPolygon(poly) == false) {
		return;
	}
*/
	y.resize(int(poly.size()));
	z.resize(int(poly.size()));
	for(int i = 0;  i < int(poly.size());  i++) {
		y[i] = poly[i]-p;
		z[i] = y[i]/modulus(y[i]);
	}
	double A[3] = {0.00, 0.00, 0.00};
	double B[3] = {0.00, 0.00, 0.00};
	double C[3] = {0.00, 0.00, 0.00};
	for(int i = 0;  i < 3;  i++) {
		vCoeff[i].resize(int(poly.size()));
		gnCoeff[i].resize(2*int(poly.size()));
		gtCoeff[i].resize(2*int(poly.size()));
		for(int j = 0;  j < int(poly.size());  j++) {
			vCoeff[i][j] = 0.00;
		}
		for(int j = 0;  j < 2*int(poly.size());  j++) {
			gnCoeff[i][j] = 0.00;
			gtCoeff[i][j] = 0.00;
		}
	}
	for(int i = 0;  i < int(poly.size());  i++) {
		int j = (i+1)%int(poly.size());
		double areaIJ = area(y[j], y[i]);
		double distSquareIJ = distSquare(y[i], y[j]);
		if(fabs(areaIJ) < 1E-10*distSquareIJ) {
			if(crossOrigin(y[i], y[j]) == true) {
				boundaryCoords(poly, p, vCoords, gnCoords, gtCoords, i, j);
				return;
			}else {
				continue;
			}
		}
		double distIJ = sqrt(distSquareIJ);
		double invDistIJ = 1.00/distIJ;

		Point2D alphaI = y[j]/areaIJ;
		Point2D kappaI[2] = {Point2D(alphaI.y/2, alphaI.x/2), Point2D(alphaI.y/2, -alphaI.x/2)};
		Point2D alphaJ = y[i]/(-areaIJ);
		Point2D kappaJ[2] = {Point2D(alphaJ.y/2, alphaJ.x/2), Point2D(alphaJ.y/2, -alphaJ.x/2)};
		Point2D kappa[2] = {kappaI[0]+kappaJ[0], kappaI[1]+kappaJ[1]};

		Point2D intgIJ2 = (z[j]*z[j]*z[j]-z[i]*z[i]*z[i])/3;
		Point2D intgIJ1 = z[j]-z[i];
		Point2D intgIJ0 = conj(z[i])-conj(z[j]);

		Point2D vecIJ = (y[j]-y[i])*invDistIJ;

		// cache intermediate variables to accelerate the computation
		Point2D kappaSquare = kappa[0]*kappa[0];
		Point2D kappaIntgIJ1 = kappa[0]*intgIJ1;
		Point2D kappaConjIntgIJ0 = kappa[1]*intgIJ0;
		Point2D kappaIJ = kappaI[0]*kappaJ[0];
		Point2D kappaKappaI = kappa[0]*kappaI[0];
		Point2D kappaI2 = kappaI[0]*kappaI[0];
		Point2D kappaI3 = kappaI[0]*kappaI2;
		Point2D kappaIIntgIJ2 = kappaI[0]*intgIJ2;
		double kappaIAbsSquare = (kappaI[0]*kappaI[1]).x;
		Point2D kappaKappaJ = kappa[0]*kappaJ[0];
		Point2D kappaJ2 = kappaJ[0]*kappaJ[0];
		Point2D kappaJ3 = kappaJ[0]*kappaJ2;
		Point2D kappaJIntgIJ2 = kappaJ[0]*intgIJ2;
		double kappaJAbsSquare = (kappaJ[0]*kappaJ[1]).x;

		// compute A[0], vCoeff[0]
		Point2D tmpI = kappa[1]*kappaI[0];
		Point2D tmpJ = kappa[1]*kappaJ[0];
		double valueI = 2*(kappaSquare*kappaIIntgIJ2+(tmpI+2*tmpI.x)*kappaIntgIJ1).y;
		double valueJ = 2*(kappaSquare*kappaJIntgIJ2+(tmpJ+2*tmpJ.x)*kappaIntgIJ1).y;
		vCoeff[0][i] += 2*valueI;
		vCoeff[0][j] += 2*valueJ;
		A[0] += 2*(valueI+valueJ);

		// compute A[1], A[2], B[0], C[0], vCoeff[1], vCoeff[2], gCoeff[0]
		Point2D intgI = rotateR(kappa[0]*kappaIIntgIJ2+2*tmpI.x*intgIJ1+kappaI[1]*kappaConjIntgIJ0);
		Point2D intgJ = rotateR(kappa[0]*kappaJIntgIJ2+2*tmpJ.x*intgIJ1+kappaJ[1]*kappaConjIntgIJ0);
		vCoeff[1][i] += 3*intgI.x;
		vCoeff[1][j] += 3*intgJ.x;
		vCoeff[2][i] += 3*intgI.y;
		vCoeff[2][j] += 3*intgJ.y;
		B[0] += (intgI.x+intgJ.x);
		C[0] += (intgI.y+intgJ.y);
		A[1] += 3*(intgI.x+intgJ.x);
		A[2] += 3*(intgI.y+intgJ.y);
		double gIJ = -vecIJ.x*(intgI.x+intgJ.x)-vecIJ.y*(intgI.y+intgJ.y);
		gnCoeff[0][2*i+0] += (vecIJ.y*intgI.x-vecIJ.x*intgI.y);
		gnCoeff[0][2*i+1] += (vecIJ.y*intgJ.x-vecIJ.x*intgJ.y);
		vCoeff[0][i] -= gIJ*invDistIJ;
		vCoeff[0][j] += gIJ*invDistIJ;

		// compute B[1], B[2], C[1], C[2], gCoeff[1], gCoeff[2]
		intgI = rotateR(kappaIIntgIJ2+kappaI[1]*intgIJ1);
		intgJ = rotateR(kappaJIntgIJ2+kappaJ[1]*intgIJ1);
		valueI = 2*(kappaI[0]*intgIJ1).y;
		valueJ = 2*(kappaJ[0]*intgIJ1).y;
		double coscosI = (valueI+intgI.x)/2;
		double sinsinI = (valueI-intgI.x)/2;
		double sincosI = intgI.y/2;
		double coscosJ = (valueJ+intgJ.x)/2;
		double sinsinJ = (valueJ-intgJ.x)/2;
		double sincosJ = intgJ.y/2;
		B[1] += 2*(coscosI+coscosJ);
		B[2] += 2*(sincosI+sincosJ);
		C[1] += 2*(sincosI+sincosJ);
		C[2] += 2*(sinsinI+sinsinJ);
		gIJ = -vecIJ.x*(coscosI+coscosJ)-vecIJ.y*(sincosI+sincosJ);
		gnCoeff[1][2*i+0] += (vecIJ.y*coscosI-vecIJ.x*sincosI);
		gnCoeff[1][2*i+1] += (vecIJ.y*coscosJ-vecIJ.x*sincosJ);
		vCoeff[1][i] -= gIJ*invDistIJ;
		vCoeff[1][j] += gIJ*invDistIJ;
		gIJ = -vecIJ.x*(sincosI+sincosJ)-vecIJ.y*(sinsinI+sinsinJ);
		gnCoeff[2][2*i+0] += (vecIJ.y*sincosI-vecIJ.x*sinsinI);
		gnCoeff[2][2*i+1] += (vecIJ.y*sincosJ-vecIJ.x*sinsinJ);
		vCoeff[2][i] -= gIJ*invDistIJ;
		vCoeff[2][j] += gIJ*invDistIJ;

		// compute the cubic components for vCoeff[*] and gtCoeff[*]
		tmpI = kappaI[1]*kappaJ[0];
		tmpJ = kappaJ[1]*kappaI[0];
		valueI = 2*distIJ*(kappaI2*kappaJIntgIJ2+(tmpI+2*tmpI.x)*kappaI[0]*intgIJ1).y;
		valueJ = 2*distIJ*(kappaJ2*kappaIIntgIJ2+(tmpJ+2*tmpJ.x)*kappaJ[0]*intgIJ1).y;
		gtCoeff[0][2*i+0] += 2*valueI;
		gtCoeff[0][2*i+1] += 2*valueJ;

		Point2D tmpIntgII = (kappaI2*intgIJ2+2*kappaIAbsSquare*intgIJ1+conj(kappaI2)*intgIJ0);
		Point2D tmpIntgJJ = (kappaJ2*intgIJ2+2*kappaJAbsSquare*intgIJ1+conj(kappaJ2)*intgIJ0);
		Point2D tmpIntgIJ = (kappaIJ*intgIJ2+(tmpI+tmpJ).x*intgIJ1+conj(kappaIJ)*intgIJ0);
		intgI = rotateR(tmpIntgII-2*tmpIntgIJ);
		intgJ = rotateR(tmpIntgJJ-2*tmpIntgIJ);
		gtCoeff[0][2*i+0] -= (vecIJ.x*intgI.x+vecIJ.y*intgI.y);
		gtCoeff[0][2*i+1] += (vecIJ.x*intgJ.x+vecIJ.y*intgJ.y);

		double kappaAbs = modulus(kappa[0]);
		double invKappaAbs = 1.00/kappaAbs;
		Point2D invKappa = kappa[1]*invKappaAbs*invKappaAbs;
		Point2D kappaR = kappa[1]*invKappa;
		Point2D intgZ1 = (atan(z[j]*kappa[0]*invKappaAbs)-atan(z[i]*kappa[0]*invKappaAbs))*invKappaAbs;
		Point2D intgZ0 = intgIJ1*invKappa-intgZ1*kappaR;

		Point2D sI = kappaI3*invKappa;
		Point2D tI = kappaI2*(3*kappaI[1]-kappaI[0]*kappaR);
		Point2D sJ = kappaJ3*invKappa;
		Point2D tJ = kappaJ2*(3*kappaJ[1]-kappaJ[0]*kappaR);
		intgI = distIJ*rotateR(tmpIntgII-(sI*intgIJ2+conj(sI)*intgIJ0+tI*intgZ0+conj(tI)*intgZ1));
		intgJ = distIJ*rotateR(tmpIntgJJ-(sJ*intgIJ2+conj(sJ)*intgIJ0+tJ*intgZ0+conj(tJ)*intgZ1));
		gtCoeff[1][2*i+0] += 3*intgI.x;
		gtCoeff[1][2*i+1] += 3*intgJ.x;
		gtCoeff[2][2*i+0] += 3*intgI.y;
		gtCoeff[2][2*i+1] += 3*intgJ.y;

		sI = 3*kappaI2*invKappa-2*kappaI[0];
		double uI = 6*kappaIAbsSquare-6*(kappaI2*kappaR).x;
		sJ = 3*kappaJ2*invKappa-2*kappaJ[0];
		double uJ = 6*kappaJAbsSquare-6*(kappaJ2*kappaR).x;
		intgI = rotateR(sI*intgIJ2+conj(sI)*intgIJ1+uI*intgZ0);
		intgJ = rotateR(sJ*intgIJ2+conj(sJ)*intgIJ1+uJ*intgZ0);
		valueI = 2*(sI*intgIJ1).y+2*uI*intgZ1.y;
		valueJ = 2*(sJ*intgIJ1).y+2*uJ*intgZ1.y;
		coscosI = (valueI+intgI.x)/2;
		sinsinI = (valueI-intgI.x)/2;
		sincosI = intgI.y/2;
		coscosJ = (valueJ+intgJ.x)/2;
		sinsinJ = (valueJ-intgJ.x)/2;
		sincosJ = intgJ.y/2;
		gtCoeff[1][2*i+0] -= (vecIJ.x*coscosI+vecIJ.y*sincosI);
		gtCoeff[1][2*i+1] += (vecIJ.x*coscosJ+vecIJ.y*sincosJ);
		gtCoeff[2][2*i+0] -= (vecIJ.x*sincosI+vecIJ.y*sinsinI);
		gtCoeff[2][2*i+1] += (vecIJ.x*sincosJ+vecIJ.y*sinsinJ);

		for(int k = 0;  k < 3;  k++) {
			vCoeff[k][i] += (gtCoeff[k][2*i+0]-gtCoeff[k][2*i+1])*invDistIJ;
			vCoeff[k][j] -= (gtCoeff[k][2*i+0]-gtCoeff[k][2*i+1])*invDistIJ;
		}
	}
	double lambda[3] = {B[1]*C[2]-B[2]*C[1], B[2]*C[0]-B[0]*C[2], B[0]*C[1]-B[1]*C[0]};
	double sum = A[0]*lambda[0]+A[1]*lambda[1]+A[2]*lambda[2];
	double invSum = 1.00/sum;
	if(sum != 0.00) {
		lambda[0] *= invSum;
		lambda[1] *= invSum;
		lambda[2] *= invSum;
	}
	for(int i = 0;  i < int(poly.size());  i++) {
		vCoords[i] = lambda[0]*vCoeff[0][i]+lambda[1]*vCoeff[1][i]+lambda[2]*vCoeff[2][i];
	}
	for(int j = 0;  j < 2*int(poly.size());  j++) {
		gnCoords[j] = lambda[0]*gnCoeff[0][j]+lambda[1]*gnCoeff[1][j]+lambda[2]*gnCoeff[2][j];
		gtCoords[j] = lambda[0]*gtCoeff[0][j]+lambda[1]*gtCoeff[1][j]+lambda[2]*gtCoeff[2][j];
	}
}

void boundaryCoords(const vector<Point2D> &poly, const vector<int> &edge, const Point2D &p, vector<double> &vCoords, vector<double> &gnCoords,
	vector<double> &gtCoords, int k) {
	for(int h = 0;  h < int(poly.size());  h++) {
		vCoords[h] = 0.00;
	}
	for(int h = 0;  h < int(edge.size());  h++) {
		gnCoords[h] = 0.00;
		gtCoords[h] = 0.00;
	}
	int i = edge[2*k+0];
	int j = edge[2*k+1];
	double distI = dist(p, poly[i]);
	double distJ = dist(p, poly[j]);
	double distIJ = dist(poly[i], poly[j]);
	double alphaI = distJ/(distI+distJ);
	double alphaJ = distI/(distI+distJ);
	double cubicI = alphaI*alphaI*alphaJ;
	double cubicJ = alphaJ*alphaJ*alphaI;
	vCoords[i] = alphaI+cubicI-cubicJ;
	vCoords[j] = alphaJ+cubicJ-cubicI;
	gtCoords[2*k+0] = distIJ*cubicI;
	gtCoords[2*k+1] = distIJ*cubicJ;
}

void cubicMVCs(const vector<Point2D> &poly, const vector<int> &edge, const Point2D &p, vector<double> &vCoords, vector<double> &gnCoords, vector<double> &gtCoords) {
/*	if(checkPolygon(poly) == false) {
		return;
	}
*/
	y.resize(int(poly.size()));
	z.resize(int(poly.size()));
	for(int i = 0;  i < int(poly.size());  i++) {
		y[i] = poly[i]-p;
		z[i] = y[i]/modulus(y[i]);
	}
	double A[3] = {0.00, 0.00, 0.00};
	double B[3] = {0.00, 0.00, 0.00};
	double C[3] = {0.00, 0.00, 0.00};
	for(int i = 0;  i < 3;  i++) {
		vCoeff[i].resize(int(poly.size()));
		gnCoeff[i].resize(int(edge.size()));
		gtCoeff[i].resize(int(edge.size()));
		for(int j = 0;  j < int(poly.size());  j++) {
			vCoeff[i][j] = 0.00;
		}
		for(int j = 0;  j < int(edge.size());  j++) {
			gnCoeff[i][j] = 0.00;
			gtCoeff[i][j] = 0.00;
		}
	}
	for(int k = 0;  k < int(edge.size())/2;  k++) {
		int i = edge[2*k+0];
		int j = edge[2*k+1];
		double areaIJ = area(y[j], y[i]);
		double distSquareIJ = distSquare(y[i], y[j]);
		if(fabs(areaIJ) < 1E-10*distSquareIJ) {
			if(crossOrigin(y[i], y[j]) == true) {
				boundaryCoords(poly, edge, p, vCoords, gnCoords, gtCoords, k);
				return;
			}else {
				continue;
			}
		}
		double distIJ = sqrt(distSquareIJ);
		double invDistIJ = 1.00/distIJ;

		Point2D alphaI = y[j]/areaIJ;
		Point2D kappaI[2] = {Point2D(alphaI.y/2, alphaI.x/2), Point2D(alphaI.y/2, -alphaI.x/2)};
		Point2D alphaJ = y[i]/(-areaIJ);
		Point2D kappaJ[2] = {Point2D(alphaJ.y/2, alphaJ.x/2), Point2D(alphaJ.y/2, -alphaJ.x/2)};
		Point2D kappa[2] = {kappaI[0]+kappaJ[0], kappaI[1]+kappaJ[1]};

		Point2D intgIJ2 = (z[j]*z[j]*z[j]-z[i]*z[i]*z[i])/3;
		Point2D intgIJ1 = z[j]-z[i];
		Point2D intgIJ0 = conj(z[i])-conj(z[j]);

		Point2D vecIJ = (y[j]-y[i])*invDistIJ;

		// cache intermediate variables to accelerate the computation
		Point2D kappaSquare = kappa[0]*kappa[0];
		Point2D kappaIntgIJ1 = kappa[0]*intgIJ1;
		Point2D kappaConjIntgIJ0 = kappa[1]*intgIJ0;
		Point2D kappaIJ = kappaI[0]*kappaJ[0];
		Point2D kappaKappaI = kappa[0]*kappaI[0];
		Point2D kappaI2 = kappaI[0]*kappaI[0];
		Point2D kappaI3 = kappaI[0]*kappaI2;
		Point2D kappaIIntgIJ2 = kappaI[0]*intgIJ2;
		double kappaIAbsSquare = (kappaI[0]*kappaI[1]).x;
		Point2D kappaKappaJ = kappa[0]*kappaJ[0];
		Point2D kappaJ2 = kappaJ[0]*kappaJ[0];
		Point2D kappaJ3 = kappaJ[0]*kappaJ2;
		Point2D kappaJIntgIJ2 = kappaJ[0]*intgIJ2;
		double kappaJAbsSquare = (kappaJ[0]*kappaJ[1]).x;

		// compute A[0], vCoeff[0]
		Point2D tmpI = kappa[1]*kappaI[0];
		Point2D tmpJ = kappa[1]*kappaJ[0];
		double valueI = 2*(kappaSquare*kappaIIntgIJ2+(tmpI+2*tmpI.x)*kappaIntgIJ1).y;
		double valueJ = 2*(kappaSquare*kappaJIntgIJ2+(tmpJ+2*tmpJ.x)*kappaIntgIJ1).y;
		vCoeff[0][i] += 2*valueI;
		vCoeff[0][j] += 2*valueJ;
		A[0] += 2*(valueI+valueJ);

		// compute A[1], A[2], B[0], C[0], vCoeff[1], vCoeff[2], gCoeff[0]
		Point2D intgI = rotateR(kappa[0]*kappaIIntgIJ2+2*tmpI.x*intgIJ1+kappaI[1]*kappaConjIntgIJ0);
		Point2D intgJ = rotateR(kappa[0]*kappaJIntgIJ2+2*tmpJ.x*intgIJ1+kappaJ[1]*kappaConjIntgIJ0);
		vCoeff[1][i] += 3*intgI.x;
		vCoeff[1][j] += 3*intgJ.x;
		vCoeff[2][i] += 3*intgI.y;
		vCoeff[2][j] += 3*intgJ.y;
		B[0] += (intgI.x+intgJ.x);
		C[0] += (intgI.y+intgJ.y);
		A[1] += 3*(intgI.x+intgJ.x);
		A[2] += 3*(intgI.y+intgJ.y);
		double gIJ = -vecIJ.x*(intgI.x+intgJ.x)-vecIJ.y*(intgI.y+intgJ.y);
		gnCoeff[0][2*k+0] += (vecIJ.y*intgI.x-vecIJ.x*intgI.y);
		gnCoeff[0][2*k+1] += (vecIJ.y*intgJ.x-vecIJ.x*intgJ.y);
		vCoeff[0][i] -= gIJ*invDistIJ;
		vCoeff[0][j] += gIJ*invDistIJ;

		// compute B[1], B[2], C[1], C[2], gCoeff[1], gCoeff[2]
		intgI = rotateR(kappaIIntgIJ2+kappaI[1]*intgIJ1);
		intgJ = rotateR(kappaJIntgIJ2+kappaJ[1]*intgIJ1);
		valueI = 2*(kappaI[0]*intgIJ1).y;
		valueJ = 2*(kappaJ[0]*intgIJ1).y;
		double coscosI = (valueI+intgI.x)/2;
		double sinsinI = (valueI-intgI.x)/2;
		double sincosI = intgI.y/2;
		double coscosJ = (valueJ+intgJ.x)/2;
		double sinsinJ = (valueJ-intgJ.x)/2;
		double sincosJ = intgJ.y/2;
		B[1] += 2*(coscosI+coscosJ);
		B[2] += 2*(sincosI+sincosJ);
		C[1] += 2*(sincosI+sincosJ);
		C[2] += 2*(sinsinI+sinsinJ);
		gIJ = -vecIJ.x*(coscosI+coscosJ)-vecIJ.y*(sincosI+sincosJ);
		gnCoeff[1][2*k+0] += (vecIJ.y*coscosI-vecIJ.x*sincosI);
		gnCoeff[1][2*k+1] += (vecIJ.y*coscosJ-vecIJ.x*sincosJ);
		vCoeff[1][i] -= gIJ*invDistIJ;
		vCoeff[1][j] += gIJ*invDistIJ;
		gIJ = -vecIJ.x*(sincosI+sincosJ)-vecIJ.y*(sinsinI+sinsinJ);
		gnCoeff[2][2*k+0] += (vecIJ.y*sincosI-vecIJ.x*sinsinI);
		gnCoeff[2][2*k+1] += (vecIJ.y*sincosJ-vecIJ.x*sinsinJ);
		vCoeff[2][i] -= gIJ*invDistIJ;
		vCoeff[2][j] += gIJ*invDistIJ;

		// compute the cubic components for vCoeff[*] and gtCoeff[*]
		tmpI = kappaI[1]*kappaJ[0];
		tmpJ = kappaJ[1]*kappaI[0];
		valueI = 2*distIJ*(kappaI2*kappaJIntgIJ2+(tmpI+2*tmpI.x)*kappaI[0]*intgIJ1).y;
		valueJ = 2*distIJ*(kappaJ2*kappaIIntgIJ2+(tmpJ+2*tmpJ.x)*kappaJ[0]*intgIJ1).y;
		gtCoeff[0][2*k+0] += 2*valueI;
		gtCoeff[0][2*k+1] += 2*valueJ;

		Point2D tmpIntgII = (kappaI2*intgIJ2+2*kappaIAbsSquare*intgIJ1+conj(kappaI2)*intgIJ0);
		Point2D tmpIntgJJ = (kappaJ2*intgIJ2+2*kappaJAbsSquare*intgIJ1+conj(kappaJ2)*intgIJ0);
		Point2D tmpIntgIJ = (kappaIJ*intgIJ2+(tmpI+tmpJ).x*intgIJ1+conj(kappaIJ)*intgIJ0);
		intgI = rotateR(tmpIntgII-2*tmpIntgIJ);
		intgJ = rotateR(tmpIntgJJ-2*tmpIntgIJ);
		gtCoeff[0][2*k+0] -= (vecIJ.x*intgI.x+vecIJ.y*intgI.y);
		gtCoeff[0][2*k+1] += (vecIJ.x*intgJ.x+vecIJ.y*intgJ.y);

		double kappaAbs = modulus(kappa[0]);
		double invKappaAbs = 1.00/kappaAbs;
		Point2D invKappa = kappa[1]*invKappaAbs*invKappaAbs;
		Point2D kappaR = kappa[1]*invKappa;
		Point2D intgZ1 = (atan(z[j]*kappa[0]*invKappaAbs)-atan(z[i]*kappa[0]*invKappaAbs))*invKappaAbs;
		Point2D intgZ0 = intgIJ1*invKappa-intgZ1*kappaR;

		Point2D sI = kappaI3*invKappa;
		Point2D tI = kappaI2*(3*kappaI[1]-kappaI[0]*kappaR);
		Point2D sJ = kappaJ3*invKappa;
		Point2D tJ = kappaJ2*(3*kappaJ[1]-kappaJ[0]*kappaR);
		intgI = distIJ*rotateR(tmpIntgII-(sI*intgIJ2+conj(sI)*intgIJ0+tI*intgZ0+conj(tI)*intgZ1));
		intgJ = distIJ*rotateR(tmpIntgJJ-(sJ*intgIJ2+conj(sJ)*intgIJ0+tJ*intgZ0+conj(tJ)*intgZ1));
		gtCoeff[1][2*k+0] += 3*intgI.x;
		gtCoeff[1][2*k+1] += 3*intgJ.x;
		gtCoeff[2][2*k+0] += 3*intgI.y;
		gtCoeff[2][2*k+1] += 3*intgJ.y;

		sI = 3*kappaI2*invKappa-2*kappaI[0];
		double uI = 6*kappaIAbsSquare-6*(kappaI2*kappaR).x;
		sJ = 3*kappaJ2*invKappa-2*kappaJ[0];
		double uJ = 6*kappaJAbsSquare-6*(kappaJ2*kappaR).x;
		intgI = rotateR(sI*intgIJ2+conj(sI)*intgIJ1+uI*intgZ0);
		intgJ = rotateR(sJ*intgIJ2+conj(sJ)*intgIJ1+uJ*intgZ0);
		valueI = 2*(sI*intgIJ1).y+2*uI*intgZ1.y;
		valueJ = 2*(sJ*intgIJ1).y+2*uJ*intgZ1.y;
		coscosI = (valueI+intgI.x)/2;
		sinsinI = (valueI-intgI.x)/2;
		sincosI = intgI.y/2;
		coscosJ = (valueJ+intgJ.x)/2;
		sinsinJ = (valueJ-intgJ.x)/2;
		sincosJ = intgJ.y/2;
		gtCoeff[1][2*k+0] -= (vecIJ.x*coscosI+vecIJ.y*sincosI);
		gtCoeff[1][2*k+1] += (vecIJ.x*coscosJ+vecIJ.y*sincosJ);
		gtCoeff[2][2*k+0] -= (vecIJ.x*sincosI+vecIJ.y*sinsinI);
		gtCoeff[2][2*k+1] += (vecIJ.x*sincosJ+vecIJ.y*sinsinJ);

		for(int h = 0;  h < 3;  h++) {
			vCoeff[h][i] += (gtCoeff[h][2*k+0]-gtCoeff[h][2*k+1])*invDistIJ;
			vCoeff[h][j] -= (gtCoeff[h][2*k+0]-gtCoeff[h][2*k+1])*invDistIJ;
		}
	}
	double lambda[3] = {B[1]*C[2]-B[2]*C[1], B[2]*C[0]-B[0]*C[2], B[0]*C[1]-B[1]*C[0]};
	double sum = A[0]*lambda[0]+A[1]*lambda[1]+A[2]*lambda[2];
	double invSum = 1.00/sum;
	if(sum != 0.00) {
		lambda[0] *= invSum;
		lambda[1] *= invSum;
		lambda[2] *= invSum;
	}
	for(int i = 0;  i < int(poly.size());  i++) {
		vCoords[i] = lambda[0]*vCoeff[0][i]+lambda[1]*vCoeff[1][i]+lambda[2]*vCoeff[2][i];
	}
	for(int j = 0;  j < int(edge.size());  j++) {
		gnCoords[j] = lambda[0]*gnCoeff[0][j]+lambda[1]*gnCoeff[1][j]+lambda[2]*gnCoeff[2][j];
		gtCoords[j] = lambda[0]*gtCoeff[0][j]+lambda[1]*gtCoeff[1][j]+lambda[2]*gtCoeff[2][j];
	}
}
