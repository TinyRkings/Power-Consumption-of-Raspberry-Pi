#include "ParallelKMeans.h"

using namespace std;
using namespace cv;


int getMaxVal(int R, int G, int B){
	if(R > G && R > B){
		return R;
	}else if(G > R && G > B){
		return G;
	}else{
		return B;
	}
}

int getMinVal(int R, int G, int B){
	if(R < G && R <B)
		return R;
	else if(G < R && G < B)
		return G;
	else
		return B;
}

// r,g,b values are from 0 to 255
// h = [0,360], s = [0,100 %], v = [0,100 %]
// if s == 0, then h = -1 (undefined) 
HSV convertBGRtoHSV(int B, int G, int R ){
	HSV hsv;
	int max = getMaxVal(R, G, B);
	int min = getMinVal(R, G, B);
	int delta = max - min;
	
	hsv.V = max/2.55; //hsv.V = max;
	
	if( max != 0 )
		hsv.S = (max - min) / max;
	else {
		// r = g = b = 0 
		// s = 0, v is undefined
		hsv.S = 0;
		hsv.H = -1;
 		return hsv;
 	}
 	if(delta == 0)
		hsv.H = 0;
	else if ( R == max )
		hsv.H = 60 * ( G - B ) / delta; // between yellow & magenta
	else if ( G == max )
		hsv.H = 60 * ( B - R ) / delta + 120; // between cyan & yellow
	else if ( B == max )
 		hsv.H = 60 * ( R - G ) / delta + 240; // between magenta & cyan
 	if ( hsv.H < 0)
 		hsv.H += 360;

	return hsv;
}

//Dimensions 288 = 18* 4* 4
int Cal_bucket(HSV hsv_value){
	int vv = hsv_value.V / 25; //0, 1 ,2 , 3
	if(vv == 4)
		vv = vv-1;
	int ss = hsv_value.S / 25; //0, 1, 2, 3
	if(ss == 4)
		ss = ss-1;
	int hh = hsv_value.H / 20; //0, 1 ,2 ,... , 17
	if(hh == 18)
		hh = hh-1;
	return 18*4*vv+18*ss+hh;
}


/*
void initPoints(int number_points, struct mycentroid *points){
	string imagepath1 = "Corel/";
	for(int i=0; i<number_points; i++){
		//string imagepath = "Corel/1.jpg";
		string imagepath = imagepath1 + to_string(i+1) + ".jpg";
		//cout<<imagepath<<endl;
		one_image(imagepath, points, i);
	}
}
*/