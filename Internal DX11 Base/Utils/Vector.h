#pragma once
#include <cmath>

namespace Utils {
    struct Vector2 {
        float x, y;

        Vector2() : x(0), y(0) {}
        Vector2(float x, float y) : x(x), y(y) {}

        // Operators
        Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
        Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
        Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
        Vector2 operator/(float scalar) const { 
            if (scalar == 0.0f) return Vector2(0, 0);
            return Vector2(x / scalar, y / scalar); 
        }
        Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
        Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
        Vector2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
        Vector2& operator/=(float scalar) { 
            if (scalar == 0.0f) { x = 0; y = 0; } 
            else { x /= scalar; y /= scalar; } 
            return *this; 
        }

        // Mathematical functions
        float Length() const { return sqrt(x * x + y * y); }
        float LengthSquared() const { return x * x + y * y; }
        float Distance(const Vector2& v) const { return (*this - v).Length(); }
        float DistanceSquared(const Vector2& v) const { return (*this - v).LengthSquared(); }
        void Normalize() { 
            float len = Length(); 
            if (len > 0.0001f) { 
                x /= len; 
                y /= len; 
            } else {
                x = 0; 
                y = 0; 
            }
        }
        Vector2 Normalized() const { Vector2 result = *this; result.Normalize(); return result; }
        float Dot(const Vector2& v) const { return x * v.x + y * v.y; }
    };

    struct Vector3 {
        float x, y, z;

        Vector3() : x(0), y(0), z(0) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

        // Operators
        Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
        Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
        Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
        Vector3 operator/(float scalar) const { 
            if (scalar == 0.0f) return Vector3(0, 0, 0);
            return Vector3(x / scalar, y / scalar, z / scalar); 
        }
        Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
        Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
        Vector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
        Vector3& operator/=(float scalar) { 
            if (scalar == 0.0f) { x = 0; y = 0; z = 0; } 
            else { x /= scalar; y /= scalar; z /= scalar; } 
            return *this; 
        }

        // Mathematical functions
        float Length() const { return sqrt(x * x + y * y + z * z); }
        float LengthSquared() const { return x * x + y * y + z * z; }
        float Distance(const Vector3& v) const { return (*this - v).Length(); }
        float DistanceSquared(const Vector3& v) const { return (*this - v).LengthSquared(); }
        void Normalize() { 
            float len = Length(); 
            if (len > 0.0001f) { 
                x /= len; 
                y /= len; 
                z /= len; 
            } else {
                x = 0; 
                y = 0; 
                z = 0; 
            }
        }
        Vector3 Normalized() const { Vector3 result = *this; result.Normalize(); return result; }
        float Dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
        Vector3 Cross(const Vector3& v) const { 
            return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); 
        }
    };

    struct Vector4 {
        float x, y, z, w;

        Vector4() : x(0), y(0), z(0), w(0) {}
        Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        Vector4(const Vector3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

        // Operators
        Vector4 operator+(const Vector4& v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
        Vector4 operator-(const Vector4& v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
        Vector4 operator*(float scalar) const { return Vector4(x * scalar, y * scalar, z * scalar, w * scalar); }
        Vector4 operator/(float scalar) const { 
            if (scalar == 0.0f) return Vector4(0, 0, 0, 0);
            return Vector4(x / scalar, y / scalar, z / scalar, w / scalar); 
        }
        Vector4& operator+=(const Vector4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
        Vector4& operator-=(const Vector4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
        Vector4& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
        Vector4& operator/=(float scalar) { 
            if (scalar == 0.0f) { x = 0; y = 0; z = 0; w = 0; } 
            else { x /= scalar; y /= scalar; z /= scalar; w /= scalar; } 
            return *this; 
        }

        // Mathematical functions
        float Length() const { return sqrt(x * x + y * y + z * z + w * w); }
        float LengthSquared() const { return x * x + y * y + z * z + w * w; }
        float Distance(const Vector4& v) const { return (*this - v).Length(); }
        float DistanceSquared(const Vector4& v) const { return (*this - v).LengthSquared(); }
        void Normalize() { 
            float len = Length(); 
            if (len > 0.0001f) { 
                x /= len; 
                y /= len; 
                z /= len; 
                w /= len; 
            } else {
                x = 0; 
                y = 0; 
                z = 0; 
                w = 0; 
            }
        }
        Vector4 Normalized() const { Vector4 result = *this; result.Normalize(); return result; }
        float Dot(const Vector4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }
    };
}
