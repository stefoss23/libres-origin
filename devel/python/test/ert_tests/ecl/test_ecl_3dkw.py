#!/usr/bin/env python
#  Copyright (C) 2011  Statoil ASA, Norway. 
#   
#  The file 'test_kw.py' is part of ERT - Ensemble based Reservoir Tool.
#   
#  ERT is free software: you can redistribute it and/or modify 
#  it under the terms of the GNU General Public License as published by 
#  the Free Software Foundation, either version 3 of the License, or 
#  (at your option) any later version. 
#   
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#  FITNESS FOR A PARTICULAR PURPOSE.   
#   
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
#  for more details.
import os
import random

from ert.util import IntVector
from ert.ecl import Ecl3DKW , EclKW, EclTypeEnum, EclFile, FortIO, EclFileFlagEnum , EclGrid
from ert.test import ExtendedTestCase , TestAreaContext



class Ecl3DKWTest(ExtendedTestCase):

    def test_create( self ):
        actnum = IntVector(default_value = 1 , initial_size = 1000)
        for i in range(100):
            actnum[i] = 0

        grid = EclGrid.createRectangular( (10,10,10) , (1,1,1) , actnum = actnum)
        kw = Ecl3DKW.create( "KW" , grid , EclTypeEnum.ECL_FLOAT_TYPE )
        self.assertEqual( len(kw) , grid.getNumActive())

        self.assertEqual( (10,10,10) , kw.dims() )


    def test_create_global_size( self ):
        actnum = IntVector(default_value = 1 , initial_size = 1000)
        for i in range(100):
            actnum[i] = 0

        grid = EclGrid.createRectangular( (10,10,10) , (1,1,1) , actnum = actnum)
        kw = Ecl3DKW.create( "KW" , grid , EclTypeEnum.ECL_FLOAT_TYPE , global_active = True)
        self.assertEqual( len(kw) , grid.getGlobalSize())

        kw.assign(50)
        self.assertEqual( kw[0,0,0] , 50 )

        kw[0,0,0] = 45
        self.assertEqual( kw[0,0,0] , 45 )


    def test_fix_uninitialized(self):
        nx = 10
        ny = 11
        nz = 12
        grid = EclGrid.createRectangular( (nx,ny,nz) , (1,1,1) )
        kw = Ecl3DKW.create("REGIONS" , grid , EclTypeEnum.ECL_INT_TYPE , global_active = True)
        kw.assign(3)
        self.assertEqual( 3 * nx*ny*nz , sum(kw))

        kw[1,1,1] = 0
        kw[3,3,3] = 0
        kw[6,6,6] = 0

        self.assertEqual( 3 * nx*ny*nz - 9 , sum(kw))
        kw.fixUninitialized( grid.getDims() )
        self.assertEqual( 3 * nx*ny*nz , sum(kw))



    def test_getitem( self ):
        actnum = IntVector(default_value = 1 , initial_size = 1000)
        for i in range(100):
            actnum[i] = 0

        grid = EclGrid.createRectangular( (10,10,10) , (1,1,1) , actnum = actnum)
        kw = Ecl3DKW.create( "KW" , grid , EclTypeEnum.ECL_FLOAT_TYPE , default_value = 77)
        
        with self.assertRaises(IndexError):
            kw[1000]

        with self.assertRaises(IndexError):
            kw[0,10,100]

        with self.assertRaises(ValueError):
            kw[1,1]

        with self.assertRaises(ValueError):
            kw[1,1,1,1]

        kw.assign(99)

        self.assertEqual( kw[0,0,0] , 77 )
        self.assertEqual( kw[0,0,1] , 99 )
        

        
    def test_setitem( self ):
        actnum = IntVector(default_value = 1 , initial_size = 1000)
        for i in range(100):
            actnum[i] = 0

        grid = EclGrid.createRectangular( (10,10,10) , (1,1,1) , actnum = actnum)
        kw = Ecl3DKW.create( "KW" , grid , EclTypeEnum.ECL_FLOAT_TYPE , default_value = 77)
        
        with self.assertRaises(IndexError):
            kw[1000]

        with self.assertRaises(IndexError):
            kw[0,10,100]

        with self.assertRaises(ValueError):
            kw[1,1]

        with self.assertRaises(ValueError):
            kw[1,1,1,1]

        kw.assign(99)
        self.assertEqual( kw[0,0,0] , 77 )
        self.assertEqual( kw[0,0,1] , 99 )

        
        with self.assertRaises(ValueError):
            kw[0,0,0] = 88
            
        kw[0,0,1] = 100 
        self.assertEqual( kw[0,0,1] , 100 )



    def test_cast(self):
        actnum = IntVector(default_value = 1 , initial_size = 1000)
        for i in range(100):
            actnum[i] = 0

        grid = EclGrid.createRectangular( (10,10,10) , (1,1,1) , actnum = actnum)
        kw_wrong_size  = EclKW.create( "KW" , 27 , EclTypeEnum.ECL_FLOAT_TYPE )
        kw_global_size = EclKW.create( "KW" , grid.getGlobalSize() , EclTypeEnum.ECL_FLOAT_TYPE )
        kw_active_size = EclKW.create( "KW" , grid.getNumActive()  , EclTypeEnum.ECL_FLOAT_TYPE )
        
        with self.assertRaises(ValueError):
            Ecl3DKW.castFromKW(kw_wrong_size , grid)

        Ecl3DKW.castFromKW(kw_global_size , grid)
        self.assertTrue( isinstance( kw_global_size , Ecl3DKW))

        Ecl3DKW.castFromKW(kw_active_size , grid , default_value = 66)
        self.assertTrue( isinstance( kw_active_size , Ecl3DKW))

        self.assertEqual( kw_active_size[0,0,0] , 66)
        with self.assertRaises(ValueError):
            kw_active_size[0,0,0] = 88


    def test_default(self):
        grid = EclGrid.createRectangular( (10,10,10) , (1,1,1))
        kw  = Ecl3DKW.create( "KW" , grid , EclTypeEnum.ECL_FLOAT_TYPE )
        kw.setDefault(55)
        self.assertTrue( 55 , kw.getDefault())
