#ifndef YET_ANOTHER_UTIL_H
#define YET_ANOTHER_UTIL_H
#include <functional>


#define prt(x) std::cerr << #x " = '" << x << "'" << std::endl;
#define prt2(x, y) std::cerr << #x " = '" << x << "'\t" << #y " = '" << y << "'" << std::endl;
#define prt3(x, y, z) std::cerr << #x " = '" << x << "'\t" << #y " = '" << y << "'\t" << #z " = '" << z << "'" << std::endl;
#define prt4(x, y, z, u) std::cerr << #x " = '" << x << "'\t" << #y " = '" << y << "'\t" << #z " = '" << z << "'\t" << #u " = '" << u << "'" << std::endl;
#define prt5(x, y, z, u, v) std::cerr << #x " = '" << x \
    << "'\t" << #y " = '" << y \
    << "'\t" << #z " = '" << z \
    << "'\t" << #u " = '" << u \
    << "'\t" << #v " = '" << v \
    << "'" << std::endl;
#define prt6(x, y, z, u, v, w) std::cerr << #x " = '" << x \
    << "'\t" << #y " = '" << y \
    << "'\t" << #z " = '" << z \
    << "'\t" << #u " = '" << u \
    << "'\t" << #v " = '" << v \
    << "'\t" << #w " = '" << w \
    << "'" << std::endl;
#define prt7(x, y, z, u, v, w, t) std::cerr << #x " = '" << x \
    << "'\t" << #y " = '" << y \
    << "'\t" << #z " = '" << z \
    << "'\t" << #u " = '" << u \
    << "'\t" << #v " = '" << v \
    << "'\t" << #w " = '" << w \
    << "'\t" << #t " = '" << t \
    << "'" << std::endl;

#define pt(x)  "\"" << #x << "\" = " << x << ";\t"

#define assertss(expr, value)\
   if (!(expr)){\
       std::cerr << "BOEM!" << std::endl << value << std::endl; \
        __assert_fail (__STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION); abort(); \
   }
#define assertssexec(expr, value, __e__)\
   if (!(expr)){\
       std::cerr << "BOEM!" << std::endl << value << std::endl; __e__; \
        __assert_fail (__STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION); abort(); \
   }

template<typename containertype>
void sortedPairContainerComparison(containertype &seta,
                                   containertype &setb,
                                   std::function<bool(const typename containertype::const_iterator &)> handleSetAOnly,
                                   std::function<bool(const typename containertype::const_iterator &)> handleSetBOnly,
                                   std::function<bool(const typename containertype::const_iterator &, const typename containertype::const_iterator &)> handleCommon) {

    auto ita = seta.begin();
    auto enda = seta.end();
    auto itb = setb.begin();
    auto endb = setb.end();

    while (ita != enda && itb != endb) {
        if (ita->first < itb->first) {
            if (!handleSetAOnly(ita)) {
                return;
            }
            ++ita;
        } else if (itb->first < ita->first) {
            if (!handleSetBOnly(itb)) {
                return;
            }
            ++itb;
        } else {
            if (!handleCommon(ita, itb)) {
                return;
            }
            ++ita;
            ++itb;
        }
    }
    while (ita != enda) {
        if (!handleSetAOnly(ita)) {
            return;
        }
        ++ita;
    }
    while (itb != endb) {
        if (!handleSetBOnly(itb)) {
            return;
        }
        ++itb;
    }
};






#endif
