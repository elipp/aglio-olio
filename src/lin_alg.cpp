#include "lin_alg.h"

#ifdef _WIN32
static const __m128 ZERO = _mm_setzero_ps();
static const __m128 MINUS_ONES = _mm_set_ps(-1.0, -1.0, -1.0, -1.0);
static const __m128 QUAT_CONJUGATE = _mm_set_ps(1.0, -1.0, -1.0, -1.0);	// in reverse order!
static const __m128 QUAT_NO_ROTATION = _mm_set_ps(1.0, 0.0, 0.0, 0.0);
static const int mask3021 = 0xC9, // 11 00 10 01_2
				mask3102 = 0xD2, // 11 01 00 10_2
				mask1032 = 0x4E, // 01 00 11 10_2
				xyz_dot_mask = 0x71,
				xyzw_dot_mask = 0xF1;

static float (*MM_DPPS_XYZ)(__m128 a, __m128 b);
static float (*MM_DPPS_XYZW)(__m128 a, __m128 b);
#endif

static const float identity_arr[] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 };

const vec4 vec4::zero_const = vec4(ZERO);
const mat4 mat4::identity_const = mat4(identity_arr);

static inline float MM_DPPS_XYZ_SSE(__m128 a, __m128 b) {
	const __m128 mul = _mm_mul_ps(a, b);
	return mul.m128_f32[0]+mul.m128_f32[1]+mul.m128_f32[2];
}

static inline float MM_DPPS_XYZW_SSE(__m128 a, __m128 b) {
	const __m128 mul = _mm_mul_ps(a, b);	
	return mul.m128_f32[0]+mul.m128_f32[1]+mul.m128_f32[2] + mul.m128_f32[3];
}

static inline float MM_DPPS_XYZ_SSE3(__m128 a, __m128 b) {
	 __m128 mul = _mm_mul_ps(a, b);
	 mul.m128_f32[3] = 0.0;	// zeroing out a f32 field in a __m128 union
							// generates only two instructions, a fldz and a fstp
	 mul = _mm_hadd_ps(mul, mul);
	 return _mm_hadd_ps(mul, mul).m128_f32[0]; 
}

static inline float MM_DPPS_XYZW_SSE3(__m128 a, __m128 b) {
	__m128 mul = _mm_mul_ps(a, b);
	 mul = _mm_hadd_ps(mul, mul);
	 return _mm_hadd_ps(mul, mul).m128_f32[0];
}

static inline float MM_DPPS_XYZ_SSE41(__m128 a, __m128 b) {
	return _mm_dp_ps(a, b, xyz_dot_mask).m128_f32[0];	
}

static inline float MM_DPPS_XYZW_SSE41(__m128 a, __m128 b) {
	return _mm_dp_ps(a, b, xyzw_dot_mask).m128_f32[0];
}

// fallback

const char* checkCPUCapabilities() {
#ifdef _WIN32
	int a[4];
	__cpuid(a, 1);

	if (!(a[3] & (0x1 << 25))) {
		return "SSE not supported by host processor!";
	}
	else if (!(a[2] & (0x1))) {
		printf("SSE3 not supported by host processor. Using SSE for dot product computation.\n"); 
		MM_DPPS_XYZ = MM_DPPS_XYZ_SSE;
		MM_DPPS_XYZW = MM_DPPS_XYZW_SSE;
	}
	
	else if (!(a[2] & (0x1 << 19))) {
		printf("NOTE: SSE4.1 not supported by host processor. Using SSE3 for dot product computation.\n");
		MM_DPPS_XYZ = MM_DPPS_XYZ_SSE3;
		MM_DPPS_XYZW = MM_DPPS_XYZW_SSE3;
	}
	else { 
		printf("Using SSE4.1 for _mm_dp_ps.\n");
		MM_DPPS_XYZ = MM_DPPS_XYZ_SSE41;
		MM_DPPS_XYZW = MM_DPPS_XYZW_SSE41;
	}
	
	return "OK";

#elif __linux__

	// nyi

#endif
}

