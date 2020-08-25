#include "hamster_math.h"

static u32
find_first_set_bit(u32 a) // least significant bit / lsb
{
	u32 result = 0;
	
#if GCC_COMPILE
	result = __builtin_ctz(a);
#else
	for(u32 i = 0; i < sizeof(u32) * 8; ++i)
	{
		if((a & (1 << i)))
		{
			result = i;
			
			break;
		}
	}
#endif
	
	return result;
}

static u64
rdtsc()
{
	u64 result = 0;
	
#if GCC_COMPILE
	result = __rdtsc();
#endif
	
	return result;
}

static u32
hash(u8 *bytes, u32 size, u32 prime, u32 range)
{
	u32 hash = 0;
	
	for(u32 i = 0; i < size; i++)
	{
		hash += powl(prime, size - (i + 1)) * bytes[i];
		hash = hash % range;
	}
	
	return hash;
}

static u32
double_hash(u8 *bytes, u32 size, u32 attempt, u32 range)
{
	u32 hash1 = hash(bytes, size, HASHTABLE_PRIME1, range);
	u32 hash2 = hash(bytes, size, HASHTABLE_PRIME2, range);
	
	return hash1 + ((hash2 + 1) * attempt) % range;
}

//Carmack's fast inverse square root
inline static f32
fast_inverse_sqrtf(f32 a)
{
	f32 result = 0.0f;
	i32 i = 0;
	f32 x = 0;
	f32 y = 0;
	f32 three_halfs = 1.5f;
	
	x = a * 0.5f;
	y = a;
	i = *(i32 *)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(f32 *)&i;
	y = y * (three_halfs - (x * y * y));
	y = y * (three_halfs - (x * y * y));
	
	result = y;
	
	return result;
}

//Carmack's fast inverse square root back to square root
inline static f32
fast_sqrtf(f32 a)
{
	f32 result = 0.0f;
	
	result = a * fast_inverse_sqrtf(a);
	
	return result;
}

static u32
xorshift32(RandomSeries *series)
{
	u32 result = 0;
	
	u32 x = series->v;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	
	series->v = result = x;
	
	return result;
}

inline static f32
random_unilateral(RandomSeries *series)
{
	f32 result = 0.0f;
	
	result = (f32)xorshift32(series) / U32MAX;
	
	return result;
}

inline static f32 
random_bilateral(RandomSeries *series)
{
	f32 result = 0.0f;
	
	result = random_unilateral(series) * 2.0f - 1.0f;
	
	return result;
}

inline static f32
lerp(f32 a, f32 t, f32 b)
{
	f32 result = 0.0f;
	
	result = (1.0f - t) * a + t * b;
	
	return result;
}

inline static f32 
to_radians(f32 a)
{
	f32 result = 0.0f;
	
	result = (a / 180.0f) * PI;
	
	return result;
}

inline static u32
clamp(u32 a, u32 min, u32 max)
{
	u32 result = a;
	
	result = MAX(min, result);
	result = MIN(max, result);
	
	return result;
}

inline static f32
clamp(f32 a, f32 min, f32 max)
{
	f32 result = a;
	
	result = MAX(min, result);
	result = MIN(max, result);
	
	return result;
}

inline static f32 
clamp01(f32 a)
{
	f32 result = a;
	
	result =	MAX(0.0f, result);
	result =	MIN(1.0f, result);
	
	return result;
}

inline static f32
len(Vec2 a)
{
	f32 result = 0.0f;
	
	result = a.x * a.x + a.y * a.y;
	
	result = sqrtf(result);
	
	return result;
}

inline static f32
inverse_len(Vec2 a)
{
	f32 result = 0.0f;
	
	result = a.x * a.x + a.y * a.y;
	
	result = fast_inverse_sqrtf(result);
	
	return result;
}

inline static Vec2
noz(Vec2 a)
{
	Vec2 result;
	f32 vec_len = 0.0f;
	
	vec_len = inverse_len(a);
	
	result = scale(a, vec_len);
	
	return result;
}

