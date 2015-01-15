from ert.enkf.export import DesignMatrixReader, SummaryCollector, GenKwCollector
from ert.test import ExtendedTestCase, ErtTestContext
import pandas
import numpy


class ExportJoinTest(ExtendedTestCase):

    def setUp(self):
        self.config = self.createTestPath("Statoil/config/with_data/config")
        self.design_matrix_file = self.createTestPath("Statoil/ert-statoil/spotfire/DesignMatrix_25.txt")

    def test_join(self):

        with ErtTestContext("python/enkf/export/export_join", self.config) as context:
            ert = context.getErt()

            dm = DesignMatrixReader.loadDesignMatrix(self.design_matrix_file)
            summary_data = SummaryCollector.loadAllSummaryData(ert, "default")
            gen_kw_data = GenKwCollector.loadAllGenKwData(ert, "default")

            result = summary_data.join(gen_kw_data, how='inner')
            result = result.join(dm, how='inner')

            self.assertFloatEqual(result["FLUID_PARAMS:SGCR"][0]["2000-02-01"], 0.295136)
            self.assertFloatEqual(result["FLUID_PARAMS:SGCR"][24]["2000-02-01"], 0.177833)
            self.assertFloatEqual(result["FLUID_PARAMS:SGCR"][24]["2004-12-01"], 0.177833)

            self.assertFloatEqual(result["EXTRA_FLOAT_COLUMN"][0]["2000-02-01"], 0.08)
            self.assertEqual(result["EXTRA_INT_COLUMN"][0]["2000-02-01"], 125)
            self.assertEqual(result["EXTRA_STRING_COLUMN"][0]["2000-02-01"], "ON")

            self.assertFloatEqual(result["EXTRA_FLOAT_COLUMN"][0]["2004-12-01"], 0.08)
            self.assertEqual(result["EXTRA_INT_COLUMN"][0]["2004-12-01"], 125)
            self.assertEqual(result["EXTRA_STRING_COLUMN"][0]["2004-12-01"], "ON")

            self.assertFloatEqual(result["EXTRA_FLOAT_COLUMN"][1]["2004-12-01"], 0.07)
            self.assertEqual(result["EXTRA_INT_COLUMN"][1]["2004-12-01"], 225)
            self.assertEqual(result["EXTRA_STRING_COLUMN"][1]["2004-12-01"], "OFF")


            with self.assertRaises(KeyError):
                realization_13 = result.loc[13]

            column_count = len(result.columns)
            self.assertEqual(result.dtypes[0], numpy.float64)
            self.assertEqual(result.dtypes[column_count - 1], numpy.object)
            self.assertEqual(result.dtypes[column_count - 2], numpy.int64)
