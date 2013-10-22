from ert.enkf.plot import PlotDataFetcher
from ert_gui.models import ErtConnector
from ert_gui.models.mixins import ModelMixin


class EnsembleSummaryPlot(ErtConnector, ModelMixin):

    def getPlotData(self):
        return PlotDataFetcher(self.ert()).fetchData()



