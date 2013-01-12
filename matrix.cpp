#include <cstdio>
#include <cstring>

#include "matrix.h"


Mat4::Mat4() // this constructor initializes an identity matrix
{
	// http://stackoverflow.com/questions/2516096/fastest-way-to-zero-out-a-2d-array-in-c
	memset(data, 0, 16 * sizeof(float)); // 4 * 4 matrix, float variables --> 4*4*sizeof(float) = 64

	for (int i = 0; i < 4; i++)
			data[5*i] = 1.0;
	
}

Mat4::Mat4(float *input) 
{
	if (input)
	{
		memcpy(data, input, 16 * sizeof(float));
	}

	else { memset(data, 0, 64); }

}

Mat4 Mat4::operator*(Mat4 const &other)  // returns a column major matrix
{
	Mat4 result;

	int i, j;

	  for (i = 0; i < 4; i++)
	  {
		  for (j = 0; j < 4; j++)
		  {
				result.data[i+4*j] = this->data[4*i] * other.data[j]
						   + this->data[4*i+1] * other.data[j + 4]  
						   + this->data[4*i+2] * other.data[j + 8]
						   + this->data[4*i+3] * other.data[j + 12];
		  }
	  }

	return result;

}

Mat4& Mat4::operator=(Mat4 const &other)
{
	memcpy(this->data, other.data, 16*sizeof(float));
	return *this;
}

void Mat4::print() const
{
	
	for (int i = 0; i < 4; i++)
		printf("%f %f %f %f\n", data[i], data[i + 4], data[i + 8], data[i + 12]);
	
}

Mat4 Mat4::transpose()
{
	Mat4 result;
	
	float temp[16];

	memcpy(temp, this->data, 16*sizeof(float));

	for (int i = 0; i < 4; i++) // i = 1 because the first element is already in the right place
	{
		for (int j = 0; j < 4; j++)
			result.data[4*i+j] = temp[i + 4*j];
	}

	return result;

}


void Mat4::updateColumn(int column, Vec3 const &a)
{
	this->data[(column-1)*4] = a.direction.x;
	this->data[(column-1)*4 + 1] = a.direction.y;
	this->data[(column-1)*4 + 2] = a.direction.z;
}

void Mat4::updateRow(int row, Vec3 const &a)
{
	this->data[(row-1)] = a.direction.x;
	this->data[(row-1) + 4] = a.direction.y;
	this->data[(row-1) + 8] = a.direction.z;
}

void Mat4::updateColumn(int column, Vec4 const &a)
{
	this->data[(column-1)*4] = a.direction.x;
	this->data[(column-1)*4 + 1] = a.direction.y;
	this->data[(column-1)*4 + 2] = a.direction.z;
	this->data[(column-1)*4 + 3] = a.direction.w;
}

void Mat4::updateRow(int row, Vec4 const &a)
{
	this->data[(row-1)] = a.direction.x;
	this->data[(row-1) + 4] = a.direction.y;
	this->data[(row-1) + 8] = a.direction.z;
	this->data[(row-1) + 12] = a.direction.w;
}