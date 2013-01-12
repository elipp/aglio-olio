#ifndef MATRIX_H
#define MATRIX_H
#include "vector.h"

class Mat4 {
	
private:
	float data[16];

public:
	Mat4();				// creates an identity matrix
	Mat4(float* input); // creates a Mat4 object based on the data
	Mat4 operator*(Mat4 const &);
	Mat4& operator=(Mat4 const &);
	Mat4 transpose();
	void updateColumn(int column, Vec3 const &a);
	void updateColumn(int column, Vec4 const &a);
	void updateRow(int row, Vec3 const &a);
	void updateRow(int row, Vec4 const &a);
	float* getData() { return &data[0];}
	
	void print() const;
};



#endif