inline static Vec2 
negate(Vec2 a)
{
	Vec2 result;
	
	result.x = -a.x;
	result.y = -a.y;
	
	return result;
}

inline static Vec2
abs(Vec2 a)
{
	Vec2 result = {};
	
	result.x = fabsf(a.x);
	result.y = fabsf(a.y);
	
	return result;
}

inline static Vec2
add(Vec2 a, Vec2 b)
{
	Vec2 result;
	
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	
	return result;
}

inline static Vec2
adds(Vec2 a, f32 scalar)
{
	Vec2 result;
	
	result.x = a.x + scalar;
	result.y = a.y + scalar;
	
	return result;
}

inline static Vec2
sub(Vec2 a, Vec2 b)
{
	Vec2 result;
	
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	
	return result;
}

inline static Vec2
subs(Vec2 a, f32 scalar)
{
	Vec2 result = { 0 };
	
	result.x = a.x - scalar;
	result.y = a.y - scalar;
	
	return result;
}

inline static Vec2
div(Vec2 a, Vec2 b)
{
	Vec2 result = { 0 };
	
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	
	return result;
}

inline static Vec2
divs(Vec2 a, f32 scalar)
{
	Vec2 result = { 0 };
	
	result.x = a.x / scalar;
	result.y = a.y / scalar;
	
	return result;
}

inline static Vec2
hadamard(Vec2 a, Vec2 b)
{
	Vec2 result = { 0 };
	
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	
	return result;
}

inline static Vec2
scale(Vec2 a, f32 scalar)
{
	Vec2 result = { 0 };
	
	result.x = a.x * scalar;
	result.y = a.y * scalar;
	
	return result;
}

inline static Vec2 
lerp(Vec2 a, f32 t, Vec2 b)
{
	Vec2 result = { 0 };
	
	result.x = (1.0f - t) * a.x + t * b.x;
	result.y = (1.0f - t) * a.y + t * b.y;
	
	return result;
}

inline static Vec2 
perp(Vec2 a)
{
	Vec2 result = { .x = -a.y, .y = a.x };
	
	return result;
}

inline static f32
inner(Vec2 a, Vec2 b)
{
	f32 result = 0.0f;
	
	result = a.x * b.x + a.y * b.y;
	
	return result;
}

inline static f32 
len(Vec3 a)
{
	f32 result = 0.0f;
	
	result = a.x * a.x + a.y * a.y + a.z * a.z;
	
	result = sqrtf(result);
	
	return result;
}

inline static f32
inverse_len(Vec3 a)
{
	f32 result = 0.0f;
	
	result = a.x * a.x + a.y * a.y + a.z * a.z;
	
	result = fast_inverse_sqrtf(result);
	
	return result;
}

inline static Vec3 
noz(Vec3 a)
{
	Vec3 result = { 0 };
	f32 vec_len = 0.0f;
	
	vec_len = inverse_len(a);
	
	result = scale(a, vec_len);
	
	return result;
}

inline static Vec3 
negate(Vec3 a)
{
	Vec3 result = { 0 };
	
	result.x = -a.x;
	result.y = -a.y;
	result.z = -a.z;
	
	return result;
}

inline static Vec3
abs(Vec3 a)
{
	Vec3 result = {};
	
	result.x = fabsf(a.x);
	result.y = fabsf(a.y);
	result.z = fabsf(a.z);
	
	return result;
}

inline static Vec3 
add(Vec3 a, Vec3 b)
{
	Vec3 result = { 0 };
	
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	
	return result;
}

inline static Vec3 
adds(Vec3 a, f32 scalar)
{
	Vec3 result = { 0 };
	
	result.x = a.x + scalar;
	result.y = a.y + scalar;
	result.z = a.z + scalar;
	
	return result;
}

inline static Vec3 
sub(Vec3 a, Vec3 b)
{
	Vec3 result = { 0 };
	
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	
	return result;
}

