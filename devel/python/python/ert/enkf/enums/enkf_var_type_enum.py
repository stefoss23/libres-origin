#  Copyright (C) 2013  Statoil ASA, Norway.
#
#  The file 'content_type_enum.py' is part of ERT - Ensemble based Reservoir Tool.
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
from ert.cwrap import BaseCEnum
from ert.enkf import ENKF_LIB


class EnkfVarType(BaseCEnum):
    PARAMETER = None
    DYNAMIC_STATE = None
    DYNAMIC_RESULT = None


EnkfVarType.addEnum("PARAMETER", 1)
EnkfVarType.addEnum("DYNAMIC_STATE", 2)
EnkfVarType.addEnum("DYNAMIC_RESULT", 4)
EnkfVarType.registerEnum(ENKF_LIB, "enkf_var_type_enum")