vec4::vec4(float _x, float _y, float _z, float _w) {

#ifdef _WIN32
	// must use _mm_setr_ps for this constructor. Note 'r' for reversed.
	//data = _mm_setr_ps(_x, _y, _z, _w);
	data = _mm_set_ps(_w, _z, _y, _x);	// could be faster, haven't tested
#elif __linux__

	data[0] = _x;
	data[1] = _y;
	data[2] = _z;
	data[3] = _w;
#endif
}


vec4::vec4() {
#ifdef _WIN32
	data = ZERO;
#elif __linux__
	memset(data, 0, sizeof(data)); 
#endif

}

// obviously enough, this requires a 4-float array as argument
vec4::vec4(const float* const a) {
#ifdef _WIN32
	data = _mm_loadu_ps(a);		// not assuming 16-byte alignment for a.
#elif __linux__
	memcpy(data, a, sizeof(data));
#endif

}

void vec4::operator*=(float scalar) {

#ifdef _WIN32
	// use xmmintrin
	// data = _mm_mul_ps(data, _mm_load1_ps(&scalar)); // not quite sure this works
	data = _mm_mul_ps(data, _mm_set1_ps(scalar));	// the disassembly is pretty much identical between the two though
#elif __linux__

	vec4 &a = (*this);
	a(0) *= scalar;
	a(1) *= scalar;
	a(2) *= scalar;
	a(3) *= scalar;

#endif


}

vec4 operator*(float scalar, const vec4& v) {

	vec4 r(v.data);
	r *= scalar;
	return r;

}	

vec4 vec4::operator*(float scalar) const{

	vec4 v(this->data);
	v *= scalar;
	return v;

}

void vec4::operator/=(float scalar) {
	// _mm_rcp_ps returns a rough approximation
	const __m128 scalar_recip = _mm_rcp_ps(_mm_set1_ps(scalar));
	this->data = _mm_mul_ps(this->data, scalar_recip);

}


vec4 vec4::operator/(float scalar) const {
	vec4 v = (*this);
	v /= scalar;
	return v;
}

void vec4::operator+=(const vec4 &b) {
	
	this->data=_mm_add_ps(data, b.data);

}

vec4 vec4::operator+(const vec4 &b) const {
#ifdef _WIN32

	vec4 v = (*this);
	v += b; 
	return v;
	
#elif __linux__

	return vec4(data[0]+b.data[0],data[1]+b.data[1],data[2]+b.data[2],data[3]+b.data[3]);

#endif
}

void vec4::operator-=(const vec4 &b) {
	this->data=_mm_sub_ps(data, b.data);
}

vec4 vec4::operator-(const vec4 &b) const {
	vec4 v = (*this);
	v -= b;
	return v;
}

vec4 vec4::operator-() const { 
	return vec4(_mm_mul_ps(MINUS_ONES, this->data));
}

float vec4::length3() const {
#ifdef _WIN32

	//return sqrt(_mm_dp_ps(this->data, this->data, xyz_dot_mask).m128_f32[0]);
	return sqrt(MM_DPPS_XYZ(this->data, this->data));
#elif __linux__
	const vec4 &v = (*this);
	return sqrt(v.data[0]*v.data[0] + v.data[1]*v.data[1] + v.data[2]*v.data[2]);

#endif

}

float vec4::length4() const {

#ifdef _WIN32

	//return sqrt(_mm_dp_ps(this->data, this->data, xyzw_dot_mask).m128_f32[0]);	// includes x,y,z,w in the computation
	return sqrt(MM_DPPS_XYZW(this->data, this->data));
#elif __linux__
	const vec4 &v = (*this);
	return sqrt(v.data[0]*v.data[0] + v.data[1]*v.data[1] + v.data[2]*v.data[2] + v.data[3]*v.data[3]);

#endif

}

// this should actually include all components, but given the application, this won't :P

