#ifndef NULLPTR_H
#define NULLPTR_H

const
class nullptr_t_t {
public:

    template<class T> operator T*() const {
        return 0;
    }

    template<class C, class T> operator T C::*() const {
        return 0;
    }
private:
    void operator&() const;
} nullptr_t = {};

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr_t

#ifdef nullptr
#undef nullptr
#endif
#define nullptr nullptr_t





#endif // NULLPTR_H