inline static Vec3 
subs(Vec3 a, f32 scalar)
{
	Vec3 result = { 0 };
	
	result.x = a.x - scalar;
	result.y = a.y - scalar;
	result.z = a.z - scalar;
	
	return result;
}

inline static Vec3 
div(Vec3 a, Vec3 b)
{
	Vec3 result = { 0 };
	
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	result.z = a.z / b.z;
	
	return result;
}

inline static Vec3 
divs(Vec3 a, f32 scalar)
{
	Vec3 result = { 0 };
	
	result.x = a.x / scalar;
	result.y = a.y / scalar;
	result.z = a.z / scalar;
	
	return result;
}

inline static Vec3 
hadamard(Vec3 a, Vec3 b)
{
	Vec3 result = { 0 };
	
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	
	return result;
}

inline static Vec3 
scale(Vec3 a, f32 scalar)
{
	Vec3 result = { 0 };
	
	result.x = a.x * scalar;
	result.y = a.y * scalar;
	result.z = a.z * scalar;
	
	return result;
}

inline static Vec3
cross(Vec3 a, Vec3 b)
{
	Vec3 result = { 0 };
	
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
	
	return result;
}

inline static Vec3
lerp(Vec3 a, f32 t, Vec3 b)
{
	Vec3 result = { 0 };
	
	result.x = (1.0f - t) * a.x + t * b.x;
	result.y = (1.0f - t) * a.y + t * b.y;
	result.z = (1.0f - t) * a.z + t * b.z;
	
	return result;
}

inline static f32 
inner(Vec3 a, Vec3 b)
{
	f32 result = 0;
	
	result = a.x * b.x + a.y * b.y + a.z * b.z;
	
	return result;
}

inline static f32 
len(Vec4 a)
{
	f32 result = 0.0f;
	
	result = a.x * a.x + a.y * a.y + a.z * a.z * a.w * a.w;
	
	result = sqrtf(result);
	
	return result;
}

inline static f32
inverse_len(Vec4 a)
{
	f32 result = 0.0f;
	
	result = a.x * a.x + a.y * a.y + a.z * a.z * a.w * a.w;
	
	result = fast_inverse_sqrtf(result);
	
	return result;
}

inline static Vec4
noz(Vec4 a)
{
	Vec4 result = { 0 };
	f32 vec_len = 0.0f;
	
	vec_len = inverse_len(a);
	
	result = scale(a, vec_len);
	
	return result;
}

inline static Vec4
negate(Vec4 a)
{
	Vec4 result = { 0 };
	
	result.x = -a.x;
	result.y = -a.y;
	result.z = -a.z;
	result.w = -a.w;
	
	return result;
}

inline static Vec4
abs(Vec4 a)
{
	Vec4 result = {};
	
	result.x = fabsf(a.x);
	result.y = fabsf(a.y);
	result.z = fabsf(a.z);
	result.w = fabsf(a.w);
	
	return result;
}

inline static Vec4 
add(Vec4 a, Vec4 b)
{
	Vec4 result = { 0 };
	
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	result.w = a.w + b.w;
	
	return result;
}

inline static Vec4 
adds(Vec4 a, f32 scalar)
{
	Vec4 result = { 0 };
	
	result.x = a.x + scalar;
	result.y = a.y + scalar;
	result.z = a.z + scalar;
	result.w = a.w + scalar;
	
	return result;
}

inline static Vec4
sub(Vec4 a, Vec4 b)
{
	Vec4 result = { 0 };
	
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	result.z = a.w - b.w;
	
	return result;
}

inline static Vec4
subs(Vec4 a, f32 scalar)
{
	Vec4 result = { 0 };
	
	result.x = a.x - scalar;
	result.y = a.y - scalar;
	result.z = a.z - scalar;
	result.w = a.w - scalar;
	
	return result;
}

inline static Vec4
div(Vec4 a, Vec4 b)
{
	Vec4 result = { 0 };
	
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	result.z = a.z / b.z;
	result.w = a.w / b.w;
	
	return result;
}