void vec4::normalize() {
#ifdef _WIN32

	// 0.08397us/iteration for this
	//const float l_recip = 1.0/sqrt(_mm_dp_ps(this->data, this->data, xyz_dot_mask).m128_f32[0]); // only x,y,z components
	//this->data = _mm_mul_ps(this->data, _mm_set1_ps(l_recip));

	// 0.07257us/iteration, probably has a bigger error margin though
	//const __m128 l_recip = _mm_rcp_ps(_mm_sqrt_ps(_mm_set1_ps(MM_DPPS_XYZ(this->data, this->data)))); // only x,y,z components
	//this->data = _mm_mul_ps(this->data, l_recip);

	// in debug mode (no optimization whatsoever), stripping the temporary value increased 
	// performance to about 0.06786us/iteration
	this->data = _mm_mul_ps(this->data, _mm_rcp_ps(_mm_sqrt_ps(_mm_set1_ps(MM_DPPS_XYZ(this->data, this->data)))));
#elif __linux__

	const float l_recip = 1.0/sqrt(length3());

	this->data[0] *= l_recip;
	this->data[1] *= l_recip;
	this->data[2] *= l_recip;

#endif

}

vec4 vec4::normalized() const {
	vec4 v(*this);
	v.normalize();
	return v;
}

void vec4::zero() {
	(*this) = vec4::zero_const;
}

void vec4::print(){

#ifdef _WIN32
	printf("(%4.3f, %4.3f, %4.3f, %4.3f)\n", data.m128_f32[0], data.m128_f32[1], data.m128_f32[2], data.m128_f32[3]);
#elif __linux__
	std::cout.precision(4);
	std::cout << std::fixed << this->data[0]<< ", " <<  this->data[1]<< ", " <<  this->data[2]<< ", " <<  this->data[3] << "\n";
#endif
}

void mat4::print() {
#ifdef _WIN32
	mat4 &M = (*this);
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++)
			printf("%4.3f ", M.elementAt(j,i));
		printf("\n");
	}
	printf("\n");
#elif __linux__
	mat4 &M = (*this);

	std::cout.precision(4);
	for (int i = 0; i < 4; i++) {
		std::cout << std::fixed << M(0, i) << " " << M(1, i) << " " << M(2, i) << " " << M(3, i) << "\n";
	}

#endif
}

void* vec4::rawData() const {
	return (void*)&data;
}


float dot(const vec4 &a, const vec4 &b) {
#ifdef _WIN32
	/*__m128 dot = _mm_dp_ps(a.data, b.data, xyz_dot_mask);	
	return dot.m128_f32[0];	*/
	return MM_DPPS_XYZ(a.data, b.data);
	
	/* SSE solution
	__m128 mul = _mm_mul_ps(a.data, b.data);
	return mul.m128_f32[0]+mul.m128_f32[1]+mul.m128_f32[2]+mul.m128_f32[3]; // for xyzw
	return mul.m128_f32[0]+mul.m128_f32[1]+mul.m128_f32[2] // for xyz only
	// the xyz version disassembly is the following:
	// fld, fadd, fadd, fstp; only four instructions.

	 //or, if SSE3 is available (xyzw)
	 __m128 mul = _mm_mul_ps(a.data, b.data);
	 mul = _mm_hadd_ps(mul, mul);
	 return _mm_hadd_ps(mul, mul).m128_f32[0];
	
	// SSE3 xyz:
	 __m128 mul = _mm_mul_ps(a.data, b.data);
	 mul.m128_f32[3] = 0.0;	// zeroing out a f32 field in a __m128 union
							// generates only two instructions, a fldz and a fstp
	 mul = _mm_hadd_ps(mul, mul);
	 return _mm_hadd_ps(mul, mul).m128_f32[0]; */

#elif __linux__

	return 0;
	//return a.data[0]*b.data[0] + a.data[1]*b.data[1] + a.data[2]*b.data[2] +a.data[3]*b.data[3];
	// NAIVE: return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
#endif

}

