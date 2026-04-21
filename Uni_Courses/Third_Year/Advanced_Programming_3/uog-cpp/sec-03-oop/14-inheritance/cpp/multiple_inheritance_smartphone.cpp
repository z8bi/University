#include <iostream>

class Camera {
public:
    void take_photo() {
        std::cout << "Taking a photo!" << std::endl;
    }
};

class Phone {
public:
    void make_call() {
        std::cout << "Making a phone call!" << std::endl;
    }
};

// Multiple Inheritance
class SmartPhone : public Camera, public Phone {
    // Inherits members from both Camera and Phone

    // add more functionality to phone
    void use_app(std::string app_name) {
        std::cout << app_name << "is being used.\n"; 
    }
};

int main() {
    SmartPhone my_phone;
    
    my_phone.take_photo();
    my_phone.make_call();
    
    return 0;
}