inline static Vec4
divs(Vec4 a, f32 scalar)
{
	Vec4 result = { 0 };
	
	result.x = a.x / scalar;
	result.y = a.y / scalar;
	result.z = a.z / scalar;
	result.w = a.w / scalar;
	
	return result;
}

inline static Vec4
hadamard(Vec4 a, Vec4 b)
{
	Vec4 result = { 0 };
	
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	result.z = a.z * b.z;
	result.w = a.w * b.w;
	
	return result;
}

inline static Vec4
scale(Vec4 a, f32 scalar)
{
	Vec4 result = { 0 };
	
	result.x = a.x * scalar;
	result.y = a.y * scalar;
	result.z = a.z * scalar;
	result.w = a.w * scalar;
	
	return result;
}

inline static Vec4
cross(Vec4 a, Vec4 b)
{
	Vec4 result = { 0 };
	
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
	
	return result;
}

inline static Vec4
lerp(Vec4 a, f32 t, Vec4 b)
{
	Vec4 result = { 0 };
	
	result.x = (1.0f - t) * a.x + t * b.x;
	result.y = (1.0f - t) * a.y + t * b.y;
	result.z = (1.0f - t) * a.z + t * b.z;
	result.w = (1.0f - t) * a.w + t * b.w;
	
	return result;
}

inline static f32 
inner(Vec4 a, Vec4 b)
{
	f32 result = 0;
	
	result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	
	return result;
}

inline static Mat2 
add(Mat2 a, Mat2 b)
{
	Mat2 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX2_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] + b.a1d[index];
	}
	
	return result;
}

inline static Mat2 
adds(Mat2 a, f32 scalar)
{
	Mat2 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX2_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] + scalar;
	}
	
	return result;
}

inline static Mat2 
sub(Mat2 a, Mat2 b)
{
	Mat2 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX2_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] - b.a1d[index];
	}
	
	return result;
}

inline static Mat2 
subs(Mat2 a, f32 scalar)
{
	Mat2 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX2_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] - scalar;
	}
	
	return result;
}

inline static Mat2 
muls(Mat2 a, f32 scalar)
{
	Mat2 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX2_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] * scalar;
	}
	
	return result;
}

inline static Mat2 
mul(Mat2 a, Mat2 b)
{
	Mat2 result = { 0 };
	
	result.a[0][0] = a.a[0][0] * b.a[0][0] + a.a[1][0] * b.a[0][1];
	result.a[0][1] = a.a[0][1] * b.a[0][0] + a.a[1][1] * b.a[0][1];
	result.a[1][0] = a.a[0][0] * b.a[0][1] + a.a[1][0] * b.a[1][1];
	result.a[1][1] = a.a[0][1] * b.a[1][0] + a.a[1][1] * b.a[1][1];
	
	return result;
}

inline static Mat4
add(Mat4 a, Mat4 b)
{
	Mat4 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX4_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] + b.a1d[index];
	}
	
	return result;
}

inline static Mat4
adds(Mat4 a, f32 scalar)
{
	Mat4 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX4_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] + scalar;
	}
	
	return result;
}

inline static Mat4
sub(Mat4 a, Mat4 b)
{
	Mat4 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX4_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] + b.a1d[index];
	}
	
	return result;
}

inline static Mat4
subs(Mat4 a, f32 scalar)
{
	Mat4 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX4_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] + scalar;
	}
	
	return result;
}

inline static Mat4
muls(Mat4 a, f32 scalar)
{
	Mat4 result = { 0 };
	
	for(u32 index = 0;
		index < MATRIX4_ELEMENTS;
		++index)
	{
		result.a1d[index] = a.a1d[index] * scalar;
	}
	
	return result;
}