vec4 cross(const vec4 &a, const vec4 &b) {

	// See: http://fastcpp.blogspot.fi/2011/04/vector-cross-product-using-sse-code.html.
	// Absolutely beautiful! (the exact same recipe can be found at
	// http://neilkemp.us/src/sse_tutorial/sse_tutorial.html#E, albeit in assembly.)
	
#ifdef _WIN32
		
	return vec4(
	_mm_sub_ps(
    _mm_mul_ps(_mm_shuffle_ps(a.data, a.data, mask3021), _mm_shuffle_ps(b.data, b.data, mask3102)), 
    _mm_mul_ps(_mm_shuffle_ps(a.data, a.data, mask3102), _mm_shuffle_ps(b.data, b.data, mask3021))
  )
  );

#elif __linux__

	//NYI!
#endif
}

mat4 vec4::toTranslationMatrix() const {
	mat4 M(MAT_IDENTITY);
	M.assignToColumn(3, (*this));
	return M;
}





mat4::mat4() {
#ifdef _WIN32

	data[0] = data[1] = data[2] = data[3] = ZERO;

#elif __linux__

	memset(this->data, 0, sizeof(this->data));
#endif

}

mat4::mat4(const float *arr) {
#ifdef _WIN32
	data[0] = _mm_loadu_ps(arr);
	data[1] = _mm_loadu_ps(arr + 4);	
	data[2] = _mm_loadu_ps(arr + 8);	
	data[3] = _mm_loadu_ps(arr + 12);	
#elif __linux__
	memcpy(this->data, data, (4*4)*sizeof(float));
#endif

}

mat4::mat4(const int main_diagonal_val) {
#ifdef _WIN32
	mat4 &a = (*this);
	a.zero();
	a(0,0)=main_diagonal_val;
	a(1,1)=main_diagonal_val;
	a(2,2)=main_diagonal_val;
	a(3,3)=main_diagonal_val;
#elif __linux__
	memset(a.data, 0, sizeof(a.data));
	a(0,0)=main_diagonal_val;
	a(1,1)=main_diagonal_val;
	a(2,2)=main_diagonal_val;
	a(3,3)=main_diagonal_val;
#endif

}


// stupid constructor.

mat4::mat4(const vec4& c1, const vec4& c2, const vec4& c3, const vec4& c4) {

	data[0] = c1.getData();
	data[1] = c2.getData();
	data[2] = c3.getData();
	data[3] = c4.getData();

}

// must be passed as references lolz, otherwise the last one will lose its alignment

mat4::mat4(const __m128& c1, const __m128&  c2, const __m128&  c3, const __m128& c4) {
	data[0] = c1;
	data[1] = c2;
	data[2] = c3;
	data[3] = c4;
}

mat4 mat4::operator* (const mat4& R) const {

#ifdef _WIN32

	mat4 L = (*this).transposed();
	mat4 ret;
	
	// we'll choose to transpose the other matrix, and instead of calling the perhaps
	// more intuitive row(), we'll be calling column(), which is a LOT faster in comparison.
	
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			//ret.data[j].m128_f32[i] = _mm_dp_ps(R.data[j], L.data[i], xyzw_dot_mask).m128_f32[0];
			ret.data[j].m128_f32[i] = MM_DPPS_XYZW(R.data[j], L.data[i]);
		}
	}
	
	return ret;
#elif __linux__

	mat4 ret;
	const mat4 &L = (*this);	// for easier syntax

//	ret(0, 0) = l(0, 0)*r(0, 0) + l(1,0)*l(0,1) + l(2,0)*r(0,2) + l(3,0)*r(0,3);

for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			ret(j,i) = L.elementAt(j, 0)*R.elementAt(0, i) 
				 + L.elementAt(j, 1)*R.elementAt(1, i) 
				 + L.elementAt(j, 2)*R.elementAt(2, i) 
				 + L.elementAt(j, 3)*R.elementAt(3, i);

	return ret;

