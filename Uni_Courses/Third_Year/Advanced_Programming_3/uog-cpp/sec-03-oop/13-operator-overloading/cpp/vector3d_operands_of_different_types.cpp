#include <iostream>

class Vector3D {
private:
    double x {0};
    double y {0};
    double z {0};

public:
    Vector3D() = default;
    Vector3D(double x, double y, double z) : x{x}, y{y}, z{z} {};

    friend Vector3D operator+(const Vector3D& a, const Vector3D& b) {
        return Vector3D {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    friend Vector3D operator+(const double scalar, const Vector3D& other) {
        return Vector3D {scalar + other.x, scalar + other.y, scalar + other.z};
    }

    // We can add this code to make the operator is commutative: a + b = b + a
    friend Vector3D operator+(Vector3D& vector, const double other) {
        return Vector3D{ other + vector};   
        // of course we need a copy constructor here. 
        // But luckily we have implicit copy constructor :D
    }

    void print() {
        std::cout << "Vector(" << x << ", " << y << ", " << z << ")";
    }
};

int main()
{
    Vector3D w {1, 2, 3};
    Vector3D v = 42 + w;
    v.print();
    std::cout << '\n';
}