inline static Mat4
mul(Mat4 a, Mat4 b)
{
	Mat4 result = { 0 };
	
	result.a[0][0] = a.a[0][0] * b.a[0][0] + a.a[1][0] * b.a[0][1] + a.a[2][0] * b.a[0][2] + a.a[3][0] * b.a[0][3];
	result.a[1][0] = a.a[0][0] * b.a[1][0] + a.a[1][0] * b.a[1][1] + a.a[2][0] * b.a[1][2] + a.a[3][0] * b.a[1][3];
	result.a[2][0] = a.a[0][0] * b.a[2][0] + a.a[1][0] * b.a[2][1] + a.a[2][0] * b.a[2][2] + a.a[3][0] * b.a[2][3];
	result.a[3][0] = a.a[0][0] * b.a[3][0] + a.a[1][0] * b.a[3][1] + a.a[2][0] * b.a[3][2] + a.a[3][0] * b.a[3][3];
	
	result.a[0][1] = a.a[0][1] * b.a[0][0] + a.a[1][1] * b.a[0][1] + a.a[2][1] * b.a[0][2] + a.a[3][1] * b.a[0][3];
	result.a[1][1] = a.a[0][1] * b.a[1][0] + a.a[1][1] * b.a[1][1] + a.a[2][1] * b.a[1][2] + a.a[3][1] * b.a[1][3];
	result.a[2][1] = a.a[0][1] * b.a[2][0] + a.a[1][1] * b.a[2][1] + a.a[2][1] * b.a[2][2] + a.a[3][1] * b.a[2][3];
	result.a[3][1] = a.a[0][1] * b.a[3][0] + a.a[1][1] * b.a[3][1] + a.a[2][1] * b.a[3][2] + a.a[3][1] * b.a[3][3];
	
	result.a[0][2] = a.a[0][2] * b.a[0][0] + a.a[1][2] * b.a[0][1] + a.a[2][2] * b.a[0][2] + a.a[3][2] * b.a[0][3];
	result.a[1][2] = a.a[0][2] * b.a[1][0] + a.a[1][2] * b.a[1][1] + a.a[2][2] * b.a[1][2] + a.a[3][2] * b.a[1][3];
	result.a[2][2] = a.a[0][2] * b.a[2][0] + a.a[1][2] * b.a[2][1] + a.a[2][2] * b.a[2][2] + a.a[3][2] * b.a[2][3];
	result.a[3][2] = a.a[0][2] * b.a[3][0] + a.a[1][2] * b.a[3][1] + a.a[2][2] * b.a[3][2] + a.a[3][2] * b.a[3][3];
	
	result.a[0][3] = a.a[0][3] * b.a[0][0] + a.a[1][3] * b.a[0][1] + a.a[2][3] * b.a[0][2] + a.a[3][3] * b.a[0][3];
	result.a[1][3] = a.a[0][3] * b.a[1][0] + a.a[1][3] * b.a[1][1] + a.a[2][3] * b.a[1][2] + a.a[3][3] * b.a[1][3];
	result.a[2][3] = a.a[0][3] * b.a[2][0] + a.a[1][3] * b.a[2][1] + a.a[2][3] * b.a[2][2] + a.a[3][3] * b.a[2][3];
	result.a[3][3] = a.a[0][3] * b.a[3][0] + a.a[1][3] * b.a[3][1] + a.a[2][3] * b.a[3][2] + a.a[3][3] * b.a[3][3];
	
	return result;
}

static Mat4
create_perspective(f32 aspect_ratio, f32 fov, f32 near_z, f32 far_z)
{
	Mat4 result = { 0 };
	f32 tan_half_fov = 0.0f;
	
	tan_half_fov = tanf(to_radians(fov) * 0.5f);
	
	result.a[0][0] = 1.0f / (aspect_ratio * tan_half_fov);
	/* result.a[0][0] = aspect_ratio / (tan_half_fov); */
	result.a[1][1] = 1.0f / tan_half_fov;
	result.a[2][2] = (near_z + far_z) / (near_z - far_z);
	result.a[2][3] = -1.0f;
	result.a[3][2] = (2.0f * near_z * far_z) / (near_z - far_z);
	
	return result;
}