#endif
}


vec4 mat4::operator* (const vec4& R) const {

#ifdef _WIN32
	
	// try with temporary mat4? :P
	// result: performs better!
	const mat4 M = (*this).transposed();
	vec4 v;
	for (int i = 0; i < 4; i++) 
		v(i) = MM_DPPS_XYZW(M.data[i], R.getData());
		//v.getData().m128_f32[i] = _mm_dp_ps(M.data[i], R.getData(), xyzw_dot_mask).m128_f32[0];	

	return v;

#elif __linux
	vec4 v;
	const mat4 &L = (*this);

	for (int i = 0; i < 4; i++)
		v(i) = L.elementAt(0, i)*R.elementAt(0)
		       + L.elementAt(1, i)*R.elementAt(1)
		       + L.elementAt(2, i)*R.elementAt(2)
		       + L.elementAt(3, i)*R.elementAt(3);

	return v;
#endif


}

mat4 operator*(float scalar, const mat4& m) {
	const __m128 scalar_m128 = _mm_set1_ps(scalar);
	return mat4(_mm_mul_ps(scalar_m128, m.data[0]), 
				_mm_mul_ps(scalar_m128, m.data[1]),
				_mm_mul_ps(scalar_m128, m.data[2]),
				_mm_mul_ps(scalar_m128, m.data[3]));

}

void mat4::zero() {
#ifdef _WIN32
	data[0] = data[1] = data[2] = data[3] = ZERO;
#elif __linux__
	memset(data, 0, sizeof(data));
#endif
}


void mat4::identity() {
#ifdef _WIN32

	// better design than to do (*this)(0,0) = 1.0 etc
	(*this) = identity_const;

#elif __linux__
	memset(this->data, 0, sizeof(this->data));

	this->data[0][0] = 1.0;
	this->data[1][1] = 1.0;
	this->data[2][2] = 1.0;
	this->data[3][3] = 1.0;
#endif
}

vec4 mat4::row(int i) const {
#ifdef _WIN32		
	return vec4((*this).transposed().data[i]);

#elif __linux__
	return vec4(data[0][i], data[1][i], data[2][i], data[3][i]);
#endif

	// benchmarks for 100000000 iterations
	// Two transpositions, no redirection to mat4::column:	15.896s
	// Two transpositions, redirection to mat4::column:		18.332s
	// Naive implementation:								14.140s. !
	// copy, transpose:										11.354s
	
	// for comparison: column, 100000000 iterations:		4.7s

}

vec4 mat4::column(int i) const {
#ifdef _WIN32
	return vec4(this->data[i]);
#elif __linux__
	return vec4(data[0][i], data[1][i], data[2][i], data[3][i]);
#endif
}

void mat4::assignToColumn(int column, const vec4& v) {
#ifdef _WIN32
	this->data[column] = v.getData();
#elif __linux__
	// NYI
#endif

}

void mat4::assignToRow(int row, const vec4& v) {
#ifdef _WIN32
	// here, since 
	// 1. row operations are inherently slower than column operations, and
	// 2. transposition is blazing fast (:D)

	// we could just transpose the matrix and do some fancy sse shit with it.
	
	// this could (and probably should) be done with a reference, like this:
	// mat4.row(i) = vec4(...). However, this is only possible if the mat4 is 
	// constructed of actual vec4s (which is something one should consider anyway)

	this->transpose();
	this->data[row] = v.getData();
	this->transpose();
	
#elif __linux__
		mat4& M = (*this);
	M(0, row) = v.elementAt(0);
	M(1, row) = v.elementAt(1);
	M(2, row) = v.elementAt(2);
	M(3, row) = v.elementAt(3);
#endif

}
// return by void pointer? :P
void *mat4::rawData() const {
#ifdef _WIN32
	return (void*)&data[0];	// seems to work just fine :D
#elif __linux__

	// nyi

#endif


}

