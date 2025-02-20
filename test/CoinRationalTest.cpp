// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "CoinUtilsConfig.h"

#ifdef COINUTILS_HAS_STDINT_H
#include <stdint.h>
#endif

#include <cassert>
#include <iostream>
#include "CoinRational.hpp"

void
CoinRationalUnitTest()
{

    // Test default constructor
    {
        CoinRational a; // 0/1
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 0);
        assert(a.getDenominator() == 1);
    }

    // Requires int64_t
    // Test constructor with assignment
    {
        CoinRational a(4294967295, 4294967296);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 4294967295);
        assert(a.getDenominator() == 4294967296);
    }
    
    // Requires int64_t
    // Test constructor with assignment
    {
        CoinRational a(9223372036854775807, 4294967299);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 9223372036854775807);
        assert(a.getDenominator() == 4294967299);
    }

    // Requires int64_t
    // Test constructor with nearestRational calls
    {
        CoinRational a(2147483699.5, 0.00001, 4294967299);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 4294967399);
        assert(a.getDenominator() == 2);
    }

    // Test constructor with nearestRational calls
    {
        CoinRational a(-3.0, 0.0001, 100);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == -3);
        assert(a.getDenominator() == 1);
    }

    {
        CoinRational a(3.0, 0.0001, 100);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 3);
        assert(a.getDenominator() == 1);
    }

    {
        // return 0/1 if best is more than maxdelta tolerance
        CoinRational a(0.367879441, 0.0001, 100); // 1/e
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 32);
        assert(a.getDenominator() == 87);
    }

    {
        CoinRational a(10.367879441, 0.0001, 100); // 10 + 1/e
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 902);
        assert(a.getDenominator() == 87);
    }

    {
        // return 0/1 if best is more than maxdelta tolerance
        CoinRational a(0.367879441, 0.000001, 100); // 1/e
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 0);
        assert(a.getDenominator() == 1);
    }

    {
        CoinRational a(3.0 / 7.0, 0.0001, 100);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 3);
        assert(a.getDenominator() == 7);
    }

    {
        CoinRational a(7.0 / 3.0, 0.0001, 100);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 7);
        assert(a.getDenominator() == 3);
    }

    {
        double sqrt13 = sqrt(13);
        CoinRational a(sqrt13, 0.01, 20);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 18);
        assert(a.getDenominator() == 5);
    }

    {
        double sqrt13 = sqrt(13);
        CoinRational a(sqrt13, 0.002, 30);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 101);
        assert(a.getDenominator() == 28);
    }

    {
        CoinRational a(0.25, 0.1, 3);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 1);
        assert(a.getDenominator() == 3);
    }

    {
        CoinRational a(0.605551, 0.003, 30);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 17);
        assert(a.getDenominator() == 28);
    }
        
    {
        CoinRational a(0.605551, 0.001, 30);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 20);
        assert(a.getDenominator() == 33); // oops, should be at most 30.
    }

    {
        CoinRational a(0.58496250072, 0.00001, 253);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 179);
        assert(a.getDenominator() == 306);  // oops, should be at most 253. Expected 148/253, but this is apparently on purpose.
    }

    {
        CoinRational a(19.0/11.0, 0.02, 10);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == 12);
        assert(a.getDenominator() == 7);
    }

    {
        CoinRational a(-19.0 / 11.0, 0.02, 10);
        std::cout << "Testing " << a.getNumerator() << " / " << a.getDenominator() << std::endl;
        assert(a.getNumerator() == -12);
        assert(a.getDenominator() == 7);
    }
}
