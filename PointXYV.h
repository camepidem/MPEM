//PointXYV.h

//Template for PointXYV class
#ifndef POINTXYV_H
#define POINTXYV_H 1

template <class T>
class PointXYV {

public:

	PointXYV(int _x, int _y, T _value);
	virtual ~PointXYV();

	int x, y;
	T value;
	
protected:

	
};

template <class T>
PointXYV<T>::PointXYV(int _x, int _y, T _value) {

	x = _x;
	y = _y;
	value = _value;
	
}



template <class T>
PointXYV<T>::~PointXYV() {

}


//#include "PointXYV.cpp"


#endif