void mat4::printRaw() const {

	const float * const ptr = (float*)&this->data[0];
		
#ifdef _WIN32
	const char* fmt = "%4.3f %4.3f %4.3f %4.3f\n";

	for (int i = 0; i < 16; i += 4)
		printf(fmt, *(ptr + i), *(ptr + i + 1), *(ptr + i + 2), *(ptr + i + 3));
	printf ("\n");

#elif __linux__
	for (int i = 0; i < 16, i += 4)
		std::cout << std::setprecision(3) << *(ptr + i) << " " << *(ptr + i + 1) << " " << *(ptr + i + 2) << " " << *(ptr + i + 3) << "\n";
	std::cout << "\n";
#endif

	
}


void mat4::make_proj_orthographic(float left, float right, float bottom, float top, float zNear, float zFar) {

	// We could just assume here that the matrix is "clean",
	// i.e. that any matrix elements other than the ones used in
	// a pure orthographic projection matrix are zero.
		
	mat4 &M = (*this);
	
	M.identity();

	M(0,0) = 2.0/(right - left);
	M(1,1) = 2.0/(top - bottom);
	M(2,2) = -2.0/(zFar - zNear);

	M(3,0) = - (right + left) / (right - left);
	M(3,1) = - (top + bottom) / (top - bottom);
	M(3,2) = - (zFar + zNear) / (zFar - zNear);
	// the element at (3,3) is already 1 (identity() was called)
}

void mat4::make_proj_perspective(float left, float right, float bottom, float top, float zNear, float zFar) {
		

	mat4 &M = (*this);
	M.identity();

	M(0,0) = (2*zNear)/(right-left);
	M(1,1) = (2*zNear)/(top-bottom);
	M(2,0) = (right+left)/(right-left);
	M(2,1) = (top+bottom)/(top-bottom);
	M(2,2) = -(zFar + zNear)/(zFar - zNear);
	M(2,3) = -1.0;
	M(3,2) = -(2*zFar*zNear)/(zFar - zNear);
	

}

// acts like a gluPerspective call :P

void mat4::make_proj_perspective(float fov_radians, float aspect, float zNear, float zFar) {

	// used glm as a reference

	const float range = tan(fov_radians/2.0) * zNear;
	const float left = -range * aspect;
	const float right = range * aspect;

	this->make_proj_perspective(left, right, -range, range, zNear, zFar);

}

// performs an "in-place transposition" of the matrix

void mat4::transpose() {

#ifdef _WIN32

	_MM_TRANSPOSE4_PS(data[0], data[1], data[2], data[3]);	// microsoft special in-place transpose macro :P

	/* Implemented as follows: (xmmintrin.h) (no copyright though)
#define _MM_TRANSPOSE4_PS(row0, row1, row2, row3) {                 \
            __m128 tmp3, tmp2, tmp1, tmp0;                          \
                                                                    \
            tmp0   = _mm_shuffle_ps((row0), (row1), 0x44);          \
            tmp2   = _mm_shuffle_ps((row0), (row1), 0xEE);          \
            tmp1   = _mm_shuffle_ps((row2), (row3), 0x44);          \
            tmp3   = _mm_shuffle_ps((row2), (row3), 0xEE);          \
                                                                    \
            (row0) = _mm_shuffle_ps(tmp0, tmp1, 0x88);              \
            (row1) = _mm_shuffle_ps(tmp0, tmp1, 0xDD);              \
            (row2) = _mm_shuffle_ps(tmp2, tmp3, 0x88);              \
            (row3) = _mm_shuffle_ps(tmp2, tmp3, 0xDD);              \
        }
		*/

#elif __linux__

	mat4 &m = (*this);
	
	float tmp;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			tmp = m(i, j);
			m(i, j) = m(j, i);
			m(j, i) = tmp;
		}
	}

#endif
}

mat4 mat4::transposed() const {

	mat4 ret = (*this);	// copying can't be avoided
	_MM_TRANSPOSE4_PS(ret.data[0], ret.data[1], ret.data[2], ret.data[3]);
	return ret;

}


