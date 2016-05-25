/*
  Copyright 2015 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdexcept>
#include <fstream>

#include <ert/util/test_util.h>

#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/FortIO.hpp>

void test_kw_vector_assign() {
    std::vector< int > vec = { 1, 2, 3, 4, 5 };
    ERT::EclKW< int > kw( "XYZ", vec );

    test_assert_size_t_equal( kw.size(), vec.size() );

    for( size_t i = 0; i < kw.size(); ++i )
        test_assert_int_equal( kw.at( i ), vec[ i ] );
}

void test_kw_vector_string() {
    std::vector< const char* > vec = {
        "short",
        "sweet",
        "padded  ",
        "verylongkeyword"
    };

    ERT::EclKW< const char* > kw( "XYZ", vec );

    test_assert_size_t_equal( kw.size(), vec.size() );

    test_assert_string_equal( kw.at( 0 ), "short   " );
    test_assert_string_equal( kw.at( 1 ), "sweet   " );
    test_assert_string_equal( kw.at( 2 ), vec.at( 2 ) );
    test_assert_string_equal( kw.at( 3 ), "verylong" );
    test_assert_string_not_equal( kw.at( 2 ), "verylongkeyword" );
}

int main (int argc, char **argv) {
    test_kw_vector_assign();
    test_kw_vector_string();
}
