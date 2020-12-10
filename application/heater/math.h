#ifndef HEATER_MATH_H
#define HEATER_MATH_H


#define MAX(X,Y)  (((X) > (Y)) ? (X) : (Y))

#define NR_REPEATS 16
#define LOG10_e 0.4342944819

static __inline double sqrt(double _x){
    double _y = 0.0;
    double tmp = _x;
    int k;

    for(k=0,_y=tmp ; k < NR_REPEATS ; k++){
        if(_y < 1.0)
            break;
        _y = ((_y*_y)+tmp)/(2.0*_y);
    }
    
    return _y;
}

static __inline double log(double x){

	if(x<0.0){	
        return -0.0;
    }
	
	double result, i;
	double clen = 0;
	double z = (x-1)/(1+x);
	double z2 = z*z;

	for(i = 10; i > 1; i--){
		
		clen = ((i-1)*(i-1)* z2 )/ ((2*i - 1) - clen);
	}
	
	result = (2*z) / (1 - clen);
	result /=LOG10_e;
	return result;
}

static __inline double fabs (double _x) {
    double _y;
    _y = (_x>0) ? (_x): (_x*-1.0);
    return _y;
}
#endif
