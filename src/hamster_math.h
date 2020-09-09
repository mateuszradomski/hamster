#ifndef HAMSTER_MATH_H

#include <cstdint>

typedef char i8;
typedef short i16;
typedef int i32;
typedef long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

typedef float f32;
typedef double f64;

#include <float.h>
#include <math.h>

// Constants
#define PI 3.1415926535

// Intiger max values
#define U8MAX ((u8)-1)
#define U16MAX ((u16)-1)
#define U32MAX ((u32)-1)
#define U64MAX ((u64)-1)

// Floating point max and min values
#define F32MIN (FLT_MIN)
#define F32MAX (FLT_MAX)

#define F64MIN (DBL_MIN)
#define F64MAX (DBL_MAX)

#define MATRIX2_ELEMENTS (2*2)
#define MATRIX3_ELEMENTS (3*3)
#define MATRIX4_ELEMENTS (4*4)

#define HASHTABLE_PRIME1 163
#define HASHTABLE_PRIME2 151

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

struct RandomSeries
{
	u32 v;
};

union Vec2
{
	struct
	{
		f32 x;
		f32 y;
	};
	f32 m[2];
	Vec2(f32 x = 0.0f, f32 y = 0.0f) :
	x(x), y(y) { }
};

union Vec3
{
	struct
	{
		f32 x;
		f32 y;
		f32 z;
	};
	struct
	{
		f32 r;
		f32 g;
		f32 b;
	};
	f32 m[3];
	Vec3(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f) :
	x(x), y(y), z(z) { }
	
	bool operator==(const Vec3 &other) const
	{
		return (this->x == other.x) && (this->y == other.y) && (this->z == other.z);
	}
};

union Vec4
{
	struct
	{
		f32 x;
		f32 y;
		f32 z;
		f32 w;
	};
	struct
	{
		f32 r;
		f32 g;
		f32 b;
		f32 a;
	};
    f32 m[4];
	
	Vec4(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f, f32 w = 0.0f) :
	x(x), y(y), z(z), w(w) { }
    Vec4(Vec3 v, f32 w = 0.0f) :
	x(v.x), y(v.y), z(v.z), w(w) { }
};

struct Quat
{
    f32 w;
    Vec3 v;
    
    Quat(f32 w = 1.0f, Vec3 v = Vec3()) :
    w(w), v(v) { }
};

union Mat2 
{
	Vec2 columns[2];
	f32 a[2][2];
	f32 a1d[4];
	
	Mat2(f32 diagonal = 1.0f)
	{
		memset(a1d, 0, sizeof(a1d));
		
		a[0][0] = diagonal;
		a[1][1] = diagonal;
	}
};

union Mat4
{
	Vec4 columns[4];
	f32 a[4][4];
	f32 a1d[16];
	
	Mat4(f32 diagonal = 1.0f)
	{
		memset(a1d, 0, sizeof(a1d));
		
		a[0][0] = diagonal;
		a[1][1] = diagonal;
		a[2][2] = diagonal;
		a[3][3] = diagonal;
	}
};

struct Plane
{
    Vec3 normal;
    f32 d;
};

#define HAMSTER_MATH_H
#endif