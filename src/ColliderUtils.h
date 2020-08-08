//
// Created by Yilin Zhang on 8/8/20.
//

#ifndef COLLIDE_COLLIDERUTILS_H
#define COLLIDE_COLLIDERUTILS_H

template <typename T>
struct RCFilter {
    T yn1;
    T a; // the filter coefficient

    RCFilter(T tau) {
        setTau(tau);
    }

    void setTau(T tau) {
        this->a = tau / (tau + APP->engine->getSampleTime());
    }

    void setCutoff(T fc) {
        this->a = 1 - fc / APP->engine->getSampleRate();
    }

    /*! Return the next value
        @T xn the target value
     */
    T process(T xn) {
        T yn = this->a * yn1 + (1 - this->a) * xn;
        yn1 = yn;
        return yn;
    }

    void reset(T rstVal = 0.f) {
        yn1 = rstVal;
    }
};

template <typename T>
struct RCDiode: RCFilter<T> {
    RCDiode(T tau): RCFilter<T>(tau) {

    }

    T charge(T vi) {
        this->yn1 = vi;
        return vi;
    }
};

#endif //COLLIDE_COLLIDERUTILS_H
