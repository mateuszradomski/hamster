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

static u64 rdtsc();
static u32 find_first_set_bit(u32 a);
static u32 xorshift32(RandomSeries *series);
static u32 hash(u8 *bytes, u32 size, u32 prime, u32 range);
static u32 double_hash(u8 *bytes, u32 size, u32 attempt, u32 range);

static f32 fast_inverse_sqrtf(f32 a);
static f32 fast_sqrtf(f32 a);

static f32 random_unilateral(RandomSeries *series);
static f32 random_bilateral(RandomSeries *series);
static f32 lerp(f32 a, f32 t, f32 b);
static f32 to_radians(f32 a);
static u32 clamp(u32 a, u32 min, u32 max);
static f32 clamp(f32 a, f32 min, f32 max);
static f32 clamp01(f32 a);

static f32 len(Vec2 a);
static f32 inverse_len(Vec2 a);
static Vec2 noz(Vec2 a);
static Vec2 negate(Vec2 a);
static Vec2 abs(Vec2 a);
static Vec2 add(Vec2 a, Vec2 b);
static Vec2 adds(Vec2 a, f32 scalar);
static Vec2 sub(Vec2 a, Vec2 b);
static Vec2 subs(Vec2 a, f32 scalar);
static Vec2 div(Vec2 a, Vec2 b);
static Vec2 divs(Vec2 a, f32 scalar);
static Vec2 hadamard(Vec2 a, Vec2 b);
static Vec2 scale(Vec2 a, f32 scalar);
static Vec2 lerp(Vec2 a, f32 t, Vec2 b);
static Vec2 perp(Vec2 a);
static f32 inner(Vec2 a, Vec2 b);

static f32 len(Vec3 a);
static f32 inverse_len(Vec3 a);
static Vec3 noz(Vec3 a);
static Vec3 negate(Vec3 a);
static Vec3 abs(Vec3 a);
static Vec3 add(Vec3 a, Vec3 b);
static Vec3 adds(Vec3 a, f32 scalar);
static Vec3 sub(Vec3 a, Vec3 b);
static Vec3 subs(Vec3 a, f32 scalar);
static Vec3 div(Vec3 a, Vec3 b);
static Vec3 divs(Vec3 a, f32 scalar);
static Vec3 hadamard(Vec3 a, Vec3 b);
static Vec3 scale(Vec3 a, f32 scalar);
static Vec3 cross(Vec3 a, Vec3 b);
static Vec3 lerp(Vec3 a, f32 t, Vec3 b);
static f32 inner(Vec3 a, Vec3 b);

static f32 len(Vec4 a);
static f32 inverse_len(Vec4 a);
static Vec4 noz(Vec4 a);
static Vec4 negate(Vec4 a);
static Vec4 abs(Vec4 a);
static Vec4 add(Vec4 a, Vec4 b);
static Vec4 adds(Vec4 a, f32 scalar);
static Vec4 sub(Vec4 a, Vec4 b);
static Vec4 subs(Vec4 a, f32 scalar);
static Vec4 div(Vec4 a, Vec4 b);
static Vec4 divs(Vec4 a, f32 scalar);
static Vec4 hadamard(Vec4 a, Vec4 b);
static Vec4 scale(Vec4 a, f32 scalar);
static Vec4 lerp(Vec4 a, f32 t, Vec4 b);
static f32 inner(Vec4 a, Vec4 b);

static f32 len(Quat a);
static Quat noz(Quat a);
static Quat mul(Quat a, Quat b);
static Quat lerp(Quat a, f32 t, Quat b);
static Quat qrot(Quat a, f32 angle, Vec3 dir);
static Quat create_qrot(f32 angle, Vec3 dir);

static Mat2 add(Mat2 a, Mat2 b);
static Mat2 adds(Mat2 a, f32 scalar);
static Mat2 sub(Mat2 a, Mat2 b);
static Mat2 subs(Mat2 a, f32 scalar);
static Mat2 muls(Mat2 a, f32 scalar);
static Mat2 mul(Mat2 a, Mat2 b);

static Mat4 add(Mat4 a, Mat4 b);
static Mat4 adds(Mat4 a, f32 scalar);
static Mat4 sub(Mat4 a, Mat4 b);
static Mat4 subs(Mat4 a, f32 scalar);
static Mat4 muls(Mat4 a, f32 scalar);
static Mat4 mul(Mat4 a, Mat4 b);
static Vec3 mul(Mat4 a, Vec3 v);
static Vec4 mul(Mat4 a, Vec4 v);

static Mat4 translate(Mat4 a, Vec3 trans);
static Mat4 create_translate(Mat4 a, Vec3 trans);
static Mat4 scale(Mat4 a, Vec3 size);
static Mat4 create_scale(Vec3 size);
static Mat4 rot_around_vec(Mat4 a, f32 angle, Vec3 vec);
static Mat4 rotate_from_quat(Quat a);
static Mat4 rotate_quat(Mat4 a, Quat q);
static Mat4 create_perspective(f32 aspect_ratio, f32 fov, f32 near_z, f32 far_z);
static Mat4 create_ortographic(f32 aspect_ratio, f32 near_z, f32 far_z);
static Mat4 look_at(Vec3 cam_target, Vec3 cam_pos, Vec3 cam_up);

#define HAMSTER_MATH_H
#endif