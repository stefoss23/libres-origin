#  Copyright (C) 2012  Statoil ASA, Norway. 
#   
#  The file 'ert_templates.py' is part of ERT - Ensemble based Reservoir Tool. 
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

import  ctypes
from    ert.cwrap.cwrap       import *
from    ert.cwrap.cclass      import CClass
from    ert.util.tvector      import * 
from    enkf_enum             import *
import  libenkf
class ErtTemplates(CClass):
    
    def __init__(self , c_ptr = None):
        self.owner = False
        self.c_ptr = c_ptr
        
        
    def __del__(self):
        if self.owner:
            cfunc.free( self )

    @property
    def alloc_list(self):
        return cfunc.alloc_list(self)

    def clear(self):
        cfunc.clear(self)

    def get_template(self,key):
        template = ert.enkf.ert_template.ErtTemplate( cfunc.get_template( self, key ))
        return template
    



##################################################################

cwrapper = CWrapper( libenkf.lib )
cwrapper.registerType( "ert_templates" , ErtTemplates )

cfunc = CWrapperNameSpace("ert_templates")
##################################################################
##################################################################
cfunc.free                   = cwrapper.prototype("void ert_templates_free( ert_templates )")
cfunc.alloc_list             = cwrapper.prototype("c_void_p ert_templates_alloc_list(ert_templates)")
cfunc.get_template           = cwrapper.prototype("c_void_p ert_templates_get_template(ert_templates, char*)")
cfunc.clear                  = cwrapper.prototype("void ert_templates_clear(ert_templates)")
cfunc.add_template           = cwrapper.prototype("void ert_templates_add_template(ert_templates, char*, char*, char*, char*)")