void mat4::invert() {

}

// REMEMBER TO REMOVE THIS
inline void m128printf32(__m128 a) {
	printf("%f %f %f %f\n", a.m128_f32[0], a.m128_f32[1], a.m128_f32[2], a.m128_f32[3]);
}

mat4 mat4::inverted() const {

	mat4 m = this->transposed();
	__m128 minor0, minor1, minor2, minor3;
	// _mm_shuffle_pd(a, a, 1) can be used to swap the hi/lo qwords of a __m128d,
	// and _mm_shuffle_ps(a, a, 0x4E) for a __m128

	__m128	row0 = m.data[0], 
			row1 = _mm_shuffle_ps(m.data[1], m.data[1], mask1032),
			row2 = m.data[2], 
			row3 = _mm_shuffle_ps(m.data[3], m.data[3], mask1032);

	__m128 det, tmp1;

	const float *src = (const float*)this->rawData();

	// The following code fragment was copied (with minor adaptations of course)
	// from http://download.intel.com/design/PentiumIII/sml/24504301.pdf.
	// Other alternatives include https://github.com/LiraNuna/glsl-sse2/blob/master/source/mat4.h#L324,
	// but based on pure intrinsic call count, the intel version is more concise (84 vs. 102, many of which shuffles)

	tmp1 = _mm_mul_ps(row2, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_mul_ps(row1, tmp1);
	minor1 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
	minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
	minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);


	tmp1 = _mm_mul_ps(row1, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
	minor3 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
	minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);


	tmp1 = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	row2 = _mm_shuffle_ps(row2, row2, 0x4E);
	minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
	minor2 = _mm_mul_ps(row0, tmp1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
	minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);


	tmp1 = _mm_mul_ps(row0, row1);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));


	tmp1 = _mm_mul_ps(row0, row3);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
	minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
	minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));


	tmp1 = _mm_mul_ps(row0, row2);
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
	minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
	minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
	tmp1 = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
	minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
	minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);
	
	det = _mm_mul_ps(row0, minor0);
	det = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
	det = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
	tmp1 = _mm_rcp_ss(det);
	det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
	det = _mm_shuffle_ps(det, det, 0x00);
	
	minor0 = _mm_mul_ps(det, minor0);
	m.data[0] = minor0;
	
	minor1 = _mm_mul_ps(det, minor1);
	m.data[1] = minor1;
	
	minor2 = _mm_mul_ps(det, minor2);
	m.data[2] = minor2;
	
	minor3 = _mm_mul_ps(det, minor3);
	m.data[3] = minor3;

	return m;
}


inline float det(const mat4 &m) {


}

// i'm not quite sure why anybody would ever want to construct a quaternion like this :-XD
Quaternion::Quaternion(float x, float y, float z, float w) { 
	data = _mm_set_ps(w, z, y, x);	// in reverse order
}

Quaternion::Quaternion() { data=QUAT_NO_ROTATION; }

Quaternion Quaternion::conjugate() const {
	return Quaternion(_mm_mul_ps(this->data, QUAT_CONJUGATE));	
}

Quaternion Quaternion::fromAxisAngle(float x, float y, float z, float angle) {

	Quaternion q;
	const __m128 sin_angle = _mm_set1_ps(sin(angle));
	
	vec4 axis(x, y, z, 0);
	axis.normalize();

	q.data = _mm_mul_ps(sin_angle, axis.getData());
	q(Q::w) = cos(angle);
	
	return q;
}

void Quaternion::print() const { printf("[(%4.5f, %4.5f, %4.5f), %4.5f]\n\n", element(Q::x), element(Q::y), element(Q::z), element(Q::w)); }


void Quaternion::normalize() { 

	Quaternion &Q = (*this);
	//const float mag_squared = _mm_dp_ps(Q.data, Q.data, xyzw_dot_mask).m128_f32[0];
	const float mag_squared = MM_DPPS_XYZW(Q.data, Q.data);
	if (fabs(mag_squared-1.0) > 0.001) {	// to prevent calculations from exploding
		Q.data = _mm_mul_ps(Q.data, _mm_set1_ps(1.0/sqrt(mag_squared)));	
	}

}

