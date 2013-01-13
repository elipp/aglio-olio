#ifndef Vec3_H
#define Vec3_H

#include <cmath>

typedef struct _coordinate3 {

	float x, y, z;

} coordinate3;

typedef struct _coordinate4 {
	
	float x, y, z, w;

} coordinate4;

class Vec3 {

private:
	
	float vlength;

public:
	coordinate3 direction;
        

    inline float length() { return sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z); }

	Vec3(float x, float y, float z)
	{
		direction.x = x;
		direction.y = y;
		direction.z = z;

		vlength = length(); 
	}


	
        inline Vec3& operator+=(Vec3 const &a);
        inline Vec3& operator+=(coordinate3 const &a);
        
		inline void updatelength() { vlength = length(); }
        inline void normalize() { if(vlength) { direction.x /= vlength; direction.y /= vlength; direction.z /= vlength; }}  // calculate unit vect$
		static inline float dot(Vec3 const &a, Vec3 const &b) { return a.direction.x * b.direction.x + a.direction.y * b.direction.y + a.direction.z * b.direction.z; }
		static inline Vec3 cross(Vec3 const &a, Vec3 const &b) { Vec3 result(a.direction.y*b.direction.z - a.direction.z*b.direction.y, 
																			  a.direction.z*b.direction.x - a.direction.x*b.direction.z,
																			  a.direction.x*b.direction.y - a.direction.y*b.direction.x);
																  return result; }

};



class Vec4 {

private:
	
	float vlength;

public:
	coordinate4 direction;

    inline float length() { return sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z + direction.w * direction.w); }
	Vec4(float x, float y, float z, float w)
	{
		direction.x = x;
		direction.y = y;
		direction.z = z;
		direction.w = w;

		vlength = length(); 
	}

	
    inline void normalize() { if(vlength) { direction.x /= vlength; direction.y /= vlength; direction.z /= vlength; direction.w /= vlength; }}  // calculate unit vect$
	
	inline void updatelength() { vlength = length(); }
	static inline float dot(Vec4 const &a, Vec4 const &b) { return a.direction.x * b.direction.x + a.direction.y * b.direction.y + a.direction.z * b.direction.z + a.direction.w * b.direction.w; }

};


#endif