static Mat4
look_at(Vec3 cam_target, Vec3 cam_pos, Vec3 cam_up)
{
	Mat4 result = { 0 };
	Vec3 look_at = { 0 };
	Vec3 right = { 0 };
	Vec3 up = { 0 };
	
	look_at = noz(sub(cam_target, cam_pos));
	right = noz(cross(look_at, cam_up));
	up = noz(cross(right, look_at));
	
	result.a[0][0] = right.x;
	result.a[1][0] = right.y;
	result.a[2][0] = right.z;
	result.a[3][0] = -inner(right, cam_pos);
	result.a[0][1] = up.x;
	result.a[1][1] = up.y;
	result.a[2][1] = up.z;
	result.a[3][1] = -inner(up, cam_pos);
	result.a[0][2] = -look_at.x;
	result.a[1][2] = -look_at.y;
	result.a[2][2] = -look_at.z;
	result.a[3][2] = inner(look_at, cam_pos);
	result.a[0][3] = result.a[1][3] = result.a[2][3] = 0;
	result.a[3][3] = 1.0f;
	
	return result;
}

inline static Mat4
translate(Mat4 a, Vec3 trans)
{
	Mat4 result = { 0 };
	
	result = create_translate(a, trans);
	result = mul(result, a);
	
	return result;
}

static Mat4 
create_translate(Mat4 a, Vec3 trans)
{
	Mat4 result = { 0 };
	Vec4 colum0 = { 0 };
	Vec4 colum1 = { 0 };
	Vec4 colum2 = { 0 };
	
	result = a;
	
	colum0 = scale(a.columns[0], trans.x);
	colum1 = scale(a.columns[1], trans.y);
	colum2 = scale(a.columns[2], trans.z);
	
	result.columns[3] = add(colum0, result.columns[3]);
	result.columns[3] = add(colum1, result.columns[3]);
	result.columns[3] = add(colum2, result.columns[3]);
	
	return result;
}

inline static Mat4 
scale(Mat4 a, Vec3 size)
{
	Mat4 result = { 0 };
	
	result.columns[0] = scale(a.columns[0], size.x);
	result.columns[1] = scale(a.columns[1], size.y);
	result.columns[2] = scale(a.columns[2], size.z);
	result.columns[3] = a.columns[3];
	
	return result;
}

static Mat4 
create_scale(Vec3 size)
{
	Mat4 result = { 0 };
	
	result.a[0][0] = size.x;
	result.a[1][1] = size.y;
	result.a[2][2] = size.z;
	result.a[3][3] = 1.0f;
	
	return result;
}

static Mat4
rot_around_vec(Mat4 a, f32 angle, Vec3 vec)
{
	Mat4 rotation_matrix = { 0 };
	
	vec = noz(vec);
	
	f32 cos_theta = cosf(angle);
	f32 sin_theta = sinf(angle);
	f32 one_minus_cos_theta = 1.0f - cos_theta;
	
	rotation_matrix.a[0][0] = cos_theta + one_minus_cos_theta * vec.x * vec.x;
	rotation_matrix.a[0][1] = one_minus_cos_theta * vec.x * vec.y + sin_theta * vec.z;
	rotation_matrix.a[0][2] = one_minus_cos_theta * vec.x * vec.z - sin_theta * vec.y;
	
	rotation_matrix.a[1][0] = one_minus_cos_theta * vec.x * vec.y - sin_theta * vec.z;
	rotation_matrix.a[1][1] = cos_theta + one_minus_cos_theta * vec.y * vec.y;
	rotation_matrix.a[1][2] = one_minus_cos_theta * vec.y * vec.z + sin_theta * vec.x;
	
	rotation_matrix.a[2][0] = one_minus_cos_theta * vec.z * vec.x + sin_theta * vec.y;
	rotation_matrix.a[2][1] = one_minus_cos_theta * vec.z * vec.y - sin_theta * vec.x;
	rotation_matrix.a[2][2] = cos_theta + one_minus_cos_theta * vec.z * vec.z;
	
	rotation_matrix.a[3][3] = 1.0f;
	
	return mul(rotation_matrix, a);
}