Quaternion Quaternion::operator*(const Quaternion &b) const {
#ifdef _WIN32

	// q1*q2 = q3
	// q3 = v3 + w3,
	// v = w1w2 - dot(v1, v2), 
	// w = w1v2 + w2v1 + cross(v1, v2)

	const Quaternion &a = (*this);

	Quaternion ret(
	_mm_sub_ps(
	_mm_mul_ps(_mm_shuffle_ps(a.data, a.data, mask3021), _mm_shuffle_ps(b.data, b.data, mask3102)),
	_mm_mul_ps(_mm_shuffle_ps(a.data, a.data, mask3102), _mm_shuffle_ps(b.data, b.data, mask3021))));
	
	ret += a.element(Q::w)*b + b.element(Q::w)*a;

	//ret(Q::w) = a.data.m128_f32[Q::w]*b.data.m128_f32[Q::w] - _mm_dp_ps(a.data, b.data, xyz_dot_mask).m128_f32[0];
	ret(Q::w) = a.element(Q::w) * b.element(Q::w) - MM_DPPS_XYZ(a.data, b.data);
	return ret;

#elif __linux__
	//nyi
#endif 

}

void Quaternion::operator*=(const Quaternion &b) {
	(*this) = (*this) * b;
}

Quaternion Quaternion::operator*(float scalar) const {
	return Quaternion(_mm_mul_ps(_mm_set1_ps(scalar), data));
}

void Quaternion::operator*=(float scalar) {
	this->data = _mm_mul_ps(_mm_set1_ps(scalar), data);
}


Quaternion Quaternion::operator+(const Quaternion& b) const {
	return Quaternion(_mm_add_ps(this->data, b.data));
}

void Quaternion::operator+=(const Quaternion &b) {
	this->data = _mm_add_ps(this->data, b.data);
}

// a name such as "applyQuaternionRotation" would be much more fitting,
// since quaternion-vector multiplication doesn't REALLY exist

vec4 Quaternion::operator*(const vec4& b) const {

	vec4 v(b);
	v.normalize();
	Quaternion vec_q(b.getData()), res_q;
	vec_q(Q::w) = 0.0;

	res_q = vec_q * (*this).conjugate();
	res_q = (*this)*res_q;
	
	return vec4(res_q.data);	// the w component probably contains some garbage, but it's not used anyway
}

mat4 Quaternion::toRotationMatrix() const {
	
	// using SSE, the initial combinatorics could be done with
	// 2 multiplications, two shuffles, and one regular
	// multiplication. not sure it's quite worth it though :P

	// ASSUMING *this is a NORMALIZED QUATERNION!

	using namespace Q;	// to use x, y etc. instead of Q::x, Q::y

	const float x2 = element(x)*element(x);
	const float y2 = element(y)*element(y);
	const float z2 = element(z)*element(z);
	const float xy = element(x)*element(y);
	const float xz = element(x)*element(z);
	const float xw = element(x)*element(w);
	const float yz = element(y)*element(z);
	const float yw = element(y)*element(w);
	const float zw = element(z)*element(w);

	return mat4(vec4(1.0 - 2.0*(y2 + z2), 2.0*(xy-zw), 2.0*(xz + yw), 0.0f),
				vec4(2.0 * (xy + zw), 1.0 - 2.0*(x2 + z2), 2.0*(yz - xw), 0.0),
				vec4(2.0 * (xz - yw), 2.0 * (yz + xw), 1.0 - 2.0 * (x2 + y2), 0.0),
				vec4(0.0, 0.0, 0.0, 1.0));

}

Quaternion operator*(float scalar, const Quaternion& q) {
	return Quaternion(_mm_mul_ps(_mm_set1_ps(scalar), q.getData()));
}
